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
#include <getopt.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <acfutils/avl.h>
#include <acfutils/conf.h>
#include <acfutils/htbl.h>
#include <acfutils/list.h>
#include <acfutils/safe_alloc.h>

#include "../src/cpdlc.h"

#define	CALLSIGN_LEN		16
#define	DEFAULT_PORT		17622
#define	CONN_BACKLOG		UINT16_MAX
#define	READ_BUF_SZ		4096
#define	MAX_BUF_SZ		8192
#define	MAX_BUF_SZ_NO_LOGON	128
#define	MAX_ADDR_LEN	\
	MAX(sizeof(struct sockaddr_in6), sizeof(struct sockaddr_in))

typedef struct {
	char		callsign[CALLSIGN_LEN];
	avl_node_t	node;
} atc_t;

typedef struct {
	char		from[CALLSIGN_LEN];
	char		to[CALLSIGN_LEN];
	bool		logon_complete;

	uint8_t		addr_buf[MAX_ADDR_LEN];
	socklen_t	addr_len;
	int		fd;

	avl_node_t	node;
	avl_node_t	from_node;

	uint8_t		*inbuf;
	size_t		inbuf_sz;
	uint8_t		*outbuf;
	size_t		outbuf_sz;
} conn_t;

typedef struct {
	char		from[CALLSIGN_LEN];
	char		to[CALLSIGN_LEN];
	char		*msg;
	list_node_t	node;
} queued_msg_t;

typedef struct {
	uint8_t		addr_buf[MAX_ADDR_LEN];
	socklen_t	addr_len;
	int		fd;
	avl_node_t	node;
} listen_sock_t;

static avl_tree_t	atcs;
static avl_tree_t	conns;
static htbl_t		conns_by_from;
static list_t		queued_msgs;
static avl_tree_t	listen_socks;

static bool		background = true;
static bool		do_shutdown = false;

static void send_error_msg(conn_t *conn, const cpdlc_msg_t *orig_msg,
    const char *fmt, ...);

static bool
set_sock_nonblock(int fd)
{
	int flags;

	return ((flags = fcntl(fd, F_GETFL)) >= 0 &&
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0);
}

static int
atc_compar(const void *a, const void *b)
{
	const atc_t *aa = a, *ab = b;
	int res = strcmp(aa->callsign, ab->callsign);
	if (res < 0)
		return (-1);
	if (res > 0)
		return (1);
	return (0);
}

static int
conn_compar(const void *a, const void *b)
{
	const conn_t *ca = a, *cb = b;
	int res;

	if (ca->addr_len < cb->addr_len)
		return (-1);
	if (ca->addr_len > cb->addr_len)
		return (1);
	res = memcmp(ca->addr_buf, cb->addr_buf, sizeof (ca->addr_len));
	if (res < 0)
		return (-1);
	if (res > 0)
		return (1);
	return (0);
}

static int
listen_sock_compar(const void *a, const void *b)
{
	const listen_sock_t *la = a, *lb = b;
	int res;

	if (la->addr_len < lb->addr_len)
		return (-1);
	if (la->addr_len > lb->addr_len)
		return (1);
	res = memcmp(la->addr_buf, lb->addr_buf, sizeof (la->addr_len));
	if (res < 0)
		return (-1);
	if (res > 0)
		return (1);
	return (0);
}

static void
init_structs(void)
{
	avl_create(&atcs, atc_compar, sizeof (atc_t), offsetof(atc_t, node));
	avl_create(&conns, conn_compar, sizeof (conn_t),
	    offsetof(conn_t, node));
	htbl_create(&conns_by_from, 1024, CALLSIGN_LEN, true);
	list_create(&queued_msgs, sizeof (queued_msg_t),
	    offsetof(queued_msg_t, node));
	avl_create(&listen_socks, listen_sock_compar, sizeof (listen_sock_t),
	    offsetof(listen_sock_t, node));
}

static void
fini_structs(void)
{
	void *cookie;
	atc_t *atc;
	conn_t *conn;
	queued_msg_t *msg;
	listen_sock_t *ls;

	cookie = NULL;
	while ((atc = avl_destroy_nodes(&atcs, &cookie)) != NULL)
		free(atc);
	avl_destroy(&atcs);

	htbl_empty(&conns_by_from, NULL, NULL);
	htbl_destroy(&conns_by_from);

	cookie = NULL;
	while ((conn = avl_destroy_nodes(&conns, &cookie)) != NULL) {
		close(conn->fd);
		free(conn);
	}
	avl_destroy(&conns);

	while ((msg = list_remove_head(&queued_msgs)) != NULL) {
		free(msg->msg);
		free(msg);
	}
	list_destroy(&queued_msgs);

	cookie = NULL;
	while ((ls = avl_destroy_nodes(&listen_socks, &cookie)) != NULL) {
		if (ls->fd != -1)
			close(ls->fd);
		free(ls);
	}
	avl_destroy(&listen_socks);
}

static void
print_usage(const char *progname, FILE *fp)
{
	fprintf(fp, "Usage: %s [-h] [-c <conffile>]\n", progname);
}

static bool
add_atc(const char *callsign)
{
	atc_t *atc = safe_calloc(1, sizeof (*atc));
	atc_t *old_atc;
	avl_index_t where;

	lacf_strlcpy(atc->callsign, callsign, sizeof (atc->callsign));
	old_atc = avl_find(&atcs, atc, &where);
	if (old_atc != NULL) {
		fprintf(stderr, "Duplicate ATC entry %s", callsign);
		return (false);
	}
	avl_insert(&atcs, atc, where);

	return (true);
}

static bool
add_listen_sock(const char *name_port)
{
	char hostname[64];
	int port = DEFAULT_PORT;
	const char *colon = strchr(name_port, ':');
	struct hostent *host;

	if (colon != NULL) {
		lacf_strlcpy(hostname, name_port, (colon - name_port) + 1);
		if (sscanf(&colon[1], "%d", &port) != 1 ||
		    port <= 0 || port > UINT16_MAX) {
			fprintf(stderr, "Invalid listen directive \"%s\": "
			    "expected valid port number after ':' character\n",
			    name_port);
			return (false);
		}
	} else {
		lacf_strlcpy(hostname, name_port, sizeof (hostname));
	}

	host = gethostbyname(hostname);
	if (host == NULL) {
		fprintf(stderr, "Invalid listen directive \"%s\": "
		    "hostname not found\n", name_port);
		return (false);
	}

	for (char **h_addr_list = host->h_addr_list; *h_addr_list != NULL;
	    h_addr_list++) {
		listen_sock_t *ls = safe_calloc(1, sizeof (*ls));
		listen_sock_t *old_ls;
		avl_index_t where;
		struct sockaddr_in sa4 = { .sin_family = AF_INET };
		struct sockaddr_in6 sa6 = { .sin6_family = AF_INET6 };
		const struct sockaddr *sa;
		socklen_t sa_len;

		memcpy(ls->addr_buf, *h_addr_list, host->h_length);
		ls->addr_len = host->h_length;

		old_ls = avl_find(&listen_socks, ls, &where);
		if (old_ls != NULL) {
			fprintf(stderr, "Invalid listen directive \"%s\": "
			    "address already used on another socket\n",
			    name_port);
			free(ls);
			return (false);
		}
		avl_insert(&listen_socks, ls, where);

		ls->fd = socket(host->h_addrtype, SOCK_STREAM, IPPROTO_TCP);
		if (ls->fd == -1) {
			fprintf(stderr, "Invalid listen directive \"%s\": "
			    "cannot create socket: %s\n", name_port,
			    strerror(errno));
			return (false);
		}
		if (host->h_addrtype == AF_INET6) {
			sa6.sin6_port = htons(port);
			sa6.sin6_addr = *(struct in6_addr *)h_addr_list;
			sa = (struct sockaddr *)&sa6;
			sa_len = sizeof (sa6);
		} else {
			sa4.sin_port = htons(port);
			sa4.sin_addr = *(struct in_addr *)h_addr_list;
			sa = (struct sockaddr *)&sa4;
			sa_len = sizeof (sa4);
		}
		if (bind(ls->fd, sa, sa_len) == -1) {
			fprintf(stderr, "Invalid listen directive \"%s\": "
			    "cannot bind socket: %s\n", name_port,
			    strerror(errno));
			return (false);
		}
		if (listen(ls->fd, CONN_BACKLOG) == -1) {
			fprintf(stderr, "Invalid listen directive \"%s\": "
			    "cannot listen on socket: %s\n", name_port,
			    strerror(errno));
			return (false);
		}
		if (!set_sock_nonblock(ls->fd)) {
			fprintf(stderr, "Invalid listen directive \"%s\": "
			    "cannot set socket as non-blocking: %s\n",
			    name_port, strerror(errno));
			return (false);
		}
	}

	return (true);
}

static bool
parse_config(const char *conf_path)
{
	int errline;
	conf_t *conf = conf_read_file(conf_path, &errline);
	const char *key, *value;
	void *cookie;

	if (conf == NULL) {
		fprintf(stderr, "Parsing error in config file on line %d\n",
		    errline);
		return (false);
	}

	cookie = NULL;
	while (conf_walk(conf, &key, &value, &cookie)) {
		if (strncmp(key, "atc/name/", 9) == 0) {
			if (!add_atc(value))
				goto errout;
		} else if (strncmp(key, "listen/", 7) == 0) {
			if (!add_listen_sock(value))
				goto errout;
		}
	}

	if (avl_numnodes(&atcs) == 0) {
		fprintf(stderr, "Error: no \"atc\" lines found in config\n");
		goto errout;
	}
	if (avl_numnodes(&listen_socks) == 0) {
		if (!add_listen_sock("localhost"))
			goto errout;
	}

	conf_free(conf);
	return (true);
errout:
	conf_free(conf);
	return (false);
}

static bool
daemonize(bool do_chdir, bool do_close)
{
	switch (fork()) {
	case -1:
		perror("fork error");
		return (false);
	case 0:
		break;
	default:
		_exit(EXIT_SUCCESS);
	}
	if (setsid() < 0) {
		perror("setsid error");
		return (false);
	}
	if (do_chdir)
		chdir("/");
	if (do_close) {
		close(STDIN_FILENO);
		if (open("/dev/null", O_RDONLY) < 0) {
			perror("Cannot replace STDIN with /dev/null");
			return (false);
		}
	}

	return (true);
}

static void
handle_accepts(listen_sock_t *ls)
{
	for (;;) {
		conn_t *conn = safe_calloc(1, sizeof (*conn));
		conn_t *old_conn;
		avl_index_t where;

		conn->addr_len = sizeof (conn->addr_buf);
		conn->fd = accept(ls->fd, (struct sockaddr *)conn->addr_buf,
		    &conn->addr_len);
		if (conn->fd == -1) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				fprintf(stderr, "Error accepting connection: "
				    "%s", strerror(errno));
				free(conn);
			}
			break;
		}
		set_sock_nonblock(conn->fd);
		old_conn = avl_find(&conns, conn, &where);
		if (old_conn != NULL) {
			fprintf(stderr, "Error accepting connection: "
			    "duplicate connection encountered?!");
			close(conn->fd);
			free(conn);
		} else {
			avl_insert(&conns, conn, where);
		}
	}
}

static void
close_conn(conn_t *conn)
{
	avl_remove(&conns, conn);
	close(conn->fd);
	free(conn->inbuf);
	free(conn->outbuf);
	free(conn);
}

static void
conn_remove_from(conn_t *conn)
{
	const list_t *l;

	ASSERT(conn != NULL);
	ASSERT(conn->logon_complete);

	l = htbl_lookup_multi(&conns_by_from, conn->from);
	ASSERT(l != NULL);
	for (void *mv = list_head(l); mv != NULL; mv = list_next(l, mv)) {
		conn_t *c = HTBL_VALUE_MULTI(mv);

		if (conn == c) {
			htbl_remove_multi(&conns_by_from, conn->from, mv);
			break;
		}
	}
	conn->logon_complete = false;
	memset(conn->from, 0, sizeof (conn->from));
}

static bool
process_logon_msg(conn_t *conn, const cpdlc_msg_t *msg)
{
	ASSERT(conn != NULL);
	ASSERT(msg != NULL);

	/* Authentication TODO */

	if (conn->logon_complete)
		conn_remove_from(conn);

	conn->logon_complete = true;
	lacf_strlcpy(conn->to, cpdlc_msg_get_to(msg), sizeof (conn->to));
	lacf_strlcpy(conn->from, cpdlc_msg_get_from(msg), sizeof (conn->from));

	if (conn->from[0] == '\0') {
		send_error_msg(conn, msg, "LOGON REQUIRES FROM= HEADER");
		return (false);
	}
	htbl_set(&conns_by_from, conn->from, conn);

	return (true);
}

static void
conn_send_msg(conn_t *conn, cpdlc_msg_t *msg)
{
	unsigned l;
	char *buf;

	ASSERT(conn != NULL);
	ASSERT(msg != NULL);

	l = cpdlc_msg_encode(msg, NULL, 0);
	buf = safe_malloc(l + 1);
	cpdlc_msg_encode(msg, buf, l + 1);

	if (conn->outbuf == NULL) {
		/* Direct send might be possible */
		ssize_t bytes = write(conn->fd, buf, l);

		if (bytes < 0) {
			if (errno != EINTR && errno != EWOULDBLOCK) {
				fprintf(stderr, "Socket send error: %s\n",
				    strerror(errno));
			}
		} else if (bytes < l) {
			conn->outbuf = safe_malloc((l - bytes) + 1);
			memcpy(conn->outbuf, &buf[bytes], (l - bytes) + 1);
			conn->outbuf_sz = l - bytes;
		}
	} else {
		conn->outbuf = safe_realloc(conn->outbuf,
		    conn->outbuf_sz + l + 1);
		lacf_strlcpy((char *)&conn->outbuf[conn->outbuf_sz], buf,
		    l + 1);
		/* Exclude training NUL char */
		conn->outbuf_sz += l;
	}

	free(buf);
}

static void
send_error_msg(conn_t *conn, const cpdlc_msg_t *orig_msg, const char *fmt, ...)
{
	int l;
	va_list ap;
	char *buf;
	cpdlc_msg_t *msg;

	ASSERT(conn != NULL);
	ASSERT(fmt != NULL);

	va_start(ap, fmt);
	l = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	buf = safe_malloc(l + 1);
	va_start(ap, fmt);
	vsnprintf(buf, l + 1, fmt, ap);
	va_end(ap);

	if (orig_msg != NULL)
		msg = cpdlc_msg_alloc(0, cpdlc_msg_get_min(orig_msg));
	else
		msg = cpdlc_msg_alloc(0, 0);
	cpdlc_msg_add_seg(msg, false, CPDLC_UM159_ERROR_description, 0);
	cpdlc_msg_seg_set_arg(msg, 0, 0, buf, NULL);

	conn_send_msg(conn, msg);

	free(buf);
	cpdlc_msg_free(msg);
}

static void
store_msg(const cpdlc_msg_t *msg, const char *to)
{
	queued_msg_t *qmsg = safe_calloc(1, sizeof (*qmsg));
	int l = cpdlc_msg_encode(msg, NULL, 0);
	char *buf = safe_malloc(l + 1);

	cpdlc_msg_encode(msg, buf, l + 1);
	qmsg->msg = buf;
	lacf_strlcpy(qmsg->from, cpdlc_msg_get_from(msg), sizeof (qmsg->from));
	lacf_strlcpy(qmsg->to, to, sizeof (qmsg->to));

	list_insert_tail(&queued_msgs, qmsg);
}

static void
conn_process_msg(conn_t *conn, cpdlc_msg_t *msg)
{
	char to[CALLSIGN_LEN] = { 0 };
	const list_t *l;

	ASSERT(conn != NULL);
	ASSERT(msg != NULL);

	if (!conn->logon_complete && !msg->is_logon) {
		send_error_msg(conn, msg, "LOGON REQUIRED");
		return;
	}
	if (msg->is_logon && !process_logon_msg(conn, msg))
		return;

	if (msg->to[0] != '\0') {
		lacf_strlcpy(to, msg->to, sizeof (to));
	} else if (conn->to[0] != '\0') {
		lacf_strlcpy(to, conn->to, sizeof (to));
	} else {
		send_error_msg(conn, msg, "MESSAGE MISSING TO= HEADER");
		return;
	}
	ASSERT(conn->from[0] != '\0');
	cpdlc_msg_set_from(msg, conn->from);

	l = htbl_lookup_multi(&conns_by_from, to);
	if (list_count(l) == 0) {
		store_msg(msg, to);
	} else {
		for (void *mv = list_head(l); mv != NULL;
		    mv = list_next(l, mv)) {
			conn_t *tgt_conn = HTBL_VALUE_MULTI(mv);

			ASSERT(tgt_conn != NULL);
			conn_send_msg(tgt_conn, msg);
		}
	}
}

static bool
conn_process_input(conn_t *conn)
{
	int consumed_total = 0;

	ASSERT(conn != NULL);
	ASSERT(conn->inbuf_sz != 0);

	for (;;) {
		int consumed;
		cpdlc_msg_t *msg;

		if (!cpdlc_msg_decode(
		    (const char *)&conn->inbuf[consumed_total], &msg,
		    &consumed)) {
			fprintf(stderr, "Error decoding message from client\n");
			return (false);
		}
		/* No more complete messages pending? */
		if (msg == NULL)
			break;
		ASSERT(consumed != 0);
		conn_process_msg(conn, msg);
		/*
		 * If the message was queued for later delivery, it will
		 * have been encoded into a textual form. So we can get
		 * rid of the in-memory representation now.
		 */
		cpdlc_msg_free(msg);
		consumed_total += consumed;
	}
	if (consumed_total != 0) {
		ASSERT3S(consumed_total, <=, conn->inbuf_sz);
		conn->inbuf_sz -= consumed_total;
		memmove(conn->inbuf, &conn->inbuf[consumed_total],
		    conn->inbuf_sz);
		conn->inbuf = realloc(conn->inbuf, conn->inbuf_sz);
	}

	return (true);
}

static bool
handle_conn_input(conn_t *conn)
{
	for (;;) {
		uint8_t buf[READ_BUF_SZ];
		size_t max_inbuf_sz = (conn->from[0] != '\0' ?
		    MAX_BUF_SZ : MAX_BUF_SZ_NO_LOGON);
		ssize_t bytes = read(conn->fd, buf, sizeof (buf));

		if (bytes < 0) {
			/* Read error, or no more data pending */
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return (true);
			fprintf(stderr, "Read error on connection: %s\n",
			    strerror(errno));
			close_conn(conn);
			return (false);
		}
		if (bytes == 0) {
			close_conn(conn);
			return (false);
		}
		for (ssize_t i = 0; i < bytes; i++) {
			/* Input sanitization, don't allow control chars */
			if (buf[i] == 0 || buf[i] > 127) {
				fprintf(stderr, "Invalid input character on "
				    "connection: data MUST be plain text\n");
				close_conn(conn);
				return (false);
			}
		}
		if (conn->inbuf_sz + bytes > max_inbuf_sz) {
			fprintf(stderr, "Input buffer overflow on connection: "
			    "wanted %d bytes, max %d bytes\n",
			    (int)(conn->inbuf_sz + bytes), (int)max_inbuf_sz);
			close_conn(conn);
			return (false);
		}

		conn->inbuf = safe_realloc(conn->inbuf, conn->inbuf_sz + bytes);
		memcpy(&conn->inbuf[conn->inbuf_sz], buf, bytes);
		conn->inbuf_sz += bytes;

		if (!conn_process_input(conn)) {
			close_conn(conn);
			return (false);
		}
	}
}

static void
handle_conn_output(conn_t *conn)
{
	ssize_t bytes;

	ASSERT(conn != NULL);
	ASSERT(conn->outbuf != NULL);

	bytes = write(conn->fd, conn->outbuf, conn->outbuf_sz);
	if (bytes < 0) {
		if (errno != EINTR && errno != EWOULDBLOCK) {
			fprintf(stderr, "Socket send error: %s\n",
			    strerror(errno));
		}
	} else if (bytes > 0) {
		if ((ssize_t)conn->outbuf_sz > bytes) {
			memmove(conn->outbuf, &conn->outbuf[bytes],
			    (conn->outbuf_sz - bytes) + 1);
			conn->outbuf = safe_realloc(conn->outbuf,
			    (conn->outbuf_sz - bytes) + 1);
			conn->outbuf_sz -= bytes;
		} else {
			free(conn->outbuf);
			conn->outbuf = NULL;
			conn->outbuf_sz = 0;
		}
	}
}

static void
poll_sockets(void)
{
	unsigned sock_nr = 0;
	unsigned num_pfds = avl_numnodes(&listen_socks) + avl_numnodes(&conns);
	struct pollfd *pfds = safe_calloc(num_pfds, sizeof (*pfds));
	int poll_res, polls_seen;

	for (listen_sock_t *ls = avl_first(&listen_socks); ls != NULL;
	    ls = AVL_NEXT(&listen_socks, ls), sock_nr++) {
		pfds[sock_nr].fd = ls->fd;
		pfds[sock_nr].events = POLLIN;
	}
	for (conn_t *conn = avl_first(&conns); conn != NULL;
	    conn = AVL_NEXT(&conns, conn), sock_nr++) {
		pfds[sock_nr].fd = conn->fd;
		pfds[sock_nr].events = POLLIN;
		if (conn->outbuf != NULL)
			pfds[sock_nr].events |= POLLOUT;
	}
	ASSERT3U(sock_nr, ==, num_pfds);

	poll_res = poll(pfds, num_pfds, -1);
	if (poll_res == -1 && errno != EINTR) {
		fprintf(stderr, "Error polling on sockets: %s\n",
		    strerror(errno));
		free(pfds);
		return;
	}

	polls_seen = 0;
	sock_nr = 0;
	for (listen_sock_t *ls = avl_first(&listen_socks); ls != NULL;
	    ls = AVL_NEXT(&listen_socks, ls), sock_nr++) {
		if (pfds[sock_nr].revents & (POLLIN | POLLPRI)) {
			handle_accepts(ls);
			polls_seen++;
			if (polls_seen == poll_res)
				goto out;
		}
	}
	for (conn_t *conn = avl_first(&conns), *next_conn = NULL;
	    conn != NULL; conn = next_conn, sock_nr++) {
		/*
		 * Grab the next connection handle now in case
		 * the connection gets closed due to EOF or errors.
		 */
		next_conn = AVL_NEXT(&conns, conn);
		if (pfds[sock_nr].revents & (POLLIN | POLLOUT)) {
			if (pfds[sock_nr].revents & POLLIN)
				handle_conn_input(conn);
			if (conn->outbuf != NULL &&
			    (pfds[sock_nr].revents & POLLOUT))
				handle_conn_output(conn);
			polls_seen++;
			if (polls_seen == poll_res)
				goto out;
		}
	}
out:

	free(pfds);
}

int
main(int argc, char *argv[])
{
	int opt;
	const char *conf_path = "cpdlcd.cfg";

	while ((opt = getopt(argc, argv, "hc:d")) != -1) {
		switch (opt) {
		case 'h':
			print_usage(argv[0], stdout);
			return (0);
		case 'c':
			conf_path = optarg;
			break;
		case 'd':
			background = false;
			break;
		default:
			print_usage(argv[0], stderr);
			return (1);
		}
	}

	init_structs();
	if (background && !daemonize(true, true))
		return (1);
	if (!parse_config(conf_path))
		return (1);

	while (!do_shutdown)
		poll_sockets();

	fini_structs();

	return (0);
}
