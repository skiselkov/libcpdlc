/*
 * Copyright 2020 Saso Kiselkov
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

#ifndef	_LIBCPDLC_NET_UTIL_H_
#define	_LIBCPDLC_NET_UTIL_H_

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef	_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else	/* !_WIN32 */
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/select.h>
#include <unistd.h>
#endif	/* !_WIN32 */

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	_WIN32

static inline bool
cpdlc_set_fd_nonblock(int fd)
{
	u_long mode = 1;
	return (ioctlsocket(fd, FIONBIO, &mode) == NO_ERROR);
}

#define	CPDLC_INVALID_SOCKET		INVALID_SOCKET
#define	CPDLC_SOCKET_IS_VALID(sock)	((sock) != INVALID_SOCKET)
#define	CPDLC_GAI_STRERROR		gai_strerrorA
typedef SOCKET cpdlc_socktype_t;
static inline bool
cpdlc_conn_in_prog(void)
{
	int err = WSAGetLastError();
	return (err == WSAEINPROGRESS || err == WSAEWOULDBLOCK);
}

static inline const char *
cpdlc_get_last_socket_error(void)
{
	static char buf[256];
	snprintf(buf, sizeof (buf), "WSA error %d", WSAGetLastError());
	return (buf);
}

#else	/* !defined(_WIN32) */

static inline bool
cpdlc_set_fd_nonblock(int fd)
{
	int flags;
	return ((flags = fcntl(fd, F_GETFL)) >= 0 &&
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0);
}

#define	CPDLC_INVALID_SOCKET		-1
#define	CPDLC_SOCKET_IS_VALID(sock)	((sock) != -1)
#define	CPDLC_GAI_STRERROR		gai_strerror
typedef int cpdlc_socktype_t;
static inline bool
cpdlc_conn_in_prog(void)
{
	return (errno == EINPROGRESS);
}

static inline const char *
cpdlc_get_last_socket_error(void)
{
	return (strerror(errno));
}

#endif	/* !defined(_WIN32) */

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_NET_UTIL_H_ */
