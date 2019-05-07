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

#ifdef	_WIN32
#include <windows.h>
#include <winsock2.h>
#else	/* !_WIN32 */
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif	/* !_WIN32 */

#include "cpdlc_alloc.h"
#include "cpdlc_assert.h"
#include "cpdlc_client.h"

struct cpdlc_client_s {
	char	server_hostname[PATH_MAX];
	char	cafile[PATH_MAX];
	int	server_port;
	bool	is_atc;

	cpdlc_client_logon_status_t		logon_status;
	gnutls_session_t			session;
	gnutls_certificate_credentials_t	xcred;

	int	sock;

	struct {
		char	*data;
		char	*from;
		char	*to;
	} logon;
};

static void reset_logon(cpdlc_client_t *cl);

static void
set_fd_nonblock(int fd)
{
	int flags;
	return ((flags = fcntl(fd, F_GETFL)) >= 0 &&
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0);
}

cpdlc_client_t *
cpdlc_client_init(const char *server_hostname, int server_port,
    const char *cafile, bool is_atc)
{
	cpdlc_client_t *cl = safe_calloc(1, sizeof (*cl));

	ASSERT(server_hostname != NULL);
	if (server_port == 0)
		server_port = 17622;

	cpdlc_strlcpy(cl->server_hostname, server_hostname,
	    sizeof (cl->server_hostname));
	if (cafile != NULL)
		cpdlc_strlcpy(cl->cafile, cafile, sizeof (cl->cafile));
	cl->server_port = server_port;
	cl->is_atc = is_atc;

	cl->sock = -1;

	return (cl);
}

void
cpdlc_client_fini(cpdlc_client *cl)
{
	ASSERT(cl != NULL);

	reset_logon(cl);
	free(cl->logon.data);
	free(cl->logon.from);
	free(cl->logon.to);
	free(cl);
}

void
cpdlc_client_set_logon_data(cpdlc_client_t *cl, const char *logon_data,
    const char *from, const char *to)
{
	ASSERT(cl != NULL);
	ASSERT(logon_data != NULL);
	ASSERT(from != NULL);
	ASSERT(to != NULL);

	free(cl->logon.data);
	free(cl->logon.from);
	free(cl->logon.to);
	cl->logon.data = strdup(logon_data);
	cl->logon.from = strdup(from);
	cl->logon.to = strdup(to);
}

static void
reset_logon(cpdlc_client_t *cl)
{
	if (cl->session != NULL) {
		if (cl->logon_status >= CPDLC_LOGON_LINK_AVAIL)
			gnutls_bye(cl->session);
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
	cl->logon_status = CPDLC_LOGON_NONE;
}

static bool
do_handshake(cpdlc_client_t *cl)
{
#define	GNUTLS_TRY(op) \
	do { \
		int error = (op); \
		if (error != GNUTLS_E_SUCCESS) { \
			fprintf(stderr, "GnuTLS error performing " #op \
			    ": %s\n", gnutls_strerror(error)); \
			reset_logon(cl); \
			return (false); \
		} \
	} while (0)

	int ret;

	ASSERT(cl != NULL);
	ASSERT3U(cl->logon_state, ==, CPDLC_LOGON_HANDSHAKING_LINK);

	if (cl->session == NULL) {
		GNUTLS_TRY(gnutls_certificate_allocate_credentials(&cl->xcred));
		if (cl->cafile[0] == '\0') {
			GNUTLS_TRY(gnutls_certificate_set_x509_system_trust(
			    cl->xcred));
		} else {
			GNUTLS_TRY(gnutls_certificate_set_x509_trust_file(
			    cl->xcred, cl->cafile, GNUTLS_X509_FMT_PEM));
		}
		GNUTLS_TRY(gnutls_init(&cl->session, GNUTLS_CLIENT));
		GNUTLS_TRY(gnutls_server_name_set(session, GNUTLS_NAME_DNS,
		    cl->server_hostname, strlen(cl->server_hostname)));
		GNUTLS_TRY(gnutls_set_default_priority(session));
		GNUTLS_TRY(gnutls_credentials_set(cl->session,
		    GNUTLS_CRD_CERTIFICATE, cl->xcred));
		gnutls_session_set_verify_cert(session, cl->server_hostname, 0);
		ASSERT(cl->sock != -1);
		gnutls_transport_set_int(cl->session, cl->sock);
		gnutls_handshake_set_timeout(cl->session,
		    GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
	}

	ret = gnutls_handshake(cl->session);
	if (ret < 0) {
		if (ret == GNUTLS_E_AGAIN)
			return (true);
	}

#undef	GNUTLS_TRY
}

static bool
socket_connect(cpdlc_client_t *cl)
{
	struct addrinfo *ai = NULL;
	int result, sock;
	char portbuf[8];
	struct addrinfo hints = {
	    .ai_family = AF_UNSPEC,
	    .ai_socktype = SOCK_STREAM,
	    .ai_protocol = IPPROTO_TCP
	};

	ASSERT3U(cl->logon_status, ==, CPDLC_LOGON_NONE);

	snprintf(portbuf, sizeof (portbuf), "%d", cl->server_port);
	result = getaddrinfo(cl->server_hostname, portbuf, &hints, &ai);
	if (result != 0) {
		fprintf(stderr, "Can't resolve %s: %s\n", cl->server_hostname,
		    gai_strerror(error));
		return (false);
	}
	ASSERT(ai != NULL);
	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sock == -1) {
		perror("Cannot create socket");
		freeaddrinfo(ai);
		return (false);
	}
	set_fd_nonblock(cl->sock)
	if (connect(sock, ai->addr, ai->addrlen) < 0) {
		if (errno != EINPROGRESS) {
			perror("Cannot connect socket");
			close(sock);
			freeaddrinfo(ai);
			return (false);
		}
		cl->logon_status = CPDLC_LOGON_CONNECTING_LINK;
	} else {
		cl->logon_status = CPDLC_LOGON_HANDSHAKING_LINK;
	}

	freeaddrinfo(ai);
	cl->sock = sock;

	if (cl->logon_status == CPDLC_LOGON_HANDSHAKING_LINK)
		return (do_handshake(cl));

	return (true);
}

static void
socket_connect_complete(cpdlc_client_t *cl)
{
	struct pollfd pfd = { .events = POLLOUT };
	int so_error;
	socklen_t so_error_len = sizeof (so_error);

	ASSERT(cl != NULL);
	ASSERT(cl->sock != -1);
	ASSERT3U(cl->logon_state, ==, CPDLC_LOGON_CONNECTING_LINK);

	pfd.fd = cl->sock;

	switch (poll(&pfd, 1, 0)) {
	case 0:
		/* Still in progress */
		return (true);
	case 1:
		/* Connection attempt completed. Let's see about the result. */
		if (getsockopt(cl->sock, SOL_SOCKET, SO_ERROR, &so_error,
		    &so_error_len) < 0) {
			perror("Error during getsockopt");
			reset_logon(cl);
			return (false);
		}
		if (so_error != 0) {
			fprintf(stderr, "Error connecting to server %s:%d: %s",
			    cl->server_hostname, cl->server_port,
			    strerror(so_error));
			reset_logon(cl);
			return (false);
		}
		return (do_handshake(cl));
	default:
		perror("Error polling on socket during connection");
		reset_logon(cl);
		return (false);
	}
}

bool
cpdlc_client_do_connect_link(cpdlc_client_t *cl);
{
	switch (cl->logon_status) {
	case CPDLC_LOGON_NONE:
		return (socket_connect(cl));
	case CPDLC_LOGON_CONNECTING_LINK:
		return (socket_connect_complete(cl));
	case CPDLC_LOGON_HANDSHAKING_LINK:
		return (do_handshake(cl));
	default:
		return (true);
	}
}

void
cpdlc_client_do_disconnect_link(cpdlc_client_t *cl)
{
	reset_logon(cl);
}

bool
cpdlc_client_do_logon(void)
{
	
}
