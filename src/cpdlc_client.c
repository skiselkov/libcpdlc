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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef	_WIN32
#include <windows.h>
#include <winsock2.h>
#else	/* !_WIN32 */
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#endif	/* !_WIN32 */

#ifdef	CPDLC_CLIENT_LWS
#include <libwebsockets.h>
#endif

#include "cpdlc_alloc.h"
#include "cpdlc_assert.h"
#include "cpdlc_client.h"
#include "cpdlc_string.h"
#include "cpdlc_thread.h"
#include "minilist.h"

#define	TLS_CHK(__op__) \
	do { \
		int error = (__op__); \
		if (error < GNUTLS_E_SUCCESS) { \
			fprintf(stderr, "GnuTLS error performing " #__op__ \
			    ": %s\n", gnutls_strerror(error)); \
			set_logon_failure(cl, "TLS setup error: %s", \
			    gnutls_strerror(error)); \
			cl->logon_status = CPDLC_LOGON_NONE; \
			return; \
		} \
	} while (0)

#define	WORKER_POLL_INTVAL	100	/* ms */
#define	READBUF_SZ		4096	/* bytes */
#define	DEFAULT_PORT_TCP	17622
#define	DEFAULT_PORT_LWS	17623

#define	CONNECTION_TIMEOUT	30	/* seconds */
#define	KEEPALIVE_TIMEOUT	300	/* seconds */

#ifdef	CPDLC_CLIENT_LWS
#define	SENDBUF_PRE_PAD		LWS_PRE
#else
#define	SENDBUF_PRE_PAD		0
#endif

#ifndef	PATH_MAX
#define	PATH_MAX		256	/* chars */
#endif

typedef struct outmsgbuf_s {
	cpdlc_msg_token_t	token;
	cpdlc_msg_status_t	status;
	char			*buf;
	size_t			bufsz;
	size_t			bytes_sent;
	bool			track_sent;
	list_node_t		node;
} outmsgbuf_t;

typedef struct inmsgbuf_s {
	cpdlc_msg_t		*msg;
	list_node_t		node;
} inmsgbuf_t;

/*
 * This object provides a generic CPDLC interfacing client. It is
 * responsible for maintaining the server connection, handling the
 * LOGON process and passing CPDLC messages over the wire. You
 * simply pass in CPDLC messages and the client takes care of
 * serializing them over the wire and sending them to the server.
 * You can also configure custom callbacks that is called when a
 * message is received from the server, or when a message has been
 * successfully sent to the server.
 *
 * This object doesn't provide facilities for tracking inter-
 * message relationships (such as message threads and formatting
 * of proper responses to requests). See cpdlc_msglist.h for that.
 */
struct cpdlc_client_s {
	/* immutable */
	bool				is_atc;

	mutex_t				lock;
	bool				bg_shutdown;

	/* protected by `lock' */
	char				host[PATH_MAX];
	int				port;
	char				*cafile;
	char				*key_file;
	char				*key_pass;
	gnutls_pkcs_encrypt_flags_t	key_enctype;
	char				*cert_file;
#ifndef	CPDLC_CLIENT_LWS
	/* LWS doesn't support in-memory keys */
	char				*key_pem_data;
	char				*cert_pem_data;
#endif	/* !CPDLC_CLIENT_LWS */

	thread_t			worker;
	bool				worker_started;
	cpdlc_logon_status_t		logon_status;
	char				logon_failure[128];
#ifdef	CPDLC_CLIENT_LWS
	struct lws_context		*lws_ctx;
	struct lws			*lws_sock;
	struct {
		cpdlc_msg_token_t	**out_tokens;
		unsigned		*num_out_tokens;
		bool			new_msgs;
	} pollinfo;
#else	/* !CPDLC_CLIENT_LWS */
	int				sock;
	time_t				conn_begin_time;
	struct addrinfo			*ai;
	struct addrinfo			*ai_cur;
	gnutls_session_t		session;
	bool				handshake_completed;
	gnutls_certificate_credentials_t xcred;
#endif	/* !CPDLC_CLIENT_LWS */

	cpdlc_msg_sent_cb_t		msg_sent_cb;
	cpdlc_msg_recv_cb_t		msg_recv_cb;
	void				*cb_userinfo;

	struct {
		/* protected by `lock' */
		cpdlc_msg_token_t	next_tok;
		list_t			sending;
		list_t			sent;
	} outmsgbufs;

	/* protected by `lock' */
	char		*inbuf;
	unsigned	inbuf_sz;
	list_t		inmsgbufs;

	/*
	 * When any last data was sent or received. Used for keepalive.
	 * Protected by `lock'.
	 */
	time_t		last_data_rdwr;

	struct {
		/* protected by `lock' */
		bool	do_logon;
		char	*data;
		char	*from;
		char	*to;
		char	*nda;
	} logon;
};

#ifdef	CPDLC_CLIENT_LWS
static void init_conn_lws(cpdlc_client_t *cl);
static bool poll_lws(cpdlc_client_t *cl, cpdlc_msg_token_t **out_tokens,
    unsigned *num_out_tokens);
static int cpdlc_lws_cb(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len);
#else	/* !CPDLC_CLIENT_LWS */
static bool resolve_host(cpdlc_client_t *cl);
static void init_conn(cpdlc_client_t *cl);
static void complete_conn(cpdlc_client_t *cl);
static void tls_handshake(cpdlc_client_t *cl);
static bool poll_for_msgs(cpdlc_client_t *cl,
    cpdlc_msg_token_t **out_tokens, unsigned *num_out_tokens);
#endif	/* !CPDLC_CLIENT_LWS */

static void reset_link_state(cpdlc_client_t *cl);

static void send_logon(cpdlc_client_t *cl);
static cpdlc_msg_token_t send_msg_impl(cpdlc_client_t *cl,
    const cpdlc_msg_t *msg, bool track_sent);

static void set_logon_failure(cpdlc_client_t *cl, const char *fmt, ...)
    PRINTF_ATTR(2);

#ifdef	CPDLC_CLIENT_LWS

static struct lws_protocols proto_list_lws[] = {
    {
	.name = "cpdlc",
	.callback = cpdlc_lws_cb,
	.rx_buffer_size = READBUF_SZ
    },
    { .name = NULL }	/* list terminator */
};

#endif	/* CPDLC_CLIENT_LWS */

static void
set_logon_failure(cpdlc_client_t *cl, const char *fmt, ...)
{
	if (fmt != NULL) {
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(cl->logon_failure, sizeof (cl->logon_failure),
		    fmt, ap);
		va_end(ap);
	} else {
		memset(cl->logon_failure, 0, sizeof (cl->logon_failure));
	}
}

static void
check_keepalive(cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);

	if (time(NULL) - cl->last_data_rdwr < KEEPALIVE_TIMEOUT)
		return;

	cpdlc_msg_t *ping = cpdlc_msg_alloc(CPDLC_PKT_PING);
	send_msg_impl(cl, ping, false);
	cpdlc_msg_free(ping);
	/* Reset the keepalive timer */
	cl->last_data_rdwr = time(NULL);
}

#ifndef	CPDLC_CLIENT_LWS

static bool
set_fd_nonblock(int fd)
{
	int flags;
	return ((flags = fcntl(fd, F_GETFL)) >= 0 &&
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0);
}

#endif	/* !CPDLC_CLIENT_LWS */

static void
logon_worker(void *userinfo)
{
	cpdlc_client_t *cl = userinfo;

	ASSERT(cl != NULL);

	mutex_enter(&cl->lock);

#ifdef	CPDLC_CLIENT_LWS
	init_conn_lws(cl);
#else	/* !CPDLC_CLIENT_LWS */
	if (resolve_host(cl))
		init_conn(cl);
#endif	/* !CPDLC_CLIENT_LWS */

	while (cl->logon_status != CPDLC_LOGON_NONE &&
	    cl->logon_failure[0] == '\0') {
		bool new_msgs = false;
		cpdlc_msg_token_t *out_tokens = NULL;
		unsigned num_out_tokens = 0;

		switch (cl->logon_status) {
		case CPDLC_LOGON_CONNECTING_LINK:
#ifdef	CPDLC_CLIENT_LWS
			ASSERT(cl->lws_sock != NULL);
			new_msgs = poll_lws(cl, &out_tokens, &num_out_tokens);
#else	/* !CPDLC_CLIENT_LWS */
			ASSERT(cl->sock != -1);
			complete_conn(cl);
#endif	/* !CPDLC_CLIENT_LWS */
			break;
		case CPDLC_LOGON_HANDSHAKING_LINK:
#ifndef	CPDLC_CLIENT_LWS
			ASSERT(cl->sock != -1);
			tls_handshake(cl);
#endif	/* !CPDLC_CLIENT_LWS */
			break;
		case CPDLC_LOGON_LINK_AVAIL:
			if (cl->logon.do_logon) {
				send_logon(cl);
			} else {
#ifdef	CPDLC_CLIENT_LWS
				new_msgs = poll_lws(cl, &out_tokens,
				    &num_out_tokens);
#else	/* !CPDLC_CLIENT_LWS */
				new_msgs = poll_for_msgs(cl, &out_tokens,
				    &num_out_tokens);
#endif	/* !CPDLC_CLIENT_LWS */
			}
			break;
		case CPDLC_LOGON_IN_PROG:
		case CPDLC_LOGON_COMPLETE:
#ifdef	CPDLC_CLIENT_LWS
			new_msgs = poll_lws(cl, &out_tokens, &num_out_tokens);
#else	/* !CPDLC_CLIENT_LWS */
			new_msgs = poll_for_msgs(cl, &out_tokens,
			    &num_out_tokens);
#endif	/* !CPDLC_CLIENT_LWS */
			break;
		default:
			VERIFY_MSG(0, "client reached impossible "
			    "logon_status = %x", cl->logon_status);
		}
		/* Schedules a keepalive message if necessary */
		if (cl->logon_status == CPDLC_LOGON_COMPLETE)
			check_keepalive(cl);

		if (new_msgs && cl->msg_recv_cb != NULL) {
			/*
			 * To prevent locking inversions, we need to drop
			 * our lock here.
			 */
			cpdlc_msg_recv_cb_t cb = cl->msg_recv_cb;
			mutex_exit(&cl->lock);
			cb(cl);
			mutex_enter(&cl->lock);
		}
		if (num_out_tokens != 0) {
			cpdlc_msg_sent_cb_t cb = cl->msg_sent_cb;

			if (cb != NULL) {
				mutex_exit(&cl->lock);
				cb(cl, out_tokens, num_out_tokens);
				mutex_enter(&cl->lock);
			}
			free(out_tokens);
		}
	}
	reset_link_state(cl);
	cl->worker_started = false;

	mutex_exit(&cl->lock);
}

cpdlc_client_t *
cpdlc_client_alloc(bool is_atc)
{
	cpdlc_client_t *cl = safe_calloc(1, sizeof (*cl));

	mutex_init(&cl->lock);

	cl->is_atc = is_atc;

#ifndef	CPDLC_CLIENT_LWS
	cl->sock = -1;
#endif

	list_create(&cl->outmsgbufs.sending, sizeof (outmsgbuf_t),
	    offsetof(outmsgbuf_t, node));
	list_create(&cl->outmsgbufs.sent, sizeof (outmsgbuf_t),
	    offsetof(outmsgbuf_t, node));
	list_create(&cl->inmsgbufs, sizeof (inmsgbuf_t),
	    offsetof(inmsgbuf_t, node));

	return (cl);
}

static void
clear_key_data(cpdlc_client_t *cl)
{
	secure_free(cl->key_file);
	secure_free(cl->key_pass);
	secure_free(cl->cert_file);

	cl->key_file = NULL;
	cl->key_pass = NULL;
	cl->key_enctype = GNUTLS_PKCS_PLAIN;
	cl->cert_file = NULL;

#ifndef	CPDLC_CLIENT_LWS
	secure_free(cl->key_pem_data);
	secure_free(cl->cert_pem_data);
	cl->key_pem_data = NULL;
	cl->cert_pem_data = NULL;
#endif	/* !CPDLC_CLIENT_LWS */
}

void
cpdlc_client_free(cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);

	mutex_enter(&cl->lock);
	if (cl->worker_started) {
		cl->logon_status = CPDLC_LOGON_NONE;
		mutex_exit(&cl->lock);
		thread_join(&cl->worker);
	} else {
		mutex_exit(&cl->lock);
	}

	ASSERT3P(cl->inbuf, ==, NULL);
	ASSERT0(cl->inbuf_sz);

	list_destroy(&cl->outmsgbufs.sending);
	list_destroy(&cl->outmsgbufs.sent);
	list_destroy(&cl->inmsgbufs);

	free(cl->cafile);
	clear_key_data(cl);

#ifndef	CPDLC_CLIENT_LWS
	if (cl->ai != NULL)
		freeaddrinfo(cl->ai);
#endif	/* !CPDLC_CLIENT_LWS */

	mutex_destroy(&cl->lock);

	free(cl->logon.data);
	free(cl->logon.from);
	free(cl->logon.to);
	free(cl->logon.nda);
	memset(cl, 0, sizeof (*cl));
	free(cl);
}

void
cpdlc_client_set_host(cpdlc_client_t *cl, const char *host)
{
	ASSERT(cl != NULL);
	mutex_enter(&cl->lock);
	if (host != NULL)
		cpdlc_strlcpy(cl->host, host, sizeof (cl->host));
	else
		memset(cl->host, 0, sizeof (cl->host));
	mutex_exit(&cl->lock);
}

const char *
cpdlc_client_get_host(cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);
	return (cl->host);
}

void
cpdlc_client_set_port(cpdlc_client_t *cl, unsigned port)
{
	ASSERT(cl != NULL);
	mutex_enter(&cl->lock);
	cl->port = port;
	mutex_exit(&cl->lock);
}

unsigned
cpdlc_client_get_port(cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);
	return (cl->port);
}

void
cpdlc_client_set_ca_file(cpdlc_client_t *cl, const char *cafile)
{
	ASSERT(cl != NULL);
	mutex_enter(&cl->lock);
	free(cl->cafile);
	cl->cafile = NULL;
	if (cafile != NULL)
		cl->cafile = strdup(cafile);
	mutex_exit(&cl->lock);
}

const char *
cpdlc_client_get_ca_file(cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);
	return (cl->cafile);
}

void
cpdlc_client_set_key_file(cpdlc_client_t *cl, const char *key_file,
    const char *key_pass, gnutls_pkcs_encrypt_flags_t key_enctype,
    const char *cert_file)
{
	ASSERT(cl != NULL);
	ASSERT(key_file != NULL || cert_file == NULL);
	ASSERT(key_pass != NULL || key_enctype == GNUTLS_PKCS_PLAIN);

	mutex_enter(&cl->lock);

	clear_key_data(cl);

	if (key_file != NULL)
		cl->key_file = secure_strdup(key_file);
	if (cert_file != NULL)
		cl->cert_file = secure_strdup(cert_file);
	if (key_pass != NULL)
		cl->key_pass = secure_strdup(key_pass);
	cl->key_enctype = key_enctype;

	mutex_exit(&cl->lock);
}

#ifndef	CPDLC_CLIENT_LWS
void
cpdlc_client_set_key_mem(cpdlc_client_t *cl, const char *key_pem_data,
    const char *key_pass, gnutls_pkcs_encrypt_flags_t key_enctype,
    const char *cert_pem_data)
{
	ASSERT(cl != NULL);
	ASSERT(key_pem_data != NULL || cert_pem_data == NULL);
	ASSERT(key_pass != NULL || key_enctype == GNUTLS_PKCS_PLAIN);

	mutex_enter(&cl->lock);

	clear_key_data(cl);

	if (key_pem_data != NULL)
		cl->key_pem_data = secure_strdup(key_pem_data);
	if (cert_pem_data != NULL)
		cl->cert_pem_data = secure_strdup(cert_pem_data);
	if (key_pass != NULL)
		cl->key_pass = secure_strdup(key_pass);
	cl->key_enctype = key_enctype;

	mutex_exit(&cl->lock);
}
#endif	/* CPDLC_CLIENT_LWS */

static void
reset_link_state(cpdlc_client_t *cl)
{
	inmsgbuf_t *inbuf;
	outmsgbuf_t *outbuf;

	ASSERT(cl != NULL);
	cl->logon_status = CPDLC_LOGON_NONE;
	free(cl->logon.nda);
	cl->logon.nda = NULL;
	free(cl->logon.to);
	cl->logon.to = NULL;

#ifdef	CPDLC_CLIENT_LWS
	if (cl->lws_ctx != NULL) {
		lws_context_destroy(cl->lws_ctx);
		cl->lws_ctx = NULL;
		cl->lws_sock = NULL;
	}
#else	/* !CPDLC_CLIENT_LWS */
	if (cl->session != NULL) {
		if (cl->handshake_completed) {
			gnutls_bye(cl->session, GNUTLS_SHUT_RDWR);
			cl->handshake_completed = false;
		}
		gnutls_deinit(cl->session);
		cl->session = NULL;
	} else {
		ASSERT0(cl->handshake_completed);
	}
	if (cl->xcred != NULL) {
		gnutls_certificate_free_credentials(cl->xcred);
		cl->xcred = NULL;
	}
	if (cl->sock != -1) {
		shutdown(cl->sock, SHUT_RDWR);
		cl->sock = -1;
	}
#endif	/* !CPDLC_CLIENT_LWS */

	while ((outbuf = list_remove_head(&cl->outmsgbufs.sending)) != NULL) {
		free(outbuf->buf);
		free(outbuf);
	}
	while ((outbuf = list_remove_head(&cl->outmsgbufs.sent)) != NULL) {
		ASSERT3P(outbuf->buf, ==, NULL);
		free(outbuf);
	}

	free(cl->inbuf);
	cl->inbuf = NULL;
	cl->inbuf_sz = 0;

	while ((inbuf = list_remove_head(&cl->inmsgbufs)) != NULL) {
		cpdlc_msg_free(inbuf->msg);
		free(inbuf);
	}
}

#ifdef	CPDLC_CLIENT_LWS

static void
init_conn_lws(cpdlc_client_t *cl)
{
	struct lws_context_creation_info info;
	struct lws_client_connect_info ccinfo;

	ASSERT(cl != NULL);

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = proto_list_lws;
	info.gid = -1;
	info.uid = -1;
	info.client_ssl_ca_filepath = cl->cafile;
	info.client_ssl_private_key_filepath = cl->key_file;
	info.client_ssl_private_key_password = cl->key_pass;
	info.client_ssl_cert_filepath = cl->cert_file;
	info.vhost_name = cl->host;
	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

	cl->lws_ctx = lws_create_context(&info);
	memset(&info, 0, sizeof (info));
	if (cl->lws_ctx == NULL) {
		fprintf(stderr, "Failed to create lws_ctx\n");
		return;
	}

	ccinfo.address = cl->host;
	ccinfo.port = (cl->port != 0 ? cl->port : DEFAULT_PORT_LWS);
	ccinfo.path = "/";
	ccinfo.host = cl->host;
	ccinfo.origin = cl->logon.from;
	ccinfo.protocol = "cpdlc";
	ccinfo.context = cl->lws_ctx;
	ccinfo.userdata = cl;
	ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED;
	cl->lws_sock = lws_client_connect_via_info(&ccinfo);
	memset(&ccinfo, 0, sizeof (ccinfo));
	if (cl->lws_sock == NULL) {
		fprintf(stderr, "Failed to create lws_sock\n");
		return;
	}

	cl->logon_status = CPDLC_LOGON_CONNECTING_LINK;
}

#else	/* !CPDLC_CLIENT_LWS */

static bool
resolve_host(cpdlc_client_t *cl)
{
	struct addrinfo *ai = NULL;
	int result;
	char portbuf[8];
	struct addrinfo hints = {
	    .ai_family = AF_UNSPEC,
	    .ai_socktype = SOCK_STREAM,
	    .ai_protocol = IPPROTO_TCP
	};
	int port = (cl->port != 0 ? cl->port : DEFAULT_PORT_TCP);
	char host[PATH_MAX];

	ASSERT(cl != NULL);
	ASSERT3S(cl->sock, ==, -1);
	ASSERT3U(cl->logon_status, ==, CPDLC_LOGON_NONE);

	if (cl->ai != NULL) {
		freeaddrinfo(cl->ai);
		cl->ai = cl->ai_cur = NULL;
	}
	if (cl->host[0] == '\0') {
		set_logon_failure(cl, "no host specified");
		return (false);
	}
	cpdlc_strlcpy(host, cl->host, sizeof (host));
	snprintf(portbuf, sizeof (portbuf), "%d", port);

	/*
	 * getaddrinfo can block for network traffic, so we need to drop
	 * the lock here to avoid blocking API callers.
	 */
	mutex_exit(&cl->lock);
	result = getaddrinfo(host, portbuf, &hints, &ai);
	mutex_enter(&cl->lock);

	if (result != 0) {
		fprintf(stderr, "Can't resolve %s: %s\n", host,
		    gai_strerror(result));
		set_logon_failure(cl, "%s: %s", host, gai_strerror(result));
		return (false);
	}
	ASSERT(ai != NULL);
	cl->ai_cur = cl->ai = ai;

	return (true);
}

static void
init_conn(cpdlc_client_t *cl)
{
	struct addrinfo *ai;
	int sock;
	cpdlc_logon_status_t new_status;

	ASSERT(cl != NULL);
	ASSERT3S(cl->sock, ==, -1);
	ASSERT3U(cl->logon_status, ==, CPDLC_LOGON_NONE);
	ASSERT(cl->ai != NULL);

	/* No more addresses to try */
	if (cl->ai_cur == NULL)
		return;

	ai = cl->ai_cur;
	cl->ai_cur = cl->ai_cur->ai_next;

	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sock == -1) {
		set_logon_failure(cl, "%s", strerror(errno));
		/* Try the next address in line, if one is available */
		init_conn(cl);
		return;
	}
	if (!set_fd_nonblock(sock)) {
		close(sock);
		set_logon_failure(cl, "%s", strerror(errno));
		init_conn(cl);
		return;
	}
	/*
	 * No need to drop locks here, we are non-blocking, so connection
	 * attempt will continue async.
	 */
	if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
		if (errno != EINPROGRESS) {
			close(sock);
			set_logon_failure(cl, "%s", strerror(errno));
			init_conn(cl);
			return;
		}
		new_status = CPDLC_LOGON_CONNECTING_LINK;
		cl->conn_begin_time = time(NULL);
	} else {
		new_status = CPDLC_LOGON_HANDSHAKING_LINK;
	}

	cl->sock = sock;
	cl->logon_status = new_status;
	set_logon_failure(cl, NULL);
}

static void
complete_conn(cpdlc_client_t *cl)
{
	struct pollfd pfd = { .events = POLLOUT };
	int so_error, res;
	socklen_t so_error_len = sizeof (so_error);

	ASSERT(cl != NULL);
	ASSERT3S(cl->logon_status, ==, CPDLC_LOGON_CONNECTING_LINK);

	pfd.fd = cl->sock;

	mutex_exit(&cl->lock);
	res = poll(&pfd, 1, WORKER_POLL_INTVAL);
	mutex_enter(&cl->lock);

	switch (res) {
	case 0:
		/* Still in progress, check timeout */
		if (time(NULL) - cl->conn_begin_time > CONNECTION_TIMEOUT) {
			set_logon_failure(cl, "Connection timeout");
			goto errout;
		}
		return;
	case 1:
		/* Connection attempt completed. Let's see about the result. */
		if (getsockopt(cl->sock, SOL_SOCKET, SO_ERROR, &so_error,
		    &so_error_len) < 0) {
			set_logon_failure(cl, "%s", strerror(errno));
			goto errout;
		}
		if (so_error != 0) {
			set_logon_failure(cl, "%s", strerror(so_error));
			goto errout;
		}
		/* Success! */
		cl->logon_status = CPDLC_LOGON_HANDSHAKING_LINK;
		return;
	default:
		set_logon_failure(cl, "%s", strerror(errno));
		goto errout;
	}
errout:
	cl->logon_status = CPDLC_LOGON_NONE;
	close(cl->sock);
	cl->sock = -1;
	/* Try the next socket in line, if one is available */
	init_conn(cl);
}

/*
 * Initiates or completes the TLS handshake procedure.
 */
static void
tls_handshake(cpdlc_client_t *cl)
{
	int ret;

	ASSERT(cl != NULL);
	ASSERT3U(cl->logon_status, ==, CPDLC_LOGON_HANDSHAKING_LINK);

	if (cl->session == NULL) {
		TLS_CHK(gnutls_certificate_allocate_credentials(&cl->xcred));
		if (cl->key_file != NULL) {
			ASSERT(cl->cert_file != NULL);
			TLS_CHK(gnutls_certificate_set_x509_key_file2(cl->xcred,
			    cl->cert_file, cl->key_file, GNUTLS_X509_FMT_PEM,
			    cl->key_pass, cl->key_enctype));
		} else if (cl->key_pem_data != NULL) {
			gnutls_datum_t key_datum = {
			    .data = (unsigned char*)cl->key_pem_data,
			    .size = strlen(cl->key_pem_data)
			};
			gnutls_datum_t cert_datum = {
			    .data = (unsigned char*)cl->cert_pem_data,
			    .size = strlen(cl->cert_pem_data)
			};
			TLS_CHK(gnutls_certificate_set_x509_key_mem2(cl->xcred,
			    &cert_datum, &key_datum, GNUTLS_X509_FMT_PEM,
			    cl->key_pass, cl->key_enctype));
			memset(&key_datum, 0, sizeof (key_datum));
			memset(&cert_datum, 0, sizeof (cert_datum));
		}
		if (cl->cafile != NULL) {
			TLS_CHK(gnutls_certificate_set_x509_trust_file(
			    cl->xcred, cl->cafile, GNUTLS_X509_FMT_PEM));
		} else {
			TLS_CHK(gnutls_certificate_set_x509_system_trust(
			    cl->xcred));
		}
		TLS_CHK(gnutls_init(&cl->session, GNUTLS_CLIENT));
		TLS_CHK(gnutls_server_name_set(cl->session, GNUTLS_NAME_DNS,
		    cl->host, strlen(cl->host)));
		TLS_CHK(gnutls_set_default_priority(cl->session));
		TLS_CHK(gnutls_credentials_set(cl->session,
		    GNUTLS_CRD_CERTIFICATE, cl->xcred));
		gnutls_session_set_verify_cert(cl->session, cl->host, 0);
		ASSERT(cl->sock != -1);
		gnutls_transport_set_int(cl->session, cl->sock);
		gnutls_handshake_set_timeout(cl->session,
		    GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
	}

	/*
	 * gnutls_handshake can block for network traffic, so drop the
	 * client lock here temporarily.
	 */
	mutex_exit(&cl->lock);
	ret = gnutls_handshake(cl->session);
	mutex_enter(&cl->lock);
	if (ret < GNUTLS_E_SUCCESS) {
		if (ret != GNUTLS_E_AGAIN) {
			set_logon_failure(cl, "TLS handshake error: %s",
			    gnutls_strerror(ret));
			cl->logon_status = CPDLC_LOGON_NONE;
		}
		return;
	} else {
		cl->handshake_completed = true;
	}
	cl->logon_status = CPDLC_LOGON_LINK_AVAIL;
}

#endif	/* !CPDLC_CLIENT_LWS */

static void
send_logon(cpdlc_client_t *cl)
{
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);

	ASSERT(cl != NULL);
	ASSERT(cl->logon.data != NULL);
	ASSERT(cl->logon.from != NULL);
	ASSERT(cl->logon.to != NULL);

	cpdlc_msg_set_logon_data(msg, cl->logon.data);
	cpdlc_msg_set_from(msg, cl->logon.from);
	if (cl->logon.nda != NULL) {
		cl->logon.to = cl->logon.nda;
		cl->logon.nda = NULL;
	}
	if (cl->logon.to != NULL)
		cpdlc_msg_set_to(msg, cl->logon.to);
	send_msg_impl(cl, msg, false);
	cpdlc_msg_free(msg);

	cl->logon_status = CPDLC_LOGON_IN_PROG;
}

static void
handle_end_svc(cpdlc_client_t *cl)
{
	/*
	 * If we have an NDA loaded, we can try recycling the link
	 * and logging onto the NDA. Otherwise, we tear down the
	 * link. This message IS passed on to the uppper layers, as
	 * it needs to be displayed to the flight crew.
	 */
	if (cl->logon.nda != NULL)
		cl->logon_status = CPDLC_LOGON_LINK_AVAIL;
	else
		cl->logon_status = CPDLC_LOGON_NONE;
}

static void
handle_nda(cpdlc_client_t *cl, cpdlc_msg_t *msg, unsigned i)
{
	char nda[128], name[128];

	ASSERT(cl->logon.to != NULL);

	free(cl->logon.nda);
	cl->logon.nda = NULL;

	cpdlc_msg_seg_get_arg(msg, i, 0, nda, sizeof (nda), name);
	if (strcmp(nda, cl->logon.to) != 0)
		cl->logon.nda = strdup(nda);
	cpdlc_msg_del_seg(msg, i);
}

static bool
queue_incoming_msg(cpdlc_client_t *cl, cpdlc_msg_t *msg)
{
	inmsgbuf_t *inmsgbuf;

	ASSERT(cl != NULL);
	ASSERT3U(cl->logon_status, ==, CPDLC_LOGON_COMPLETE);
	ASSERT(msg != NULL);

	if (!cl->is_atc && strcmp(cpdlc_msg_get_from(msg), cl->logon.to) != 0) {
		cpdlc_msg_t *errmsg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);

		cpdlc_msg_set_mrn(errmsg, cpdlc_msg_get_min(msg));
		cpdlc_msg_set_to(errmsg, cpdlc_msg_get_from(msg));
		cpdlc_msg_add_seg(errmsg, true,
		    CPDLC_DM63_NOT_CURRENT_DATA_AUTHORITY, 0);
		send_msg_impl(cl, errmsg, false);

		cpdlc_msg_free(msg);
		cpdlc_msg_free(errmsg);

		return (false);
	}

	if (!cl->is_atc) {
		for (unsigned i = 0; i < msg->num_segs;) {
			ASSERT(msg->segs[i].info != NULL);
			if (msg->segs[i].info->msg_type ==
			    CPDLC_UM161_END_SVC) {
				handle_end_svc(cl);
				cpdlc_msg_free(msg);
				return (false);
			}
			if (msg->segs[i].info->msg_type ==
			    CPDLC_UM160_NEXT_DATA_AUTHORITY_id) {
				/* this deletes the segment, so don't incr i */
				handle_nda(cl, msg, i);
				continue;
			}
			i++;
		}
	}
	if (cpdlc_msg_get_num_segs(msg) == 0) {
		cpdlc_msg_free(msg);
		return (false);
	}

	inmsgbuf = safe_calloc(1, sizeof (*inmsgbuf));
	inmsgbuf->msg = msg;
	list_insert_tail(&cl->inmsgbufs, inmsgbuf);

	return (true);
}

static bool
process_msg(cpdlc_client_t *cl, cpdlc_msg_t *msg)
{
	bool new_msgs = false;

	ASSERT(cl != NULL);
	ASSERT(msg != NULL);

	if (msg->pkt_type != CPDLC_PKT_CPDLC) {
		/* Discard ping/pong messages */
		cpdlc_msg_free(msg);
		return (false);
	}

	switch (cl->logon_status) {
	case CPDLC_LOGON_LINK_AVAIL:
		/* Discard any incoming message traffic in this state. */
		cpdlc_msg_free(msg);
		break;
	case CPDLC_LOGON_IN_PROG:
		if (msg->is_logon) {
			const char *logon_data = cpdlc_msg_get_logon_data(msg);

			if (strcmp(logon_data, "SUCCESS") == 0) {
				cl->logon_status = CPDLC_LOGON_COMPLETE;
				cl->last_data_rdwr = time(NULL);
				set_logon_failure(cl, NULL);
			} else {
				cl->logon_status = CPDLC_LOGON_LINK_AVAIL;
				set_logon_failure(cl, "Logon denied");
			}
			cpdlc_msg_free(msg);
		} else {
			/* Discard non-logon messages in this state */
			cpdlc_msg_free(msg);
		}
		break;
	case CPDLC_LOGON_COMPLETE:
		if (msg->is_logon) {
			/* Spurious logon message? Why? */
			cpdlc_msg_free(msg);
		} else {
			new_msgs |= queue_incoming_msg(cl, msg);
		}
		break;
	default:
		VERIFY_MSG(0, "Invalid client state %x", cl->logon_status);
	}

	return (new_msgs);
}

static bool
process_input(cpdlc_client_t *cl)
{
	bool new_msgs = false;
	size_t consumed_total = 0;

	ASSERT(cl != NULL);
	ASSERT(cl->inbuf != NULL);
	ASSERT(cl->inbuf_sz != 0);

	while (consumed_total < cl->inbuf_sz) {
		int consumed;
		cpdlc_msg_t *msg;
		char error[sizeof (cl->logon_failure)];

		/* Try to decode a message from our accumulated input. */
		ASSERT3S(consumed_total, <=, cl->inbuf_sz);
		if (!cpdlc_msg_decode(&cl->inbuf[consumed_total], &msg,
		    &consumed, error, sizeof (error))) {
			cl->logon_status = CPDLC_LOGON_NONE;
			cpdlc_strlcpy(cl->logon_failure, error,
			    sizeof (cl->logon_failure));
			break;
		}
		/* No more complete messages pending? */
		if (msg == NULL)
			break;
		ASSERT(consumed != 0);
		new_msgs |= process_msg(cl, msg);
		/* Do not free the message, `process_msg' consumes it */
		consumed_total += consumed;
		ASSERT3S(consumed_total, <=, cl->inbuf_sz);
	}
	if (consumed_total != 0) {
		ASSERT3S(consumed_total, <=, cl->inbuf_sz);
		cl->inbuf_sz -= consumed_total;
		memmove(cl->inbuf, &cl->inbuf[consumed_total],
		    cl->inbuf_sz + 1);
		cl->inbuf = realloc(cl->inbuf, cl->inbuf_sz);
	}

	return (new_msgs);
}

static bool
sanitize_input(const uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uint8_t c = buf[i];
		/* Input sanitization, don't allow control chars */
		if ((c < 32 || c > 127) && c != '\n' && c != '\r' && c != '\t')
			return (false);
	}
	return (true);
}

#ifdef	CPDLC_CLIENT_LWS

static bool
do_msg_input_lws(cpdlc_client_t *cl, const uint8_t *buf, size_t len)
{
	ASSERT(cl != NULL);
	ASSERT(buf != NULL);
	ASSERT(len != 0);

	cl->inbuf = realloc(cl->inbuf, cl->inbuf_sz + len + 1);
	cpdlc_strlcpy(&cl->inbuf[cl->inbuf_sz], (const char *)buf, len + 1);
	cl->inbuf_sz += len;
	/* Reset the keepalive timer */
	cl->last_data_rdwr = time(NULL);

	return (process_input(cl));
}

#else	/* !CPDLC_CLIENT_LWS */

static bool
do_msg_input(cpdlc_client_t *cl)
{
	bool new_msgs = false;

	ASSERT(cl != NULL);

	for (;;) {
		uint8_t buf[READBUF_SZ];
		int bytes = gnutls_record_recv(cl->session, buf, sizeof (buf));

		if (bytes < 0) {
			if (bytes != GNUTLS_E_AGAIN &&
			    gnutls_error_is_fatal(bytes)) {
				cl->logon_status = CPDLC_LOGON_NONE;
			}
			break;
		}
		if (bytes == 0) {
			/* Remote end closed our connection */
			cl->logon_status = CPDLC_LOGON_NONE;
			break;
		}
		/* Input sanitization, don't allow control chars */
		if (!sanitize_input(buf, bytes)) {
			cl->logon_status = CPDLC_LOGON_NONE;
			break;
		}
		cl->inbuf = realloc(cl->inbuf, cl->inbuf_sz + bytes + 1);
		cpdlc_strlcpy(&cl->inbuf[cl->inbuf_sz], (const char *)buf,
		    bytes + 1);
		cl->inbuf_sz += bytes;
		/* Reset the keepalive timer */
		cl->last_data_rdwr = time(NULL);

		new_msgs |= process_input(cl);
	}

	return (new_msgs);
}

#endif	/* !CPDLC_CLIENT_LWS */

#ifdef	CPDLC_CLIENT_LWS
static cpdlc_msg_token_t *
do_msg_output(cpdlc_client_t *cl, struct lws *wsi, unsigned *num_tokens_p)
#else	/* !CPDLC_CLIENT_LWS */
static cpdlc_msg_token_t *
do_msg_output(cpdlc_client_t *cl, unsigned *num_tokens_p)
#endif	/* !CPDLC_CLIENT_LWS */
{
	unsigned num_tokens = 0;
	cpdlc_msg_token_t *tokens = NULL;

	ASSERT(cl != NULL);
	ASSERT(num_tokens_p != NULL);

	for (outmsgbuf_t *outmsgbuf = list_head(&cl->outmsgbufs.sending);
	    outmsgbuf != NULL; outmsgbuf = list_head(&cl->outmsgbufs.sending)) {
		int bytes;

#ifdef	CPDLC_CLIENT_LWS
		bytes = lws_write(wsi, (void *)&outmsgbuf->buf[SENDBUF_PRE_PAD],
		    outmsgbuf->bufsz, LWS_WRITE_TEXT);
		if (bytes == -1) {
			/* Fatal send error */
			cl->logon_status = CPDLC_LOGON_NONE;
			break;
		}
		if (bytes == 0) {
			lws_callback_on_writable(wsi);
			break;
		}
		/*
		 * LWS should buffer any unsent data internally, so we check
		 * that we absolutely super-duper have sent everything.
		 */
		ASSERT3S(bytes, ==, outmsgbuf->bufsz);
#else	/* !CPDLC_CLIENT_LWS */
		bytes = gnutls_record_send(cl->session,
		    &outmsgbuf->buf[outmsgbuf->bytes_sent],
		    outmsgbuf->bufsz - outmsgbuf->bytes_sent);

		if (bytes < 0) {
			if (bytes != GNUTLS_E_AGAIN &&
			    gnutls_error_is_fatal(bytes)) {
				cl->logon_status = CPDLC_LOGON_NONE;
			}
			break;
		}
		if (bytes == 0) {
			/* Remote end closed our connection */
			cl->logon_status = CPDLC_LOGON_NONE;
			break;
		}
#endif	/* !CPDLC_CLIENT_LWS */
		outmsgbuf->bytes_sent += bytes;
		ASSERT3S(outmsgbuf->bytes_sent, <=, outmsgbuf->bufsz);
		/* Reset the keepalive timer */
		cl->last_data_rdwr = time(NULL);
		/* short byte count sent, need to wait for more writing */
		if (outmsgbuf->bytes_sent < outmsgbuf->bufsz)
			break;
		list_remove(&cl->outmsgbufs.sending, outmsgbuf);
		/* Don't need the buffer inside anymore */
		free(outmsgbuf->buf);
		outmsgbuf->buf = NULL;
		outmsgbuf->bufsz = 0;
		if (outmsgbuf->track_sent) {
			list_insert_tail(&cl->outmsgbufs.sent, outmsgbuf);
			tokens = safe_realloc(tokens, (num_tokens + 1) *
			    sizeof (*tokens));
			tokens[num_tokens] = outmsgbuf->token;
		} else {
			free(outmsgbuf);
		}
	}

	*num_tokens_p = num_tokens;
	return (tokens);
}

#ifdef	CPDLC_CLIENT_LWS

static bool
poll_lws(cpdlc_client_t *cl, cpdlc_msg_token_t **out_tokens,
    unsigned *num_out_tokens)
{
	ASSERT(cl != NULL);
	ASSERT(out_tokens != NULL);
	ASSERT(num_out_tokens != NULL);

	cl->pollinfo.out_tokens = out_tokens;
	cl->pollinfo.num_out_tokens = num_out_tokens;
	cl->pollinfo.new_msgs = false;

	if (list_count(&cl->outmsgbufs.sending) != 0)
		lws_callback_on_writable(cl->lws_sock);

	mutex_exit(&cl->lock);
	lws_service(cl->lws_ctx, WORKER_POLL_INTVAL);
	mutex_enter(&cl->lock);

	return (cl->pollinfo.new_msgs);
}

static int
cpdlc_lws_cb(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len)
{
	cpdlc_client_t *cl;

	ASSERT(wsi != NULL);

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		cl = user;
		ASSERT(cl != NULL);
		mutex_enter(&cl->lock);
		cl->logon_status = CPDLC_LOGON_LINK_AVAIL;
		mutex_exit(&cl->lock);
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE:
		cl = user;
		ASSERT(cl != NULL);
		ASSERT(in != NULL);
		ASSERT(len != 0);

		mutex_enter(&cl->lock);
		if (!sanitize_input(in, len)) {
			cl->logon_status = CPDLC_LOGON_NONE;
			cpdlc_strlcpy(cl->logon_failure, "Bad data on link",
			    sizeof (cl->logon_failure));
		}
		if (cl->logon_status != CPDLC_LOGON_NONE)
			cl->pollinfo.new_msgs |= do_msg_input_lws(cl, in, len);
		mutex_exit(&cl->lock);
		break;
	case LWS_CALLBACK_CLIENT_WRITEABLE:
		cl = user;
		ASSERT(cl != NULL);
		ASSERT(cl->pollinfo.out_tokens != NULL);

		mutex_enter(&cl->lock);
		if (cl->logon_status != CPDLC_LOGON_NONE) {
			*cl->pollinfo.out_tokens = do_msg_output(cl, wsi,
			    cl->pollinfo.num_out_tokens);
		}
		mutex_exit(&cl->lock);
		break;
	case LWS_CALLBACK_CLOSED:
	case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
	case LWS_CALLBACK_WSI_DESTROY:
		cl = user;
		ASSERT(cl != NULL);

		mutex_enter(&cl->lock);
		cl->logon_status = CPDLC_LOGON_NONE;
		mutex_exit(&cl->lock);
		break;
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		cl = user;
		ASSERT(cl != NULL);

		mutex_enter(&cl->lock);
		cl->logon_status = CPDLC_LOGON_NONE;
		cpdlc_strlcpy(cl->logon_failure, "Connection error",
		    sizeof (cl->logon_failure));
		mutex_exit(&cl->lock);
		break;
	default:
		break;
	}

	return (0);
}

#else	/* !CPDLC_CLIENT_LWS */

static bool
poll_for_msgs(cpdlc_client_t *cl, cpdlc_msg_token_t **out_tokens,
    unsigned *num_out_tokens)
{
	struct pollfd pfd;
	int ret;
	bool new_msgs = false;

	ASSERT(cl != NULL);
	ASSERT3U(cl->logon_status, >=, CPDLC_LOGON_LINK_AVAIL);
	ASSERT(cl->session != NULL);
	ASSERT(cl->sock != -1);
	ASSERT(out_tokens != NULL);
	ASSERT(num_out_tokens != NULL);

	pfd.fd = cl->sock;
	pfd.events = POLLIN;
	if (list_head(&cl->outmsgbufs.sending) != NULL)
		pfd.events |= POLLOUT;

	/* Release the lock to allow for state updates while running */
	mutex_exit(&cl->lock);
	ret = poll(&pfd, 1, WORKER_POLL_INTVAL);
	mutex_enter(&cl->lock);
	/* Poll error or external shutdown request */
	if (ret < 0 || cl->logon_status == CPDLC_LOGON_NONE) {
		cl->logon_status = CPDLC_LOGON_NONE;
		return (false);
	}

	if (pfd.revents & POLLIN)
		new_msgs = do_msg_input(cl);
	/* In case input killed the connection, recheck logon_status */
	if (cl->logon_status != CPDLC_LOGON_NONE && (pfd.revents & POLLOUT))
		*out_tokens = do_msg_output(cl, num_out_tokens);

	return (new_msgs);
}

#endif	/* !CPDLC_CLIENT_LWS */

size_t
cpdlc_client_get_cda(cpdlc_client_t *cl, char *buf, size_t cap)
{
	size_t n = 0;

	ASSERT(cl != NULL);

	mutex_enter(&cl->lock);
	if (cl->logon.to != NULL) {
		n = strlen(cl->logon.to);
		cpdlc_strlcpy(buf, cl->logon.to, cap);
	} else {
		cpdlc_strlcpy(buf, "", cap);
	}
	mutex_exit(&cl->lock);

	return (n);
}

size_t
cpdlc_client_get_nda(cpdlc_client_t *cl, char *buf, size_t cap)
{
	size_t n = 0;

	ASSERT(cl != NULL);

	mutex_enter(&cl->lock);
	if (cl->logon.nda != NULL) {
		n = strlen(cl->logon.nda);
		cpdlc_strlcpy(buf, cl->logon.nda, cap);
	} else {
		cpdlc_strlcpy(buf, "", cap);
	}
	mutex_exit(&cl->lock);

	return (n);
}

void
cpdlc_client_logon(cpdlc_client_t *cl, const char *logon_data,
    const char *from, const char *to)
{
	ASSERT(cl != NULL);
	ASSERT(logon_data != NULL);
	ASSERT(from != NULL);

	mutex_enter(&cl->lock);

	free(cl->logon.data);
	free(cl->logon.from);
	free(cl->logon.to);
	cl->logon.do_logon = true;
	cl->logon.data = strdup(logon_data);
	cl->logon.from = strdup(from);
	free(cl->logon.nda);
	cl->logon.nda = NULL;
	if (to != NULL)
		cl->logon.to = strdup(to);
	else
		cl->logon.to = NULL;
	set_logon_failure(cl, NULL);

	if (!cl->worker_started) {
		cl->worker_started = true;
		VERIFY(thread_create(&cl->worker, logon_worker, cl));
	}

	mutex_exit(&cl->lock);
}

void
cpdlc_client_logoff(cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);
	mutex_enter(&cl->lock);
	cl->logon_status = CPDLC_LOGON_NONE;
	set_logon_failure(cl, NULL);
	mutex_exit(&cl->lock);
}

cpdlc_logon_status_t
cpdlc_client_get_logon_status(cpdlc_client_t *cl, char logon_failure[128])
{
	cpdlc_logon_status_t st;

	ASSERT(cl != NULL);

	mutex_enter(&cl->lock);
	if (logon_failure != NULL)
		cpdlc_strlcpy(logon_failure, cl->logon_failure, 128);
	st = cl->logon_status;
	mutex_exit(&cl->lock);

	return (st);
}

void
cpdlc_client_reset_logon_failure(cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);
	mutex_enter(&cl->lock);
	set_logon_failure(cl, NULL);
	mutex_exit(&cl->lock);
}

static cpdlc_msg_token_t
send_msg_impl(cpdlc_client_t *cl, const cpdlc_msg_t *msg, bool track_sent)
{
	outmsgbuf_t *outmsgbuf;

	ASSERT(cl != NULL);
	ASSERT(msg != NULL);

	outmsgbuf = safe_calloc(1, sizeof (*outmsgbuf));
	outmsgbuf->token = cl->outmsgbufs.next_tok++;
	outmsgbuf->bufsz = cpdlc_msg_encode(msg, NULL, 0);
	outmsgbuf->buf = safe_malloc(SENDBUF_PRE_PAD + outmsgbuf->bufsz + 1);
	cpdlc_msg_encode(msg, &outmsgbuf->buf[SENDBUF_PRE_PAD],
	    outmsgbuf->bufsz + 1);
	outmsgbuf->track_sent = track_sent;

	list_insert_tail(&cl->outmsgbufs.sending, outmsgbuf);

	return (outmsgbuf->token);
}

cpdlc_msg_token_t
cpdlc_client_send_msg(cpdlc_client_t *cl, const cpdlc_msg_t *msg)
{
	cpdlc_msg_token_t tok;

	ASSERT(cl != NULL);
	ASSERT(msg != NULL);

	mutex_enter(&cl->lock);
	if (cl->logon_status != CPDLC_LOGON_COMPLETE) {
		mutex_exit(&cl->lock);
		return (CPDLC_INVALID_MSG_TOKEN);
	}
	tok = send_msg_impl(cl, msg, true);
	mutex_exit(&cl->lock);

	return (tok);
}

cpdlc_msg_status_t
cpdlc_client_get_msg_status(cpdlc_client_t *cl, cpdlc_msg_token_t token)
{
	cpdlc_msg_status_t status = CPDLC_MSG_STATUS_INVALID_TOKEN;

	ASSERT(cl != NULL);
	ASSERT(token != CPDLC_INVALID_MSG_TOKEN);

	mutex_enter(&cl->lock);

	for (outmsgbuf_t *outmsgbuf = list_head(&cl->outmsgbufs.sent);
	    outmsgbuf != NULL;
	    outmsgbuf = list_next(&cl->outmsgbufs.sent, outmsgbuf)) {
		if (outmsgbuf->token == token) {
			status = outmsgbuf->status;
			list_remove(&cl->outmsgbufs.sent, outmsgbuf);
			free(outmsgbuf->buf);
			free(outmsgbuf);
			goto out;
		}
	}
	for (outmsgbuf_t *outmsgbuf = list_head(&cl->outmsgbufs.sending);
	    outmsgbuf != NULL;
	    outmsgbuf = list_next(&cl->outmsgbufs.sending, outmsgbuf)) {
		if (outmsgbuf->token == token) {
			status = outmsgbuf->status;
			goto out;
		}
	}
out:
	mutex_exit(&cl->lock);

	return (status);
}

cpdlc_msg_t *
cpdlc_client_recv_msg(cpdlc_client_t *cl)
{
	inmsgbuf_t *inmsgbuf;
	cpdlc_msg_t *msg = NULL;

	mutex_enter(&cl->lock);

	inmsgbuf = cl->inmsgbufs.head;
	if (inmsgbuf != NULL) {
		list_remove(&cl->inmsgbufs, inmsgbuf);
		ASSERT(inmsgbuf->msg != NULL);
		msg = inmsgbuf->msg;
		free(inmsgbuf);
	}

	mutex_exit(&cl->lock);

	return (msg);
}

void
cpdlc_client_set_cb_userinfo(cpdlc_client_t *cl, void *userinfo)
{
	ASSERT(cl != NULL);
	mutex_enter(&cl->lock);
	cl->cb_userinfo = userinfo;
	mutex_exit(&cl->lock);
}

void *
cpdlc_client_get_cb_userinfo(cpdlc_client_t *cl)
{
	void *userinfo;

	ASSERT(cl != NULL);
	mutex_enter(&cl->lock);
	userinfo = cl->cb_userinfo;
	mutex_exit(&cl->lock);

	return (userinfo);
}

void
cpdlc_client_set_msg_sent_cb(cpdlc_client_t *cl, cpdlc_msg_sent_cb_t cb)
{
	ASSERT(cl != NULL);
	mutex_enter(&cl->lock);
	cl->msg_sent_cb = cb;
	mutex_exit(&cl->lock);
}

void
cpdlc_client_set_msg_recv_cb(cpdlc_client_t *cl, cpdlc_msg_recv_cb_t cb)
{
	ASSERT(cl != NULL);
	mutex_enter(&cl->lock);
	cl->msg_recv_cb = cb;
	mutex_exit(&cl->lock);
}
