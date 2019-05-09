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
			cl->logon_status = CPDLC_LOGON_NONE; \
			return; \
		} \
	} while (0)

#define	WORKER_POLL_INTVAL	100	/* ms */
#define	READBUF_SZ		4096	/* bytes */

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
	struct outmsgbuf_s	*next;
	struct outmsgbuf_s	*prev;
} outmsgbuf_t;

typedef struct inmsgbuf_s {
	cpdlc_msg_t		*msg;
	struct inmsgbuf_s	*next;
	struct inmsgbuf_s	*prev;
} inmsgbuf_t;

struct cpdlc_client_s {
	/* immutable */
	char				server_hostname[PATH_MAX];
	int				server_port;
	bool				is_atc;
	char				*cafile;
	char				*keyfile;
	char				*key_pass;
	gnutls_pkcs_encrypt_flags_t	key_enctype;
	char				*certfile;
	char				*key_pem_data;
	char				*cert_pem_data;

	mutex_t				lock;
	bool				bg_shutdown;
	/* protected by `lock' */
	thread_t			worker;
	bool				worker_started;
	cpdlc_client_logon_status_t	logon_status;
	int				sock;
	gnutls_session_t		session;
	gnutls_certificate_credentials_t xcred;

	struct {
		/* protected by `lock' */
		cpdlc_msg_token_t	next_tok;
		outmsgbuf_t		*sending_head;
		outmsgbuf_t		*sending_tail;
		outmsgbuf_t		*sent_head;
		outmsgbuf_t		*sent_tail;
	} outmsgbufs;

	/* Only accessed from worker thread */
	char			*inbuf;
	unsigned		inbuf_sz;
	struct {
		inmsgbuf_t	*head;
		inmsgbuf_t	*tail;
	} inmsgbufs;

	struct {
		/* protected by `lock' */
		bool	do_logon;
		char	*data;
		char	*from;
		char	*to;
	} logon;
};

static void reset_link_state(cpdlc_client_t *cl);
static void init_conn(cpdlc_client_t *cl);
static void complete_conn(cpdlc_client_t *cl);
static void tls_handshake(cpdlc_client_t *cl);
static void send_logon(cpdlc_client_t *cl);
static void poll_for_msgs(cpdlc_client_t *cl);
static cpdlc_msg_token_t send_msg_impl(cpdlc_client_t *cl,
    const cpdlc_msg_t *msg, bool track_sent);

static bool
set_fd_nonblock(int fd)
{
	int flags;
	return ((flags = fcntl(fd, F_GETFL)) >= 0 &&
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0);
}

static void
logon_worker(void *userinfo)
{
	cpdlc_client_t *cl = userinfo;

	ASSERT(cl != NULL);

	mutex_enter(&cl->lock);

	init_conn(cl);

	while (cl->logon_status != CPDLC_LOGON_NONE) {
		switch (cl->logon_status) {
		case CPDLC_LOGON_CONNECTING_LINK:
			ASSERT(cl->sock != -1);
			complete_conn(cl);
			break;
		case CPDLC_LOGON_HANDSHAKING_LINK:
			ASSERT(cl->sock != -1);
			tls_handshake(cl);
			break;
		case CPDLC_LOGON_LINK_AVAIL:
			if (cl->logon.do_logon)
				send_logon(cl);
			else
				poll_for_msgs(cl);
			break;
		case CPDLC_LOGON_IN_PROG:
		case CPDLC_LOGON_COMPLETE:
			poll_for_msgs(cl);
			break;
		default:
			VERIFY_MSG(0, "client reached impossible "
			    "logon_status = %x", cl->logon_status);
		}
	}
	reset_link_state(cl);
	cl->worker_started = false;

	mutex_exit(&cl->lock);
}

cpdlc_client_t *
cpdlc_client_init(const char *server_hostname, int server_port,
    const char *cafile, bool is_atc)
{
	cpdlc_client_t *cl = safe_calloc(1, sizeof (*cl));

	mutex_init(&cl->lock);

	ASSERT(server_hostname != NULL);
	if (server_port == 0)
		server_port = 17622;

	cpdlc_strlcpy(cl->server_hostname, server_hostname,
	    sizeof (cl->server_hostname));
	if (cafile != NULL)
		cl->cafile = secure_strdup(cafile);
	cl->server_port = server_port;
	cl->is_atc = is_atc;

	cl->sock = -1;

	return (cl);
}

static void
clear_key_data(cpdlc_client_t *cl)
{
	secure_free(cl->keyfile);
	secure_free(cl->key_pass);
	secure_free(cl->certfile);
	secure_free(cl->key_pem_data);
	secure_free(cl->cert_pem_data);

	cl->keyfile = NULL;
	cl->key_pass = NULL;
	cl->key_enctype = GNUTLS_PKCS_PLAIN;
	cl->certfile = NULL;
	cl->key_pem_data = NULL;
	cl->cert_pem_data = NULL;
}

void
cpdlc_client_fini(cpdlc_client_t *cl)
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
	ASSERT3P(cl->outmsgbufs.sending_head, ==, NULL);
	ASSERT3P(cl->outmsgbufs.sent_head, ==, NULL);

	free(cl->cafile);

	clear_key_data(cl);

	mutex_destroy(&cl->lock);

	free(cl->logon.data);
	free(cl->logon.from);
	free(cl->logon.to);
	free(cl);
}

void
cpdlc_client_set_key_file(cpdlc_client_t *cl, const char *keyfile,
    const char *key_pass, gnutls_pkcs_encrypt_flags_t key_enctype,
    const char *certfile)
{
	ASSERT(cl != NULL);
	ASSERT(keyfile != NULL || certfile == NULL);
	ASSERT(key_pass != NULL || key_enctype == GNUTLS_PKCS_PLAIN);

	mutex_enter(&cl->lock);

	clear_key_data(cl);

	if (keyfile != NULL)
		cl->keyfile = secure_strdup(keyfile);
	if (certfile != NULL)
		cl->certfile = secure_strdup(certfile);
	if (key_pass != NULL)
		cl->key_pass = secure_strdup(key_pass);
	cl->key_enctype = key_enctype;

	mutex_exit(&cl->lock);
}

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

static void
reset_link_state(cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);
	ASSERT3U(cl->logon_status, ==, CPDLC_LOGON_NONE);
	if (cl->session != NULL) {
		if (cl->logon_status >= CPDLC_LOGON_LINK_AVAIL)
			gnutls_bye(cl->session, GNUTLS_SHUT_WR);
		gnutls_deinit(cl->session);
		cl->session = NULL;
	}
	if (cl->xcred != NULL) {
		gnutls_certificate_free_credentials(cl->xcred);
		cl->xcred = NULL;
	}
	if (cl->sock != -1) {
		close(cl->sock);
		cl->sock = -1;
	}

	while (cl->outmsgbufs.sending_head != NULL) {
		outmsgbuf_t *outmsgbuf = cl->outmsgbufs.sending_head;
		LIST_REMOVE(cl->outmsgbufs.sending_head, cl->outmsgbufs.sending_tail,
		    outmsgbuf);
		free(outmsgbuf->buf);
		free(outmsgbuf);
	}
	while (cl->outmsgbufs.sent_head != NULL) {
		outmsgbuf_t *outmsgbuf = cl->outmsgbufs.sent_head;
		LIST_REMOVE(cl->outmsgbufs.sent_head, cl->outmsgbufs.sent_tail,
		    outmsgbuf);
		free(outmsgbuf->buf);
		free(outmsgbuf);
	}

	free(cl->inbuf);
	cl->inbuf = NULL;
	cl->inbuf_sz = 0;

	while (cl->inmsgbufs.head != NULL) {
		inmsgbuf_t *inmsgbuf = cl->inmsgbufs.head;
		LIST_REMOVE(cl->inmsgbufs.head, cl->inmsgbufs.tail, inmsgbuf);
		cpdlc_msg_free(inmsgbuf->msg);
		free(inmsgbuf);
	}
}

static void
init_conn(cpdlc_client_t *cl)
{
	struct addrinfo *ai = NULL;
	int result, sock;
	char portbuf[8];
	struct addrinfo hints = {
	    .ai_family = AF_UNSPEC,
	    .ai_socktype = SOCK_STREAM,
	    .ai_protocol = IPPROTO_TCP
	};
	cpdlc_client_logon_status_t new_status;

	ASSERT(cl != NULL);
	ASSERT3S(cl->sock, ==, -1);
	ASSERT3U(cl->logon_status, ==, CPDLC_LOGON_NONE);

	snprintf(portbuf, sizeof (portbuf), "%d", cl->server_port);
	result = getaddrinfo(cl->server_hostname, portbuf, &hints, &ai);
	if (result != 0) {
		fprintf(stderr, "Can't resolve %s: %s\n", cl->server_hostname,
		    gai_strerror(result));
		goto out;
	}
	ASSERT(ai != NULL);
	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sock == -1) {
		perror("Cannot create socket");
		goto out;
	}
	if (!set_fd_nonblock(sock)) {
		perror("Cannot socket to non-blocking");
		close(sock);
		goto out;
	}
	if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
		if (errno != EINPROGRESS) {
			perror("Cannot connect socket");
			close(sock);
			goto out;
		}
		new_status = CPDLC_LOGON_CONNECTING_LINK;
	} else {
		new_status = CPDLC_LOGON_HANDSHAKING_LINK;
	}

	cl->sock = sock;
	cl->logon_status = new_status;
out:
	if (ai != NULL)
		freeaddrinfo(ai);
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
		/* Still in progress */
		return;
	case 1:
		/* Connection attempt completed. Let's see about the result. */
		if (getsockopt(cl->sock, SOL_SOCKET, SO_ERROR, &so_error,
		    &so_error_len) < 0) {
			perror("Error during getsockopt");
			goto errout;
		}
		if (so_error != 0) {
			fprintf(stderr, "Error connecting to server %s:%d: %s",
			    cl->server_hostname, cl->server_port,
			    strerror(so_error));
			goto errout;
		}
		/* Success! */
		cl->logon_status = CPDLC_LOGON_HANDSHAKING_LINK;
		return;
	default:
		perror("Error polling on socket during connection");
		goto errout;
	}
errout:
	cl->logon_status = CPDLC_LOGON_NONE;
}

static void
tls_handshake(cpdlc_client_t *cl)
{
	int ret;

	ASSERT(cl != NULL);
	ASSERT3U(cl->logon_status, ==, CPDLC_LOGON_HANDSHAKING_LINK);

	if (cl->session == NULL) {
		TLS_CHK(gnutls_certificate_allocate_credentials(&cl->xcred));
		if (cl->keyfile != NULL) {
			ASSERT(cl->certfile != NULL);
			TLS_CHK(gnutls_certificate_set_x509_key_file2(cl->xcred,
			    cl->certfile, cl->keyfile, GNUTLS_X509_FMT_PEM,
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
		    cl->server_hostname, strlen(cl->server_hostname)));
		TLS_CHK(gnutls_set_default_priority(cl->session));
		TLS_CHK(gnutls_credentials_set(cl->session,
		    GNUTLS_CRD_CERTIFICATE, cl->xcred));
		gnutls_session_set_verify_cert(cl->session,
		    cl->server_hostname, 0);
		ASSERT(cl->sock != -1);
		gnutls_transport_set_int(cl->session, cl->sock);
		gnutls_handshake_set_timeout(cl->session,
		    GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
	}

	ret = gnutls_handshake(cl->session);
	if (ret < GNUTLS_E_SUCCESS) {
		if (ret != GNUTLS_E_AGAIN)
			cl->logon_status = CPDLC_LOGON_NONE;
		return;
	}
	cl->logon_status = CPDLC_LOGON_LINK_AVAIL;
}

static void
send_logon(cpdlc_client_t *cl)
{
	cpdlc_msg_t *msg = cpdlc_msg_alloc(0, 0);

	ASSERT(cl != NULL);
	ASSERT(cl->logon.data != NULL);
	ASSERT(cl->logon.from != NULL);
	ASSERT(cl->logon.to != NULL);

	cpdlc_msg_set_logon_data(msg, cl->logon.data);
	cpdlc_msg_set_from(msg, cl->logon.from);
	if (cl->logon.to != NULL)
		cpdlc_msg_set_to(msg, cl->logon.to);
	send_msg_impl(cl, msg, false);
	cpdlc_msg_free(msg);

	cl->logon_status = CPDLC_LOGON_IN_PROG;
}

static void
queue_incoming_msg(cpdlc_client_t *cl, cpdlc_msg_t *msg)
{
	inmsgbuf_t *inmsgbuf;

	ASSERT(cl != NULL);
	ASSERT3U(cl->logon_status, ==, CPDLC_LOGON_COMPLETE);
	ASSERT(msg != NULL);
	ASSERT(msg->segs[0].info != NULL);

	if (msg->segs[0].info->msg_type == CPDLC_UM161_END_SVC) {
		cpdlc_msg_free(msg);
		cl->logon_status = CPDLC_LOGON_LINK_AVAIL;
		return;
	}

	inmsgbuf = safe_calloc(1, sizeof (*inmsgbuf));
	inmsgbuf->msg = msg;
	LIST_INSERT_TAIL(cl->inmsgbufs.head, cl->inmsgbufs.tail, inmsgbuf);
}

static void
process_msg(cpdlc_client_t *cl, cpdlc_msg_t *msg)
{
	ASSERT(cl != NULL);
	ASSERT(msg != NULL);

	switch (cl->logon_status) {
	case CPDLC_LOGON_LINK_AVAIL:
		/* Discard any incoming message traffic in this state. */
		cpdlc_msg_free(msg);
		break;
	case CPDLC_LOGON_IN_PROG:
		if (msg->is_logon) {
			const char *logon_data = cpdlc_msg_get_logon_data(msg);

			if (strcmp(logon_data, "SUCCESS") == 0)
				cl->logon_status = CPDLC_LOGON_COMPLETE;
			else
				cl->logon_status = CPDLC_LOGON_LINK_AVAIL;
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
			queue_incoming_msg(cl, msg);
		}
		break;
	default:
		VERIFY_MSG(0, "Invalid client state %x", cl->logon_status);
	}
}

static void
process_input(cpdlc_client_t *cl)
{
	size_t consumed_total = 0;

	ASSERT(cl != NULL);
	ASSERT(cl->inbuf != NULL);
	ASSERT(cl->inbuf_sz != 0);

	while (consumed_total < cl->inbuf_sz) {
		int consumed;
		cpdlc_msg_t *msg;

		/* Try to decode a message from our accumulated input. */
		ASSERT3S(consumed_total, <=, cl->inbuf_sz);
		if (!cpdlc_msg_decode(&cl->inbuf[consumed_total], &msg,
		    &consumed)) {
			cl->logon_status = CPDLC_LOGON_NONE;
			break;
		}
		/* No more complete messages pending? */
		if (msg == NULL)
			break;
		ASSERT(consumed != 0);
		process_msg(cl, msg);
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
}

static bool
validate_input(const uint8_t *buf, int l)
{
	for (int i = 0; i < l; i++) {
		if (buf[i] == 0 || buf[i] > 127) {
			fprintf(stderr, "Invalid input character on "
			    "connection: data MUST be plain text");
			return (false);
		}
	}
	return (true);
}

static void
do_msg_input(cpdlc_client_t *cl)
{
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
		if (!validate_input(buf, bytes)) {
			cl->logon_status = CPDLC_LOGON_NONE;
			break;
		}
		cl->inbuf = realloc(cl->inbuf, cl->inbuf_sz + bytes + 1);
		cpdlc_strlcpy(&cl->inbuf[cl->inbuf_sz], (const char *)buf,
		    bytes + 1);
		cl->inbuf_sz += bytes;

		process_input(cl);
	}
}

static void
do_msg_output(cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);

	for (outmsgbuf_t *outmsgbuf = cl->outmsgbufs.sending_head;
	    outmsgbuf != NULL; outmsgbuf = cl->outmsgbufs.sending_head) {
		int bytes = gnutls_record_send(cl->session,
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
		outmsgbuf->bytes_sent += bytes;
		ASSERT3S(outmsgbuf->bytes_sent, <=, outmsgbuf->bufsz);
		LIST_REMOVE(cl->outmsgbufs.sending_head,
		    cl->outmsgbufs.sending_tail, outmsgbuf);
		/* Don't need the buffer inside anymore */
		free(outmsgbuf->buf);
		outmsgbuf->buf = NULL;
		outmsgbuf->bufsz = 0;
		if (outmsgbuf->track_sent) {
			LIST_INSERT_TAIL(cl->outmsgbufs.sent_head,
			    cl->outmsgbufs.sent_tail, outmsgbuf);
		} else {
			free(outmsgbuf);
		}
	}
}

static void
poll_for_msgs(cpdlc_client_t *cl)
{
	struct pollfd pfd;
	int ret;

	ASSERT(cl != NULL);
	ASSERT3U(cl->logon_status, >=, CPDLC_LOGON_LINK_AVAIL);
	ASSERT(cl->session != NULL);
	ASSERT(cl->sock != -1);

	pfd.fd = cl->sock;
	pfd.events = POLLIN;
	if (cl->outmsgbufs.sending_head != NULL)
		pfd.events |= POLLOUT;

	ret = poll(&pfd, 1, WORKER_POLL_INTVAL);
	/* Poll error */
	if (ret < 0) {
		cl->logon_status = CPDLC_LOGON_NONE;
		return;
	}

	if (pfd.revents & POLLIN)
		do_msg_input(cl);
	/* In case input killed the connection, recheck logon_status */
	if (cl->logon_status != CPDLC_LOGON_NONE && (pfd.revents & POLLOUT))
		do_msg_output(cl);
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
	if (to != NULL)
		cl->logon.to = strdup(to);
	else
		cl->logon.to = NULL;

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
	mutex_exit(&cl->lock);
}

cpdlc_client_logon_status_t
cpdlc_client_get_logon_status(const cpdlc_client_t *cl)
{
	ASSERT(cl != NULL);
	return (cl->logon_status);
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
	outmsgbuf->buf = safe_malloc(outmsgbuf->bufsz + 1);
	cpdlc_msg_encode(msg, outmsgbuf->buf, outmsgbuf->bufsz + 1);
	outmsgbuf->track_sent = track_sent;

	LIST_INSERT_TAIL(cl->outmsgbufs.sending_head,
	    cl->outmsgbufs.sending_tail, outmsgbuf);

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

	for (outmsgbuf_t *outmsgbuf = cl->outmsgbufs.sent_head; outmsgbuf != NULL;
	    outmsgbuf = outmsgbuf->next) {
		if (outmsgbuf->token == token) {
			status = outmsgbuf->status;
			LIST_REMOVE(cl->outmsgbufs.sent_head,
			    cl->outmsgbufs.sent_tail, outmsgbuf);
			free(outmsgbuf->buf);
			free(outmsgbuf);
			goto out;
		}
	}
	for (outmsgbuf_t *outmsgbuf = cl->outmsgbufs.sending_head;
	    outmsgbuf != NULL; outmsgbuf = outmsgbuf->next) {
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
		LIST_REMOVE(cl->inmsgbufs.head, cl->inmsgbufs.tail, inmsgbuf);
		ASSERT(inmsgbuf->msg != NULL);
		msg = inmsgbuf->msg;
		free(inmsgbuf);
	}

	mutex_exit(&cl->lock);

	return (msg);
}
