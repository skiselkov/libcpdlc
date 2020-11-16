/*
 * Copyright 2019 Saso Kiselkov
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stddef.h>
#include <stdio.h>

#include <arpa/inet.h>

#include <curl/curl.h>

#include <acfutils/avl.h>
#include <acfutils/assert.h>
#include <acfutils/base64.h>
#include <acfutils/helpers.h>
#include <acfutils/hexcode.h>
#include <acfutils/safe_alloc.h>
#include <acfutils/taskq.h>
#include <acfutils/thread.h>

#include "auth.h"
#include "common.h"
#include "rpc.h"

#define	REALLOC_STEP	(16 << 10)	/* 16 KiB */
#define	AUTH_TIMEOUT	30L		/* seconds */
#define	MAX_DL_SIZE	(128 << 10)	/* 128 KiB */

typedef struct {
	auth_sess_key_t	key;
	thread_t	thread;
	bool		kill;
	auth_done_cb_t	done_cb;
	void		*userinfo;
	avl_node_t	node;

	char		remote_addr[SOCKADDR_STRLEN];
	char		from[CALLSIGN_LEN];
	char		to[CALLSIGN_LEN];
	char		logon_data[128];
} auth_sess_t;

typedef struct {
	bool		logon;
	char		remote_addr[SOCKADDR_STRLEN];
	char		from[CALLSIGN_LEN];
	char		to[CALLSIGN_LEN];
	bool		is_atc;
	bool		is_lws;
} logon_notify_t;

typedef struct {
	CURL		*curl;
} logon_notifier_t;

static bool		inited = false;
static char		auth_url[PATH_MAX] = { 0 };
static rpc_spec_t	rpc_spec = {};
static mutex_t		lock;
static auth_sess_key_t	next_sess_key = 0;	/* next session key to use */
/*
 * Tree of auth_sess_t structures, keyed by their `key' values.
 */
static avl_tree_t	sessions;
/*
 * This condition variable gets signalled by an authentication session
 * worker just as it is about to exit. This is used auth_fini to wait
 * for all authentication session to shut down before returning.
 */
static condvar_t	sess_shutdown_cv;

static struct {
	taskq_t		*tq;
	rpc_spec_t	rpc_spec;
} notify = {};

static void *logon_notifier_init(void *userinfo);
static void logon_notifier_fini(void *userinfo, void *thr_info);
static void logon_notifier_proc(void *userinfo, void *thr_info, void *task);
static void logon_notifier_discard(void *userinfo, void *task);

/*
 * `sessions' AVL tree comparator function.
 */
static int
sess_compar(const void *a, const void *b)
{
	const auth_sess_t *sa = a, *sb = b;
	if (sa->key < sb->key)
		return (-1);
	if (sa->key > sb->key)
		return (1);
	return (0);
}

/*
 * This is the background authentication thread worker function.
 * This function performs the actual RPC to the remote authenticator,
 * collects its response and calls the authentication completion callback.
 *
 * @param userinfo A pointer to the auth_sess_t for this auth worker.
 */
static void
auth_worker(void *userinfo)
{
	auth_sess_t *sess = userinfo;
	rpc_result_t result;
	bool rpc_ret;
	CURL *curl;

	ASSERT(sess != NULL);
	thread_set_name("auth_worker");
	/*
	 * cURL session setup. All POST data has already been prepared,
	 * so we just pass it on here. See `auth_sess_open' for a
	 * description of the POST fields we send.
	 */
	curl = curl_easy_init();
	VERIFY(curl != NULL);
	rpc_curl_setup(curl, &rpc_spec);
	rpc_ret = rpc_perform(&rpc_spec, &result, curl,
	    "LOGON", sess->logon_data,
	    "TO", sess->to,
	    "FROM", sess->from,
	    "ADDR", sess->remote_addr,
	    NULL);

	mutex_enter(&lock);
	/*
	 * In case of early kill, don't call done_cb, just exit.
	 */
	if (!sess->kill) {
		bool auth_result = false;
		bool auth_atc = false;

		if (rpc_ret) {
			auth_result = (result.num_results >= 1 &&
			    atoi(result.values[0]) != 0);
			auth_atc = (result.num_results >= 2 &&
			    atoi(result.values[1]) != 0);
		}
		ASSERT(sess->done_cb != NULL);
		sess->done_cb(auth_result, auth_atc, sess->userinfo);
	}
	avl_remove(&sessions, sess);
	cv_broadcast(&sess_shutdown_cv);

	mutex_exit(&lock);
	/*
	 * All of the remaining state is held by us and not visible globally.
	 */
	curl_easy_cleanup(curl);
	/* This is kinda sensitive, so zero out before freeing */
	memset(sess, 0, sizeof (*sess));
	free(sess);
}

static bool
parse_conf_logon_notify(const conf_t *conf)
{
	int min_threads = 0, max_threads = 4;
	uint64_t stop_delay = SEC2USEC(2);

	ASSERT(conf != NULL);

	if (!rpc_spec_parse(conf, "logon_notify/rpc", false, &notify.rpc_spec))
		return (false);
	if (conf_get_i(conf, "logon_notify/min_threads", &min_threads))
		min_threads = MAX(min_threads, 0);
	if (conf_get_i(conf, "logon_notify/max_threads", &max_threads))
		max_threads = MAX(max_threads, min_threads);
	if (conf_get_lli(conf, "logon_notify/stop_delay",
	    (long long *)&stop_delay)) {
		stop_delay = SEC2USEC(stop_delay);
	}
	notify.tq = taskq_alloc(min_threads, max_threads, stop_delay,
	    logon_notifier_init, logon_notifier_fini,
	    logon_notifier_proc, logon_notifier_discard, NULL);

	return (true);
}

/*
 * Authenticator system global initializer function.
 */
bool
auth_init(const conf_t *conf)
{
	ASSERT(!inited);
	inited = true;

	mutex_init(&lock);
	avl_create(&sessions, sess_compar, sizeof (auth_sess_t),
	    offsetof(auth_sess_t, node));
	cv_init(&sess_shutdown_cv);

	if (conf != NULL) {
		const char *str;

		if (conf_get_str(conf, "auth/rpc/url", &str))
			lacf_strlcpy(auth_url, str, sizeof (auth_url));
		if (strncmp(auth_url, "file://", 7) != 0 &&
		    !rpc_spec_parse(conf, "auth/rpc", true, &rpc_spec)) {
			return (false);
		}
		if (conf_get_str(conf, "logon_notify/rpc/url", &str) &&
		    !parse_conf_logon_notify(conf)) {
			return (false);
		}
	}

	return (true);
}

/*
 * Authenticator system global cleanup function.
 */
void
auth_fini(void)
{
	if (!inited)
		return;
	inited = false;

	if (notify.tq != NULL) {
		taskq_free(notify.tq);
		notify.tq = NULL;
	}
	mutex_enter(&lock);
	for (auth_sess_t *sess = avl_first(&sessions); sess != NULL;
	    sess = AVL_NEXT(&sessions, sess)) {
		sess->kill = true;
	}
	while (avl_numnodes(&sessions) != 0)
		cv_wait(&sess_shutdown_cv, &lock);
	mutex_exit(&lock);

	cv_destroy(&sess_shutdown_cv);
	avl_destroy(&sessions);
	mutex_destroy(&lock);
}

static void
auth_file(const cpdlc_msg_t *logon_msg, auth_done_cb_t done_cb, void *userinfo)
{
	FILE *fp;
	CURL *curl;
	char *from = NULL, *to = NULL;
	char *line = NULL;
	size_t linecap = 0;
	bool success = false, failure = false, policy_allow = false;
	unsigned linenum = 0;

	ASSERT(logon_msg != NULL);
	ASSERT(done_cb != NULL);
	ASSERT3U(strlen(auth_url), >, 7);

	/* This is simply to get curl_easy_escape */
	curl = curl_easy_init();

	fp = fopen(&auth_url[7], "r");
	if (fp == NULL) {
		logMsg("Can't open auth/url %s: %s", auth_url, strerror(errno));
		goto errout;
	}
	ASSERT(cpdlc_msg_get_logon_data(logon_msg) != NULL);
	ASSERT(cpdlc_msg_get_from(logon_msg) != NULL);

	from = curl_easy_escape(curl, cpdlc_msg_get_from(logon_msg), 0);
	if (cpdlc_msg_get_to(logon_msg) != NULL)
		to = curl_easy_escape(curl, cpdlc_msg_get_to(logon_msg), 0);

	while (parser_get_next_line(fp, &line, &linecap, &linenum) > 0 &&
	    !success) {
		char **comps;
		size_t n_comps;

		comps = strsplit(line, ":", false, &n_comps);
		if (n_comps != 2 && n_comps != 3) {
			logMsg("%s:%d: malformed line, each line must "
			    "have exactly 2 or 3 parts, separated by "
			    "whitespace", auth_url, linenum);
			continue;
		}
		if (strcasecmp(comps[0], "policy") == 0) {
			policy_allow = (strcasecmp(comps[1], "allow") == 0);
			continue;
		}
		if (strcmp(comps[0], from) == 0) {
			char *encr = crypt(cpdlc_msg_get_logon_data(logon_msg),
			    comps[1]);

			if (comps[1][0] == '\0' ||
			    strcmp(comps[1], encr) == 0) {
				bool is_atc = false;
				/* logon success, determine if it's ATC */
				if (n_comps == 3 &&
				    strcasecmp(comps[2], "atc") == 0) {
					is_atc = true;
				}
				done_cb(true, is_atc, userinfo);
				success = true;
			} else {
				failure = true;
			}
			free_strlist(comps, n_comps);
			break;
		}

		free_strlist(comps, n_comps);
	}

	if (!success) {
		if (!failure && policy_allow)
			done_cb(true, false, userinfo);
		else
			done_cb(false, false, userinfo);
	}

	curl_free(from);
	curl_free(to);
	curl_easy_cleanup(curl);
	lacf_free(line);
	fclose(fp);

	return;
errout:
	done_cb(false, false, userinfo);
	curl_free(from);
	curl_free(to);
	curl_easy_cleanup(curl);
	lacf_free(line);
	if (fp != NULL)
		fclose(fp);
}

/*
 * Initiates a new authentication session. The session will run in
 * a background thread and call a completion callback when the remote
 * authentication server has responded.
 *
 * This function does all necessary preparation and data serialization
 * here. The background thread then simply constructs a cURL context
 * and passes that data into the cURL context. This avoids having to
 * hold on to the passed logon message or any sockaddr structures.
 *
 * @param logon_msg The original logon message that caused this
 *	authentication request to be generated. The caller retains
 *	ownership of the message.
 * @param addr The sockaddr structure of the connection's remote end.
 * @param done_cb A completion callback that will be invoked once the
 *	authentication session has finished processing. If the session
 *	encountered a communication error with the remote authenticator,
 *	the callback is called with a "logon failed" result. If the
 *	authentication session is killed early by a call to
 *	`auth_sess_kill', this callback will NOT be called.
 * @param userinfo An optional userinfo parameter that will be passed
 *	to the completion callback.
 *
 * @return An authentication session ID. This can be used in
 *	auth_sess_kill to terminate the session early.
 */
auth_sess_key_t
auth_sess_open(const cpdlc_msg_t *logon_msg, const char *remote_addr,
    auth_done_cb_t done_cb, void *userinfo)
{
	auth_sess_t *sess;
	uint64_t key;

	ASSERT(logon_msg != NULL);
	ASSERT(remote_addr != NULL);
	ASSERT(done_cb != NULL);

	if (auth_url[0] == '\0') {
		done_cb(true, true, userinfo);
		return (0);
	}
	if (strncmp(auth_url, "file://", 7) == 0) {
		auth_file(logon_msg, done_cb, userinfo);
		return (0);
	}

	sess = safe_calloc(1, sizeof (*sess));
	sess->done_cb = done_cb;
	sess->userinfo = userinfo;
	/*
	 * Save the data we'll need in the authenticator.
	 */
	ASSERT(cpdlc_msg_get_logon_data(logon_msg) != NULL);
	strlcpy(sess->logon_data, cpdlc_msg_get_logon_data(logon_msg),
	    sizeof (sess->logon_data));
	ASSERT(cpdlc_msg_get_from(logon_msg) != NULL);
	strlcpy(sess->from, cpdlc_msg_get_from(logon_msg), sizeof (sess->from));
	if (cpdlc_msg_get_to(logon_msg) != NULL) {
		strlcpy(sess->to, cpdlc_msg_get_to(logon_msg),
		    sizeof (sess->to));
	}
	strlcpy(sess->remote_addr, remote_addr, sizeof (sess->remote_addr));

	mutex_enter(&lock);
	key = sess->key = next_sess_key++;
	avl_add(&sessions, sess);
	VERIFY(thread_create(&sess->thread, auth_worker, sess));
	mutex_exit(&lock);
	/*
	 * Mustn't touch `sess' after this! done_cb might have by fired now.
	 */

	return (key);
}

/*
 * Kills an authentication session early. If the session no longer exists,
 * this function does nothing. If the session is still in progress, it is
 * marked as `killed'. This will tell the session to terminate early and
 * NOT call the authentication completion callback.
 */
void
auth_sess_kill(auth_sess_key_t key)
{
	auth_sess_t srch = { .key = key };
	auth_sess_t *sess;

	mutex_enter(&lock);
	ASSERT3U(key, <, next_sess_key);
	sess = avl_find(&sessions, &srch, NULL);
	if (sess != NULL)
		sess->kill = true;
	mutex_exit(&lock);
}

bool
auth_encrypt_userpwd(bool silent)
{
	enum { SALT_SZ = 8 };
	size_t cap = 0;
	char *password = NULL;
	char saltraw[SALT_SZ];
	char saltstr[BASE64_ENC_SIZE(SALT_SZ) + 8] = { 0 };
	char *buf;
	FILE *fp;

	if (!silent) {
		printf("Password: ");
		fflush(stdout);
	}

	if (getline(&password, &cap, stdin) <= 0) {
		fprintf(stderr, "Error: expected password input\n");
		return (false);
	}
	ASSERT3U(strlen(password), >, 0);
	/* Strip a trailing newline */
	if (password[strlen(password) - 1] == '\n')
		password[strlen(password) - 1] = '\0';

	fp = fopen("/dev/urandom", "r");
	if (fp == NULL) {
		fprintf(stderr, "Can't open /dev/urandom: %s\n",
		    strerror(errno));
		lacf_free(password);
		return (false);
	}
	if (fread(saltraw, 1, sizeof (saltraw), fp) != SALT_SZ) {
		fprintf(stderr, "Can't read /dev/urandom: short byte "
		    "count read\n");
		fclose(fp);
		lacf_free(password);
		return (false);
	}
	fclose(fp);

	lacf_strlcpy(saltstr, "$5$", sizeof (saltstr));
	lacf_base64_encode((const uint8_t *)saltraw, sizeof (saltraw),
	    (uint8_t *)&saltstr[3]);
	saltstr[strlen(saltstr) - 1] = '$';

	buf = crypt(password, saltstr);
	ASSERT(buf != NULL);

	printf("%s\n", buf);

	lacf_free(password);

	return (true);
}

static void *
logon_notifier_init(void *userinfo)
{
	logon_notifier_t *nter = safe_calloc(1, sizeof (*nter));

	UNUSED(userinfo);

	nter->curl = curl_easy_init();
	ASSERT(nter->curl != NULL);
	rpc_curl_setup(nter->curl, &notify.rpc_spec);

	return (nter);
}

static void
logon_notifier_fini(void *userinfo, void *thr_info)
{
	logon_notifier_t *nter;

	UNUSED(userinfo);
	ASSERT(thr_info != NULL);
	nter = thr_info;

	curl_easy_cleanup(nter->curl);
	free(nter);
}

static void
logon_notifier_proc(void *userinfo, void *thr_info, void *task)
{
	logon_notifier_t *nter;
	logon_notify_t *ln;
	rpc_result_t result;

	UNUSED(userinfo);
	ASSERT(thr_info != NULL);
	nter = thr_info;
	ASSERT(task != NULL);
	ln = task;

	(void)rpc_perform(&notify.rpc_spec, &result, nter->curl,
	    "TYPE", ln->logon ? "LOGON" : "LOGOFF",
	    "FROM", ln->from,
	    "TO", ln->to,
	    "STATYPE", ln->is_atc ? "ATC" : "ACFT",
	    "CONNTYPE", ln->is_lws ? "LWS" : "TLS",
	    "ADDR", ln->remote_addr,
	    NULL);
	free(ln);
}

static void
logon_notifier_discard(void *userinfo, void *task)
{
	UNUSED(userinfo);
	ASSERT(task != NULL);
	free(task);
}

void
auth_notify_logon(bool is_logon, const char *from, const char *to,
    const char *remote_addr, bool is_atc, bool is_lws)
{
	logon_notify_t *ln;

	ASSERT(inited);
	ASSERT(from != NULL);
	/* to can be NULL */
	ASSERT(remote_addr != NULL);

	if (notify.tq == NULL)
		return;

	ln = safe_calloc(1, sizeof (*ln));
	ln->logon = is_logon;
	strlcpy(ln->from, from, sizeof (ln->from));
	if (to != NULL)
		strlcpy(ln->to, to, sizeof (ln->to));
	else
		strlcpy(ln->to, "-", sizeof (ln->to));
	strlcpy(ln->remote_addr, remote_addr, sizeof (ln->remote_addr));
	ln->is_atc = is_atc;
	ln->is_lws = is_lws;

	taskq_submit(notify.tq, ln);
}
