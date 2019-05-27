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
#include <acfutils/helpers.h>
#include <acfutils/hexcode.h>
#include <acfutils/safe_alloc.h>
#include <acfutils/thread.h>

#include "auth.h"

#define	REALLOC_STEP	(16 << 10)	/* 16 KiB */
#define	AUTH_TIMEOUT	30L		/* seconds */
#define	MAX_DL_SIZE	(128 << 10)	/* 128 KiB */

typedef struct {
	auth_sess_key_t	key;
	thread_t	thread;
	bool		kill;
	char		*postdata;
	auth_done_cb_t	done_cb;
	void		*userinfo;
	avl_node_t	node;
} auth_sess_t;

/*
 * Download info structure used by our CURL_WRITEFUNCTION (dl_write).
 * This contains the incoming data buffer and session structure pointer.
 */
typedef struct {
	auth_sess_t	*sess;
	uint8_t		*buf;
	size_t		bufcap;
	size_t		bufsz;
} dl_info_t;

static bool		inited = false;
static char		auth_url[PATH_MAX] = { 0 };
static char		cainfo[PATH_MAX] = { 0 };
static char		auth_username[64] = { 0 };
static char		auth_password[64] = { 0 };
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

/*
 * cURL data download write callback (CURLOPT_WRITEFUNCTION).
 * This is used to grab all incoming server response data.
 */
static size_t
dl_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	dl_info_t *dl_info = userdata;
	size_t bytes = size * nmemb;

	ASSERT(dl_info != NULL);
	ASSERT(dl_info->sess != NULL);

	/* Respond to an early termination request with a write error */
	mutex_enter(&lock);
	if (dl_info->sess->kill) {
		mutex_exit(&lock);
		return (0);
	}
	mutex_exit(&lock);
	/*
	 * Protect against the authenticator flooding us with a giant
	 * response. This is mostly to protect against somebody accidentally
	 * pointing the auth URL at something like a giant file download.
	 */
	if (dl_info->bufsz + bytes > MAX_DL_SIZE) {
		logMsg("auth_sess: remote authenticator %s has returned "
		    "too much data (%ld bytes), bailing out", auth_url,
		    (long)(dl_info->bufsz + bytes));
		return (0);
	}
	/*
	 * The server response is expected to be x-www-form-urlencoded
	 * plain text.
	 */
	if (dl_info->bufcap < dl_info->bufsz + bytes) {
		dl_info->bufcap += REALLOC_STEP;
		dl_info->buf = safe_realloc(dl_info->buf, dl_info->bufcap + 1);
	}
	memcpy(&dl_info->buf[dl_info->bufsz], ptr, bytes);
	dl_info->bufsz += bytes;
	/* Make sure the buffer is properly NUL-terminated */
	dl_info->buf[dl_info->bufsz] = '\0';

	return (bytes);
}

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
 * Generic cURL session setup function. This sets a number of default
 * cURL options such as session timeout, write function and signal handling.
 */
static void
setup_curl(CURL *curl)
{
	ASSERT(curl != NULL);
	ASSERT(auth_url[0] != '\0');

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, AUTH_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dl_write);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, auth_url);
	if (cainfo[0] != '\0')
		curl_easy_setopt(curl, CURLOPT_CAINFO, cainfo);
	if (auth_username[0] != '\0')
		curl_easy_setopt(curl, CURLOPT_USERNAME, auth_username);
	if (auth_password[0] != '\0')
		curl_easy_setopt(curl, CURLOPT_PASSWORD, auth_password);
}

/*
 * Parses the authenticator x-www-form-urlencoded response. We're looking
 * for two fields: auth=0/1 (authentication success/failure) and
 * atc=0/1 (station is ATC or aircraft).
 *
 * @param buf Authenticator response data string (expected to be plain text).
 * @param auth_result Will be filled with the "auth" field. If no "auth"
 *	field is found, this is set to `false'.
 * @param atc_result Will be filled with the "atc" field. If no "atc"
 *	field is found, this is set to `false'.
 */
static void
parse_auth_response(const char *buf, bool *auth_result, bool *auth_atc)
{
	size_t num_comps;
	char **comps;

	ASSERT(buf != NULL);
	ASSERT(auth_result != NULL);
	ASSERT(auth_atc != NULL);

	comps = strsplit(buf, "&", true, &num_comps);
	/* Default response is to deny everything */
	*auth_result = false;
	*auth_atc = false;

	for (size_t i = 0; i < num_comps; i++) {
		strtolower(comps[i]);
		if (strncmp(comps[i], "auth=", 5) == 0) {
			*auth_result = !!atoi(&comps[i][5]);
		} else if (strncmp(comps[i], "atc=", 4) == 0) {
			*auth_atc = !!atoi(&comps[i][4]);
		}
	}
	free_strlist(comps, num_comps);
}

/*
 * This is the background authentication thread worker function.
 * This function performs the actual HTTP POST to the remote authenticator,
 * collects its response and calls the authentication completion callback.
 *
 * @param userinfo A pointer to the auth_sess_t for this auth worker.
 */
static void
auth_worker(void *userinfo)
{
	auth_sess_t *sess = userinfo;
	CURL *curl;
	dl_info_t dl_info = { .sess = sess };
	CURLcode res;
	long code = 0;
	struct curl_slist *hdrs = curl_slist_append(NULL,
	    "Content-Type: application/x-www-form-urlencoded");

	ASSERT(sess != NULL);
	thread_set_name("auth_worker");
	/*
	 * cURL session setup. All POST data has already been prepared,
	 * so we just pass it on here. See `auth_sess_open' for a
	 * description of the POST fields we send.
	 */
	curl = curl_easy_init();
	VERIFY(curl != NULL);
	setup_curl(curl);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dl_info);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sess->postdata);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

	res = curl_easy_perform(curl);

	mutex_enter(&lock);

	/*
	 * In case of early kill, don't call done_cb, just exit.
	 */
	if (!sess->kill) {
		bool auth_result = false;
		bool auth_atc = false;

		/* If we weren't killed early */
		if (res == CURLE_OK)
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		if (res == CURLE_OK && code == 200 && dl_info.bufsz != 0) {
			parse_auth_response((const char *)dl_info.buf,
			    &auth_result, &auth_atc);
		} else {
			if (res != CURLE_OK) {
				logMsg("Error querying authenticator %s: "
				    "%s", auth_url, curl_easy_strerror(res));
			} else if (dl_info.bufsz == 0) {
				logMsg("Error querying authenticator %s: "
				    "no data in response", auth_url);
			} else {
				logMsg("Error querying authenticator %s: "
				    "HTTP error %ld", auth_url, code);
			}
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
	free(dl_info.buf);
	curl_easy_cleanup(curl);
	curl_slist_free_all(hdrs);
	/* This is kinda sensitive, so zero out before freeing */
	memset(sess->postdata, 0, strlen(sess->postdata));
	free(sess->postdata);
	memset(sess, 0, sizeof (*sess));
	free(sess);
}

/*
 * Authenticator system global initializer function.
 *
 * @param url URL with which to authenticate. The authenticator will be
 *	sending HTTP POST requests with the authentication data in the
 *	request body. See `auth_sess_open' for details on what is in
 *	included in the authentication body.
 * @param new_cainfo A path to cainfo file for cURL, containing a list
 *	of trusted certificate authorities. This can be NULL, in which
 *	case the authenticator uses the system-installed trusted
 *	certificate store (typically /usr/share/ca-certificates or
 *	/etc/ssl/certs on Linux machines).
 * @param new_username If not NULL, cURL is instructed to use HTTP basic
 *	authentication with this username with the remote authenticator.
 * @param new_password If not NULL, cURL is instructed to use HTTP basic
 *	authentication with this password with the remote authenticator.
 */
void
auth_init(const char *url, const char *new_cainfo,
    const char *new_username, const char *new_password)
{
	ASSERT(!inited);
	inited = true;

	if (url != NULL)
		lacf_strlcpy(auth_url, url, sizeof (auth_url));
	if (new_cainfo != NULL)
		lacf_strlcpy(cainfo, new_cainfo, sizeof (cainfo));
	if (new_username != NULL) {
		lacf_strlcpy(auth_username, new_username,
		    sizeof (auth_username));
	}
	if (new_password != NULL) {
		lacf_strlcpy(auth_password, new_password,
		    sizeof (auth_password));
	}

	mutex_init(&lock);
	avl_create(&sessions, sess_compar, sizeof (auth_sess_t),
	    offsetof(auth_sess_t, node));
	cv_init(&sess_shutdown_cv);
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
auth_sess_open(const cpdlc_msg_t *logon_msg, const void *sockaddr,
    auth_done_cb_t done_cb, void *userinfo)
{
	auth_sess_t *sess;
	char *tmpstr;
	char addrbuf[64];
	size_t cap = 0;
	uint64_t key;
	/* temp curl context only used for URL-escaping purposes */
	CURL *curl;
	sa_family_t addr_family;

	ASSERT(logon_msg != NULL);
	ASSERT(sockaddr != NULL);
	addr_family = ((struct sockaddr *)sockaddr)->sa_family;
	ASSERT(addr_family == AF_INET || addr_family == AF_INET6);
	ASSERT(done_cb != NULL);

	if (auth_url[0] == '\0') {
		done_cb(true, true, userinfo);
		return (0);
	}

	curl = curl_easy_init();
	VERIFY(curl != NULL);

	sess = safe_calloc(1, sizeof (*sess));
	sess->done_cb = done_cb;
	sess->userinfo = userinfo;

	/*
	 * We pass the following fields in the HTTP POST data:
	 *
	 * "LOGON" - the original data passed in the "LOGON=" field of the
	 *	LOGON message. The meaning of this is to be interpreted by
	 *	the authenticator. It can be a secret password, a separate
	 *	"username/password" combo or the current positions of the
	 *	stars, for all we care.
	 * "FROM" - the "FROM=" data field in the LOGON message. This will
	 *	be the connection's new identity on the CPDLC network.
	 * "TO" - if present, the "TO=" data field in the LOGON message.
	 *	The server doesn't attempt to validate if this is the
	 *	identity of an ATC station. That is up to the authenticator.
	 * "REMOTEADDR" - the remote connection address. If addr_family is
	 *	AF_INET, this will be an IPv4 address in dotted-quad
	 *	notation. If addr_family is AF_INET6, this will be an IPv6
	 *	address in hex-colon notation.
	 *
	 * We expect the authenticator to respond with two
	 * x-www-form-urlencoded fields:
	 * "auth" - a boolean 0/1 value indicating whether authentication
	 *	was successful
	 * "atc" - if authentication was success, this is a boolean 0/1
	 *	value indicating whether the connection is an ATC station (1)
	 *	or an aircraft station (0).
	 */
	ASSERT(cpdlc_msg_get_logon_data(logon_msg) != NULL);
	tmpstr = curl_easy_escape(curl, cpdlc_msg_get_logon_data(logon_msg), 0);
	append_format(&sess->postdata, &cap, "LOGON=%s", tmpstr);
	curl_free(tmpstr);

	ASSERT(cpdlc_msg_get_from(logon_msg) != NULL);
	tmpstr = curl_easy_escape(curl, cpdlc_msg_get_from(logon_msg), 0);
	append_format(&sess->postdata, &cap, "&FROM=%s", tmpstr);
	curl_free(tmpstr);

	if (cpdlc_msg_get_to(logon_msg) != NULL) {
		tmpstr = curl_easy_escape(curl, cpdlc_msg_get_to(logon_msg), 0);
		append_format(&sess->postdata, &cap, "&TO=%s", tmpstr);
		curl_free(tmpstr);
	}
	if (addr_family == AF_INET) {
		const struct sockaddr_in *sockaddr_v4 =
		    (const struct sockaddr_in *)sockaddr;
		inet_ntop(addr_family, &sockaddr_v4->sin_addr, addrbuf,
		    sizeof (addrbuf));
	} else {
		char addrbuf[64];
		const struct sockaddr_in6 *sockaddr_v6 =
		    (const struct sockaddr_in6 *)sockaddr;
		inet_ntop(addr_family, &sockaddr_v6->sin6_addr, addrbuf,
		    sizeof (addrbuf));
	}
	tmpstr = curl_easy_escape(curl, addrbuf, 0);
	append_format(&sess->postdata, &cap, "&REMOTEADDR=%s", tmpstr);
	curl_free(tmpstr);

	mutex_enter(&lock);
	key = sess->key = next_sess_key++;
	avl_add(&sessions, sess);
	VERIFY(thread_create(&sess->thread, auth_worker, sess));
	mutex_exit(&lock);
	/*
	 * Mustn't touch `sess' after this! done_cb might have by fired now.
	 */

	curl_easy_cleanup(curl);

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
