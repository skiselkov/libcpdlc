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

#define	REALLOC_STEP	(8 << 20)	/* bytes */
#define	AUTH_TIMEOUT	30L		/* seconds */

typedef struct {
	auth_sess_key_t	key;
	thread_t	thread;
	bool		kill;
	char		*postdata;
	auth_done_cb_t	done_cb;
	void		*userinfo;
	avl_node_t	node;
} auth_sess_t;

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
static uint64_t		next_sess_key = 0;
static avl_tree_t	sessions;
static condvar_t	sess_shutdown_cv;

static size_t
dl_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	dl_info_t *dl_info = userdata;
	size_t bytes = size * nmemb;

	ASSERT(dl_info != NULL);
	ASSERT(dl_info->sess != NULL);

	/* Respond to an early termination request */
	mutex_enter(&lock);
	if (dl_info->sess->kill) {
		mutex_exit(&lock);
		return (0);
	}
	mutex_exit(&lock);

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

static void
parse_auth_response(const char *buf, bool *auth_result, bool *auth_atc)
{
	size_t num_comps;
	char **comps;

	ASSERT(buf != NULL);
	ASSERT(auth_result != NULL);
	ASSERT(auth_atc != NULL);

	comps = strsplit(buf, "&", true, &num_comps);
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
}

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
				fprintf(stderr, "Error querying authenticator "
				    "%s: %s\n", auth_url,
				    curl_easy_strerror(res));
			} else if (dl_info.bufsz == 0) {
				fprintf(stderr, "Error querying authenticator "
				    "%s: no data in response\n", auth_url);
			} else {
				fprintf(stderr, "Error querying authenticator "
				    "%s: HTTP error %ld\n", auth_url, code);
			}
		}
		ASSERT(sess->done_cb != NULL);
		sess->done_cb(auth_result, auth_atc, sess->userinfo);
	}
	avl_remove(&sessions, sess);
	cv_broadcast(&sess_shutdown_cv);

	free(dl_info.buf);
	curl_easy_cleanup(curl);
	curl_slist_free_all(hdrs);
	free(sess->postdata);
	free(sess);

	mutex_exit(&lock);
}

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

auth_sess_key_t
auth_sess_open(const cpdlc_msg_t *logon_msg, const void *addr,
    int addr_family, auth_done_cb_t done_cb, void *userinfo)
{
	auth_sess_t *sess;
	char *tmpstr;
	char addrbuf[64];
	size_t cap = 0;
	uint64_t key;
	/* temp curl context only used for URL-escaping purposes */
	CURL *curl;

	ASSERT(logon_msg != NULL);
	ASSERT(addr != NULL);
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
		const struct sockaddr_in *addr_v4 =
		    (const struct sockaddr_in *)addr;
		inet_ntop(addr_family, &addr_v4->sin_addr, addrbuf,
		    sizeof (addrbuf));
		append_format(&sess->postdata, &cap, "&REMOTEPORT=%d",
		    ntohs(addr_v4->sin_port));
	} else {
		char addrbuf[64];
		const struct sockaddr_in6 *addr_v6 =
		    (const struct sockaddr_in6 *)addr;
		inet_ntop(addr_family, &addr_v6->sin6_addr, addrbuf,
		    sizeof (addrbuf));
		append_format(&sess->postdata, &cap, "&REMOTEPORT=%d",
		    ntohs(addr_v6->sin6_port));
	}
	tmpstr = curl_easy_escape(curl, addrbuf, 0);
	append_format(&sess->postdata, &cap, "&REMOTEADDR=%s", tmpstr);
	curl_free(tmpstr);

	mutex_enter(&lock);
	key = sess->key = next_sess_key++;
	avl_add(&sessions, sess);
	VERIFY(thread_create(&sess->thread, auth_worker, sess));
	mutex_exit(&lock);
	/* Mustn't touch `sess' after this! done_cb might have fired now. */

	curl_easy_cleanup(curl);

	return (key);
}

void
auth_sess_kill(auth_sess_key_t key)
{
	auth_sess_t srch = { .key = key };
	auth_sess_t *sess;

	mutex_enter(&lock);
	sess = avl_find(&sessions, &srch, NULL);
	if (sess != NULL)
		sess->kill = true;
	mutex_exit(&lock);
}
