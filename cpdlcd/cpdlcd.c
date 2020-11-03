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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <curl/curl.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include <libwebsockets.h>

#include <acfutils/avl.h>
#include <acfutils/conf.h>
#include <acfutils/crc64.h>
#include <acfutils/htbl.h>
#include <acfutils/log.h>
#include <acfutils/list.h>
#include <acfutils/safe_alloc.h>
#include <acfutils/thread.h>

#include "../common/cpdlc_config_common.h"
#include "../src/cpdlc_client.h"
#include "../src/cpdlc_msg.h"
#include "../src/cpdlc_string.h"

#include "auth.h"
#include "blocklist.h"
#include "common.h"
#include "msgquota.h"
#include "msg_router.h"

#define	CONN_BACKLOG		UINT16_MAX
#define	READ_BUF_SZ		4096	/* bytes */
/*
 * Buffer size limits for logged on and non-logged-on connections. If a
 * message is not completed within this amount of bytes, the associated
 * connection is terminated with an error.
 */
#define	MAX_BUF_SZ		8192	/* bytes */
#define	MAX_BUF_SZ_NO_LOGON	128	/* bytes */
#define	POLL_TIMEOUT		500	/* ms */
/*
 * This value is tuned to be greater + a sufficient margin above the longest
 * possible message validity timeout (LONG_TIMEOUT in cpdlc_infos.c). This is
 * ATM 300 seconds (for low-priority instructions), so a 600 second timeout is
 * enough margin to definitely get those through within the validity period.
 */
#define	QUEUED_MSG_TIMEOUT	600	/* seconds */
/*
 * If no logon is completed within this time period from the connection
 * having been established, the connection is terminated.
 */
#define	LOGON_GRACE_TIME	30	/* seconds */

#define	AF2ADDRLEN(sa_family) \
	((sa_family) == AF_INET ? sizeof (struct sockaddr_in) : \
	    sizeof (struct sockaddr_in6))

/*
 * Checks whether the appropriate connections list mutex is held for
 * a particular connection. For TCP connections, we want to be holding
 * `conns_tcp_lock', whereas for libwebsocket connections, we want to
 * be holding `conns_lws_lock'.
 */
#define	ASSERT_CONNS_MUTEX_HELD(conn) \
	do { \
		if ((conn)->is_lws) \
			ASSERT_MUTEX_HELD(&conns_lws_lock); \
		else \
			ASSERT_MUTEX_HELD(&conns_tcp_lock); \
	} while (0)

/*
 * Logon status enum:
 * - LOGON_NONE: initial state when new connection is received and its TLS
 *	handshake has completed.
 * - LOGON_STARTED: client has sent a LOGON message and we have initiated
 *	a background auth session to validate the logon.
 * - LOGON_COMPLETING: background auth session has completed and its results
 *	have been recorded in the connection structure. The main thread
 *	needs to send the corresponding result to the client.
 * - LOGON_COMPLETE: successfully logged on and the connection has assumed
 *	its intended logon identity. If the logon fails, the logon status
 *	reverts back to LOGON_NONE instead.
 */
typedef enum {
	LOGON_NONE,
	LOGON_STARTED,
	LOGON_COMPLETING,
	LOGON_COMPLETE
} logon_status_t;

typedef struct {
	char	ident[CALLSIGN_LEN];
	list_t	node;
} ident_list_t;

/*
 * Master connection tracking structure. This structure holds all the state
 * associated with a client connection. It is held in the `conns_tcp' and
 * `conns_lws' lists. If the client has completed a logon, it can also be
 * referenced from the `conns_by_from' hash table.
 */
typedef struct {
	/* immutable once set */
	bool			is_lws;
	uint64_t		outbuf_pre_pad;

	struct lws		*wsi;
	bool			kill_wsi;

	/* only set & read from main thread */
	struct sockaddr_storage	sockaddr;
	char			addr_str[SOCKADDR_STRLEN];
	int			fd;
	time_t			logoff_time;

	gnutls_session_t	session;
	bool			tls_handshake_complete;

	mutex_t			lock;

	/* protected by `lock' */
	logon_status_t		logon_status;
	/* Actual identity after a successful LOGON */
	list_t			from_list;
	char			to[CALLSIGN_LEN];
	/* Identity requested by a LOGON message */
	char			logon_from[CALLSIGN_LEN];
	char			logon_to[CALLSIGN_LEN];
	bool			logon_success;
	/* MIN value of LOGON message */
	unsigned		logon_min;
	auth_sess_key_t		auth_key;
	bool			is_atc;
	/* Data received over the TLS/WS connection */
	uint8_t			*inbuf;
	size_t			inbuf_sz;
	/* Data about to be sent to the client over the TLS/WS connection */
	uint8_t			*outbuf;
	size_t			outbuf_sz;

	list_node_t		conns_node;
} conn_t;

/*
 * An encoded message waiting in the delivery queue.
 */
typedef struct {
	char		from[CALLSIGN_LEN];
	char		to[CALLSIGN_LEN];
	bool		is_atc;
	time_t		created;	/* when the msg entered the queue */
	char		*msg;		/* message contents */
	list_node_t	queued_msgs_node;
} queued_msg_t;

/*
 * Structure holding all information about a socket on which we listen
 * for new incoming connections. This structure is held in the
 * `listen_socks' list.
 */
typedef struct {
	struct sockaddr_storage	sockaddr;
	int			fd;
	list_node_t		listen_socks_node;
} listen_sock_t;

typedef struct {
	bool			is_lws;
	struct lws_context	*ctx;
	bool			shutdown;
	thread_t		worker;
	list_node_t		listen_lws_node;
} listen_lws_t;

/*
 * Master connections lists. All conn_t's are gathered and primarily
 * held in one of two lists. `conns_tcp' collects connections over raw
 * TLS over TCP. `conns_lws' collects connections over libwebsocket.
 * Each list has its corresponding manipulation lock.
 */
static mutex_t		conns_tcp_lock;
static list_t		conns_tcp;

static mutex_t		conns_lws_lock;
static list_t		conns_lws;
/*
 * If modifications to `conns_tcp' are done, we need to raise this flag.
 * This is because TCP input handling requires constructing a pollfd list
 * that then needs to be back-correlated to the list of connections in
 * `conns_tcp' exactly. In case `conns_tcp' is modified while polling, we
 * need to throw away the poll result, reconstruct the pollfd list and
 * try the poll operation again.
 */
static bool		conns_tcp_dirty = false;
/*
 * Hash table mapping station "FROM" identities to one or more connections.
 * This mapping is established after a successful LOGON.
 */
static mutex_t		conns_by_from_lock;
static htbl_t		conns_by_from;
static bool		conns_by_from_changed = true;
/*
 * Bitshift defining the size of the `conns_by_from' hash table.
 * The default value of `12' defines a hash table with 4096 entries in it.
 */
#define	CONNS_BY_FROM_SHIFT	12
/*
 * Master lists of listening ends. `listen_socks' is for TCP sockets,
 * `listen_lws' is for WebSockets.
 */
static list_t		listen_socks;
static list_t		listen_lws;

/*
 * Since the main thread can be sitting in poll(), we need an I/O-based
 * method of waking it up. We do so by poll()ing on the read side of a
 * pipe. If another thread needs the main thread's attention, it simply
 * needs to call wake_main_thread(). This then writes a single byte into
 * the write end of the pipe. If the main thread was sitting in poll(),
 * it will immediately wake up and start processing. If it was doing
 * something else, it will immediately exit from poll() on the next call.
 * As part of standard poll processing, the main thread drains the wakeup
 * pipe of any written wakeup calls.
 */
static int		poll_wakeup_pipe[2] = { -1, -1 };

/*
 * TLS configuration parameters.
 */
static char		tls_keyfile[PATH_MAX] = { 0 };
static char		tls_certfile[PATH_MAX] = { 0 };
static char		tls_cafile[PATH_MAX] = { 0 };
static char		tls_crlfile[PATH_MAX] = { 0 };
static char		tls_keyfile_pass[PATH_MAX] = { 0 };
static gnutls_pkcs_encrypt_flags_t tls_keyfile_enctype = GNUTLS_PKCS_PLAIN;
/*
 * Global TLS state.
 */
static gnutls_certificate_credentials_t	x509_creds;
static gnutls_priority_t		prio_cache;

/*
 * List of messages queued for later delivery (recipient currently not
 * connected). A list of queued_msg_t's. The list is processed at regular
 * intervals to expunge timed out messages.
 */
static list_t		queued_msgs;
/* Current amount of bytes consumed by messages in `queued_msgs' */
static uint64_t		queued_msg_bytes = 0;
/* Maximum size that `queued_msgs' can grow to. */
static uint64_t		queued_msg_max_bytes = 128 << 20;	/* 128 MiB */
/*
 * Global server config parameters. Can be overridden from config file.
 */
static bool		background = true;
static bool		do_shutdown = false;
static bool		req_client_cert = false;
static mutex_t		msg_log_lock;
static char		msg_log_filename[PATH_MAX] = {};
static FILE		*msg_log_file = NULL;
static char		logon_list_file[PATH_MAX] = {};
static char		*logon_cmd = NULL;
static char		*logoff_cmd = NULL;

static void lws_worker(void *userinfo);
static int http_lws_cb(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len);
static int cpdlc_lws_cb(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len);

static struct lws_protocols proto_list_lws[] =
{
    /* The first protocol must always be the HTTP handler */
    {
	.name = "http-only",
	.callback = http_lws_cb,
	.per_session_data_size = 0,
	.rx_buffer_size = 0
    },
    {
	.name = "cpdlc",
	.callback = cpdlc_lws_cb,
	.per_session_data_size = sizeof (conn_t),
	.rx_buffer_size = MAX_BUF_SZ
    },
    { .name = NULL }	/* list terminator */
};

static void send_error_msg_v(conn_t *conn, const cpdlc_msg_t *orig_msg,
    const char *fmt, va_list ap);
static void send_error_msg(conn_t *conn, const cpdlc_msg_t *orig_msg,
    const char *fmt, ...);
static void send_error_msg_to(const char *to, const cpdlc_msg_t *orig_msg,
    const char *fmt, ...);
static void send_svc_unavail_msg(conn_t *conn, unsigned orig_min);
static void close_conn(conn_t *conn);
static void conn_send_msg(conn_t *conn, const cpdlc_msg_t *msg);

static char *
str_subst(char *str, const char *pattern, const char *replace)
{
	char *result = NULL;
	size_t sz = 0;
	char *srch, *hit;

	ASSERT(str != NULL);
	ASSERT(pattern != NULL);
	ASSERT(replace != NULL);

	for (srch = str, hit = strstr(srch, pattern);
	    srch < str + strlen(str) && hit != NULL;
	    srch = hit + strlen(pattern), hit = strstr(srch, pattern)) {
		char buf[hit - srch + 1];

		strlcpy(buf, srch, hit - srch + 1);
		append_format(&result, &sz, "%s%s", buf, replace);
	}
	append_format(&result, &sz, "%s", srch);
	free(str);

	return (result);
}

static void
exec_cmd_common(const char *cmd_in, const char *from, const conn_t *conn)
{
	const char *to;
	char *cmd;

	ASSERT(cmd_in != NULL);
	ASSERT(from != NULL);
	ASSERT(conn != NULL);
	cmd = strdup(cmd_in);
	to = (conn->to[0] != '\0' ? conn->to : "-");

	cmd = str_subst(cmd, "${FROM}", from);
	cmd = str_subst(cmd, "${TO}", to);
	cmd = str_subst(cmd, "${ADDR}", conn->addr_str);
	cmd = str_subst(cmd, "${STATYPE}", conn->is_atc ? "ATC" : "ACFT");
	cmd = str_subst(cmd, "${CONNTYPE}", conn->is_lws ? "WS" : "TLS");
	switch (fork()) {
	case -1:
		logMsg("fork() failed: %s", strerror(errno));
		break;
	case 0:
		execl("/bin/sh", "sh", "-c", cmd, NULL);
		logMsg("execl(\"/bin/sh\") failed: %s", strerror(errno));
		exit(EXIT_FAILURE);
	default:
		break;
	}
	free(cmd);
}

static void
exec_logon_cmd(const char *from, const conn_t *conn)
{
	if (logon_cmd != NULL)
		exec_cmd_common(logon_cmd, from, conn);
}

static void
exec_logoff_cmd(const char *from, const conn_t *conn)
{
	if (logoff_cmd != NULL)
		exec_cmd_common(logoff_cmd, from, conn);
}

/*
 * Writes a single byte into the main thread wakeup pipe. This forces
 * the main thread to wake up and perform its other regular tasks.
 */
static void
wake_up_main_thread(void)
{
	uint8_t buf[1];
	(void) write(poll_wakeup_pipe[1], buf, sizeof (buf));
}

/*
 * Sets a file descriptor to non-blocking mode.
 */
static bool
set_fd_nonblock(int fd)
{
	int flags;

	return ((flags = fcntl(fd, F_GETFL)) >= 0 &&
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0);
}

static void
sockaddr2str(const struct sockaddr_storage *ss, char str[SOCKADDR_STRLEN])
{
	ASSERT(ss != NULL);
	ASSERT(str != NULL);

	if (ss->ss_family == AF_INET) {
		char addr[SOCKADDR_STRLEN] = { 0 };
		struct sockaddr_in *sa = (struct sockaddr_in *)ss;

		VERIFY_MSG(inet_ntop(ss->ss_family, &sa->sin_addr, addr,
		    sizeof (addr)) != NULL,
		    "Cannot convert sockaddr to string: %s", strerror(errno));
		if (sa->sin_port != 0) {
			snprintf(str, SOCKADDR_STRLEN, "%s:%d", addr,
			    ntohs(sa->sin_port));
		} else {
			cpdlc_strlcpy(str, addr, SOCKADDR_STRLEN);
		}
	} else {
		char addr[SOCKADDR_STRLEN] = { 0 };
		struct sockaddr_in6 *sa = (struct sockaddr_in6 *)ss;

		VERIFY_MSG(inet_ntop(ss->ss_family, &sa->sin6_addr, addr,
		    sizeof (addr)) != NULL,
		    "Cannot convert sockaddr to string: %s", strerror(errno));
		if (sa->sin6_port != 0) {
			snprintf(str, SOCKADDR_STRLEN, "[%s]:%d", addr,
			    ntohs(sa->sin6_port));
		} else {
			snprintf(str, SOCKADDR_STRLEN, "[%s]", addr);
		}
	}
}

/*
 * Initializes our global data structures.
 */
static void
init_structs(void)
{
	ASSERT(CONNS_BY_FROM_SHIFT > 0);
	mutex_init(&conns_tcp_lock);
	mutex_init(&msg_log_lock);
	list_create(&conns_tcp, sizeof (conn_t), offsetof(conn_t, conns_node));
	mutex_init(&conns_lws_lock);
	list_create(&conns_lws, sizeof (conn_t), offsetof(conn_t, conns_node));
	mutex_init(&conns_by_from_lock);
	htbl_create(&conns_by_from, 1 << CONNS_BY_FROM_SHIFT, CALLSIGN_LEN,
	    true);
	list_create(&queued_msgs, sizeof (queued_msg_t),
	    offsetof(queued_msg_t, queued_msgs_node));
	list_create(&listen_socks, sizeof (listen_sock_t),
	    offsetof(listen_sock_t, listen_socks_node));
	list_create(&listen_lws, sizeof (listen_lws_t),
	    offsetof(listen_lws_t, listen_lws_node));
	blocklist_init();
	VERIFY_MSG(pipe(poll_wakeup_pipe) != -1, "pipe() failed: %s",
	    strerror(errno));
	set_fd_nonblock(poll_wakeup_pipe[0]);
	set_fd_nonblock(poll_wakeup_pipe[1]);
}

/*
 * Destroys and cleans up our global data structures.
 */
static void
fini_structs(void)
{
	conn_t *conn;
	queued_msg_t *msg;
	listen_sock_t *ls;
	listen_lws_t *lws;

	htbl_empty(&conns_by_from, NULL, NULL);
	htbl_destroy(&conns_by_from);
	mutex_destroy(&conns_by_from_lock);

	mutex_enter(&conns_tcp_lock);
	/* calling `close_conn' removes the connection from `conns_tcp' */
	while ((conn = list_head(&conns_tcp)) != NULL) {
		ASSERT(!conn->is_lws);
		close_conn(conn);
	}
	mutex_exit(&conns_tcp_lock);
	list_destroy(&conns_tcp);
	mutex_destroy(&conns_tcp_lock);

	/* Destroying the LWS contexts should have drained this list */
	ASSERT0(list_count(&conns_lws));
	list_destroy(&conns_lws);
	mutex_destroy(&conns_lws_lock);

	while ((msg = list_remove_head(&queued_msgs)) != NULL) {
		free(msg->msg);
		free(msg);
	}
	list_destroy(&queued_msgs);
	queued_msg_bytes = 0;

	while ((ls = list_remove_head(&listen_socks)) != NULL) {
		if (ls->fd != -1)
			close(ls->fd);
		free(ls);
	}
	list_destroy(&listen_socks);

	/*
	 * First mark all LWS contexts for destruction, then join all the
	 * worker threads.
	 */
	for (lws = list_head(&listen_lws); lws != NULL;
	    lws = list_next(&listen_lws, lws)) {
		lws->shutdown = true;
	}
	while ((lws = list_remove_head(&listen_lws)) != NULL) {
		thread_join(&lws->worker);
		free(lws);
	}
	list_destroy(&listen_lws);

	blocklist_fini();

	close(poll_wakeup_pipe[0]);
	close(poll_wakeup_pipe[1]);
}

static void
print_usage(const char *progname, FILE *fp)
{
	fprintf(fp, "Usage: %s [-hd] [-c <conffile>]\n", progname);
}

static bool
add_listen_sock_lws(const char *iface, int port, const char *name_port)
{
	struct lws_context_creation_info info;
	listen_lws_t *lws = safe_calloc(1, sizeof (*lws));

	ASSERT(iface != NULL);
	ASSERT3S(port, >, 0);
	ASSERT3S(port, <, UINT16_MAX);

	memset(&info, 0, sizeof(info));

	info.port = port;
	info.protocols = proto_list_lws;
	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	if (strcmp(iface, "loopback") == 0) {
#if	APL || SUN
		info.iface = "lo0";
#elif	LIN
		info.iface = "lo";
#else
#error	"Unsupported platform"
#endif
	} else if (strcmp(iface, "*") != 0) {
		info.iface = iface;
	}

	info.gid = -1;
	info.uid = -1;

	info.ssl_private_key_filepath = tls_keyfile;
	if (tls_keyfile_enctype != GNUTLS_PKCS_PLAIN)
		info.ssl_private_key_password = tls_keyfile_pass;
	info.ssl_cert_filepath = tls_certfile;
	if (strlen(tls_cafile) != 0)
		info.ssl_ca_filepath = tls_cafile;

	lws->ctx = lws_create_context(&info);
	if (lws->ctx == NULL) {
		logMsg("Error creating LWS context for %s", name_port);
		free(lws);
		return (false);
	}
	VERIFY(thread_create(&lws->worker, lws_worker, lws));

	return (true);
}

static bool
add_listen_sock_tcp(const char *hostname, int port, const char *name_port)
{
	struct addrinfo *ai_full = NULL;
	char portbuf[8];
	int error;
	struct addrinfo hints = {
	    .ai_family = AF_UNSPEC,
	    .ai_socktype = SOCK_STREAM,
	    .ai_protocol = IPPROTO_TCP
	};

	ASSERT(hostname != NULL);
	ASSERT3S(port, >, 0);
	ASSERT3S(port, <, UINT16_MAX);

	snprintf(portbuf, sizeof (portbuf), "%d", port);

	if (strcmp(hostname, "*") == 0) {
		/* Allow listening on the wildcard hostname */
		hints.ai_flags = AI_PASSIVE;
		error = getaddrinfo(NULL, portbuf, &hints, &ai_full);
	} else if (strcmp(hostname, "localhost") == 0) {
		/* Grab the appropriate loopback address */
		error = getaddrinfo(NULL, portbuf, &hints, &ai_full);
	} else {
		/* Do the hostname lookup properly */
		error = getaddrinfo(hostname, portbuf, &hints, &ai_full);
	}
	if (error != 0) {
		logMsg("Invalid listen directive \"%s\": %s", name_port,
		    gai_strerror(error));
		return (false);
	}

	for (const struct addrinfo *ai = ai_full; ai != NULL;
	    ai = ai->ai_next) {
		listen_sock_t *ls;
		unsigned int one = 1;

		ASSERT3U(ai->ai_protocol, ==, IPPROTO_TCP);
		ls = safe_calloc(1, sizeof (*ls));
		ASSERT3U(ai->ai_addrlen, <=, sizeof (ls->sockaddr));
		memcpy(&ls->sockaddr, ai->ai_addr, ai->ai_addrlen);

		list_insert_tail(&listen_socks, ls);

		ls->fd = socket(ai->ai_family, ai->ai_socktype,
		    ai->ai_protocol);
		if (ls->fd == -1) {
			logMsg("Invalid listen directive \"%s\": cannot "
			    "create socket: %s", name_port, strerror(errno));
			goto errout;
		}
		setsockopt(ls->fd, SOL_SOCKET, SO_REUSEADDR, &one,
		    sizeof (one));
		if (bind(ls->fd, ai->ai_addr, ai->ai_addrlen) == -1) {
			logMsg("Invalid listen directive \"%s\": cannot bind "
			    "socket: %s", name_port, strerror(errno));
			goto errout;
		}
		if (listen(ls->fd, CONN_BACKLOG) == -1) {
			logMsg("Invalid listen directive \"%s\": cannot "
			    "listen on socket: %s", name_port, strerror(errno));
			goto errout;
		}
		if (!set_fd_nonblock(ls->fd)) {
			logMsg("Invalid listen directive \"%s\": cannot set "
			    "socket as non-blocking: %s", name_port,
			    strerror(errno));
			goto errout;
		}
	}

	freeaddrinfo(ai_full);
	return (true);
errout:
	if (ai_full != NULL)
		freeaddrinfo(ai_full);
	return (false);
}

/*
 * Adds a listen socket to the server's list of incoming sockets.
 * @param name_port String specifying the "hostname:port" combo to listen on.
 * @return true if the socket was added successfully, false on error.
 *	The error reason is printed to the log.
 */
static bool
add_listen_sock(const char *name_port, bool lws)
{
	char hostname[CPDLC_HOSTNAME_MAX_LEN] = { 0 };
	int port;

	if (!cpdlc_config_str2hostname_port(name_port, hostname, &port, lws)) {
		logMsg("Invalid listen directive \"%s\": expected valid port "
		    "number following last ':' character", name_port);
		return (false);
	}
	if (lws)
		return (add_listen_sock_lws(hostname, port, name_port));
	else
		return (add_listen_sock_tcp(hostname, port, name_port));
}

/*
 * Parses a numerical byte-count specification with an optional multiplier
 * suffix and returns the raw number of bytes. For example, '128k' returns
 * 131072, '2g' returns 2147483648, etc.
 */
static uint64_t
parse_bytes(const char *str)
{
	uint64_t value;
	const char *mult = "kmgtep";
	char c;

	ASSERT(str != NULL);
	ASSERT(strlen(str) != 0);
	value = atoll(str);
	c = tolower(str[strlen(str) - 1]);
	for (int i = 0, n = strlen(mult); i < n; i++) {
		if (c == mult[i]) {
			value <<= (i + 1) * 10;
			break;
		}
	}

	return (value);
}

static bool
msglog_reopen(void)
{
	if (msg_log_filename[0] != '\0') {
		FILE *fp = fopen(msg_log_filename, "a");

		if (fp == NULL) {
			logMsg("Can't open msglog %s: %s", msg_log_filename,
			    strerror(errno));
			return (false);;
		}
		mutex_enter(&msg_log_lock);
		if (msg_log_file != NULL)
			fclose(msg_log_file);
		msg_log_file = fp;
		mutex_exit(&msg_log_lock);
	} else {
		mutex_enter(&msg_log_lock);
		if (msg_log_file != NULL) {
			fclose(msg_log_file);
			msg_log_file = NULL;
		}
		mutex_exit(&msg_log_lock);
	}
	return (true);
}

/*
 * Parses the server's configuration file. The config file is arranged
 * as a sequence of "key = value" pairs, using the config file syntax
 * of libacfutils' conf.h class.
 *
 * @param conf_path Config file path.
 *
 * @return true if the config file was parsed successfully, false on error.
 *	The error reason is printed to the log.
 */
static bool
parse_config(const char *conf_path)
{
	int errline;
	uint64_t msgquota_max = 0;
	conf_t *conf = conf_read_file(conf_path, &errline);
	const char *key, *value;
	void *cookie;
	const char *auth_url = NULL, *cainfo = NULL;
	const char *auth_username = NULL, *auth_password = NULL;

	if (conf == NULL) {
		if (errline == -1)
			logMsg("Can't open %s: %s", conf_path, strerror(errno));
		else
			logMsg("%s: parsing error on %d", conf_path, errline);
		return (false);
	}
	if (conf_get_str(conf, "tls/keyfile", &value))
		lacf_strlcpy(tls_keyfile, value, sizeof (tls_keyfile));
	if (conf_get_str(conf, "tls/keyfile_pass", &value)) {
		lacf_strlcpy(tls_keyfile_pass, value,
		    sizeof (tls_keyfile_pass));
		if (tls_keyfile_enctype == GNUTLS_PKCS_PLAIN)
			tls_keyfile_enctype = GNUTLS_PKCS_PBES2_AES_256;
	}
	if (conf_get_str(conf, "tls/keyfile_enctype", &value)) {
		tls_keyfile_enctype = cpdlc_config_str2encflags(value);
		if (tls_keyfile_enctype == GNUTLS_PKCS_PLAIN) {
			logMsg("Unsupported value for tls_keyfile_enctype "
			    "(%s). Must be one of: \"3DES\", \"RC4\", "
			    "\"AES128\", \"AES192\", \"AES256\" or "
			    "\"PKCS12/3DES\".", value);
			goto errout;
		}
	}
	if (conf_get_str(conf, "tls/certfile", &value))
		lacf_strlcpy(tls_certfile, value, sizeof (tls_certfile));
	if (conf_get_str(conf, "tls/cafile", &value))
		lacf_strlcpy(tls_cafile, value, sizeof (tls_cafile));
	if (conf_get_str(conf, "tls/crlfile", &value))
		lacf_strlcpy(tls_crlfile, value, sizeof (tls_crlfile));
	conf_get_b(conf, "tls/req_client_cert", (bool_t *)&req_client_cert);
	if (conf_get_str(conf, "blocklist", &value))
		blocklist_set_filename(value);
	if (conf_get_str(conf, "auth/url", &value))
		auth_url = value;
	if (conf_get_str(conf, "cainfo", &value))
		cainfo = value;
	if (conf_get_str(conf, "auth/username", &value))
		auth_username = value;
	if (conf_get_str(conf, "auth/password", &value))
		auth_password = value;
	if (conf_get_str(conf, "msgqueue/quota", &value))
		msgquota_max = parse_bytes(value);
	if (conf_get_str(conf, "msgqueue/max", &value))
		queued_msg_max_bytes = parse_bytes(value);
	if (conf_get_str(conf, "msglog", &value)) {
		cpdlc_strlcpy(msg_log_filename, value,
		    sizeof (msg_log_filename));
		if (!msglog_reopen())
			goto errout;
	} else {
		msglog_reopen();
	}
	if (conf_get_str(conf, "logon_list_file", &value))
		strlcpy(logon_list_file, value, sizeof (logon_list_file));
	if (conf_get_str(conf, "logon_cmd", &value))
		logon_cmd = strdup(value);
	if (conf_get_str(conf, "logoff_cmd", &value))
		logoff_cmd = strdup(value);

	/*
	 * Must go after all TLS parameters have been parsed, because
	 * LWS connections can request them immediately.
	 */
	cookie = NULL;
	while (conf_walk(conf, &key, &value, &cookie)) {
		if (strncmp(key, "listen/tcp/", 11) == 0) {
			if (!add_listen_sock(value, false))
				goto errout;
		} else if (strncmp(key, "listen/lws/", 11) == 0) {
			if (!add_listen_sock(value, true))
				goto errout;
		}
	}

	if (list_count(&listen_socks) == 0 &&
	    (!add_listen_sock("localhost", false) ||
	    !add_listen_sock("loopback", true))) {
		goto errout;
	}
	auth_init(auth_url, cainfo, auth_username, auth_password);
	msgquota_init(msgquota_max);
	if (!msg_router_init(conf))
		goto errout;

	conf_free(conf);
	return (true);
errout:
	conf_free(conf);
	return (false);
}

/*
 * In the absence of a config file, performs minimal server auto-configuration.
 *
 * @return true on success, false on error.
 */
static bool
auto_config(void)
{
	auth_init(NULL, NULL, NULL, NULL);
	VERIFY(msg_router_init(NULL));
	msgquota_init(0);
	return (add_listen_sock("localhost", false));
}

/*
 * Puts the daemon into the background (forks and shuts down the foreground
 * copy).
 *
 * @param do_chdir If true, the daemon chdir()s to "/" after forking.
 * @param do_close If true, the daemon closes its stdin to decouple from
 *	an attached terminals.
 *
 * @return true on success, false on failure (error reason is printed to
 *	the log).
 */
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
		VERIFY0(chdir("/"));
	if (do_close) {
		close(STDIN_FILENO);
		if (open("/dev/null", O_RDONLY) < 0) {
			perror("Cannot replace STDIN with /dev/null");
			return (false);
		}
	}

	return (true);
}

/*
 * Handles an new incoming connections on a listen socket. Connections
 * are accepted until there are no more pending connections. The function
 * then returns.
 *
 * @param ls Listen socket on which to accept connections.
 */
static void
handle_accepts(listen_sock_t *ls)
{
	for (;;) {
		conn_t *conn = safe_calloc(1, sizeof (*conn));
		socklen_t addr_len = sizeof (conn->sockaddr);

		conn->fd = accept(ls->fd, (struct sockaddr *)&conn->sockaddr,
		    &addr_len);
		if (conn->fd == -1) {
			free(conn);
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				/* No more pending connections, we're done. */
				break;
			}
			/* Genuine accept() error */
			logMsg("Error accepting connection: %s",
			    strerror(errno));
			continue;
		}
		ASSERT(conn->sockaddr.ss_family == AF_INET ||
		    conn->sockaddr.ss_family == AF_INET6);
		sockaddr2str(&conn->sockaddr, conn->addr_str);
		/*
		 * Interrogate the blocklist as early as possible, so we're
		 * not wasting any resources on blocked hosts.
		 */
		if (!blocklist_check(&conn->sockaddr)) {
			logMsg("Incoming connection blocked: "
			    "address %s on blocklist.", conn->addr_str);
			close(conn->fd);
			free(conn);
			continue;
		}
		/*
		 * Set the socket as non-blocking and check for duplicate
		 * connections (this would indicate a kernel bug, really).
		 */
		set_fd_nonblock(conn->fd);
		conn->logoff_time = time(NULL);
		/*
		 * Start the TLS handshake process.
		 */
		VERIFY0(gnutls_init(&conn->session,
		    GNUTLS_SERVER | GNUTLS_NONBLOCK | GNUTLS_NO_SIGNAL));
		VERIFY0(gnutls_priority_set(conn->session, prio_cache));
		VERIFY0(gnutls_credentials_set(conn->session,
		    GNUTLS_CRD_CERTIFICATE, x509_creds));
		/* If client certs are required, request one. */
		gnutls_certificate_server_set_request(conn->session,
		    req_client_cert ? GNUTLS_CERT_REQUIRE : GNUTLS_CERT_IGNORE);
		gnutls_handshake_set_timeout(conn->session,
		    GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
		gnutls_transport_set_int(conn->session, conn->fd);

		mutex_init(&conn->lock);
		list_create(&conn->from_list, sizeof (ident_list_t),
		    offsetof(ident_list_t, node));

		mutex_enter(&conns_tcp_lock);
		list_insert_tail(&conns_tcp, conn);
		conns_tcp_dirty = true;
		mutex_exit(&conns_tcp_lock);
	}
}

static void
conns_by_from_remove(conn_t *conn, const char ident[CALLSIGN_LEN])
{
	const list_t *l;

	mutex_enter(&conns_by_from_lock);

	l = htbl_lookup_multi(&conns_by_from, ident);
	ASSERT(l != NULL);
	for (void *mv = list_head(l); mv != NULL; mv = list_next(l, mv)) {
		conn_t *c = HTBL_VALUE_MULTI(mv);

		if (conn == c) {
			exec_logoff_cmd(ident, c);
			htbl_remove_multi(&conns_by_from, ident, mv);
			conns_by_from_changed = true;
			break;
		}
	}

	mutex_exit(&conns_by_from_lock);
}

/*
 * Resets the logon status of a connection. This de-associates the
 * connection from its "FROM" identity and prevents any further message
 * submissions without a new logon.
 */
static void
conn_reset_logon(conn_t *conn)
{
	ident_list_t *idl;

	ASSERT(conn != NULL);

	mutex_enter(&conn->lock);
	/*
	 * If the logon has completed, we MUST have had this connection
	 * inserted in the conns_by_from hashtable.
	 */
	while ((idl = list_remove_head(&conn->from_list)) != NULL) {
		conns_by_from_remove(conn, idl->ident);
		free(idl);
	}
	/*
	 * If a background auth session has been started, kill it.
	 */
	if (conn->logon_status == LOGON_STARTED) {
		auth_sess_kill(conn->auth_key);
		goto out;
	}
	if (conn->logon_status != LOGON_COMPLETE)
		goto out;
	conn->is_atc = false;
	memset(conn->to, 0, sizeof (conn->to));
	memset(conn->logon_from, 0, sizeof (conn->logon_from));
	memset(conn->logon_to, 0, sizeof (conn->logon_to));
out:
	conn->logon_status = LOGON_NONE;
	conn->logon_success = false;

	mutex_exit(&conn->lock);
}

/*
 * Closes a network connection and kills all associated state with it.
 */
static void
close_conn(conn_t *conn)
{
	ASSERT(conn != NULL);
	ASSERT_CONNS_MUTEX_HELD(conn);

	/*
	 * Must be done before unlinking the connection from any list,
	 * because we can be called in the background to complete a logon.
	 */
	conn_reset_logon(conn);

	if (conn->is_lws) {
		list_remove(&conns_lws, conn);
	} else {
		list_remove(&conns_tcp, conn);
		conns_tcp_dirty = true;
	}

	mutex_destroy(&conn->lock);
	list_destroy(&conn->from_list);
	free(conn->inbuf);
	free(conn->outbuf);

	if (!conn->is_lws) {
		if (conn->tls_handshake_complete)
			gnutls_bye(conn->session, GNUTLS_SHUT_WR);
		ASSERT(conn->fd != -1);
		close(conn->fd);
		gnutls_deinit(conn->session);

		memset(conn, 0, sizeof (*conn));
		free(conn);
	}
}

/*
 * Asynchronous logon authentication completion callback. This is called
 * by the background authenticator to confirm if a particular connection
 * is authenticated or not.
 */
static void
logon_done_cb(bool result, bool is_atc, void *userinfo)
{
	conn_t *conn;

	ASSERT(userinfo != NULL);
	conn = userinfo;
	/*
	 * Simply record the results and wake up the main thread.
	 */
	mutex_enter(&conn->lock);
	ASSERT3U(conn->logon_status, ==, LOGON_STARTED);
	conn->logon_status = LOGON_COMPLETING;
	conn->logon_success = result;
	conn->is_atc = is_atc;
	mutex_exit(&conn->lock);

	wake_up_main_thread();
}

/*
 * Main thread processing done in response to a completed login.
 * This is invoked after logon_done_cb has given us the results
 * of the authentication callback.
 */
static void
complete_logon(conn_t *conn)
{
	cpdlc_msg_t *msg;

	ASSERT(conn);
	ASSERT_CONNS_MUTEX_HELD(conn);

	mutex_enter(&conn->lock);

	if (conn->logon_status != LOGON_COMPLETING) {
		mutex_exit(&conn->lock);
		return;
	}
	msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
	cpdlc_msg_set_mrn(msg, conn->logon_min);
	if (conn->logon_success && !conn->is_atc &&
	    conn->logon_to[0] == '\0') {
		conn->logon_status = LOGON_NONE;
		cpdlc_msg_add_seg(msg, false, CPDLC_UM159_ERROR_description, 0);
		cpdlc_msg_seg_set_arg(msg, 0, 0, "LOGON REQUIRES TO= HEADER",
		    NULL);
	} else if (conn->logon_success) {
		ident_list_t *idl = safe_calloc(1, sizeof (*idl));

		conn->logon_status = LOGON_COMPLETE;

		lacf_strlcpy(conn->to, conn->logon_to, sizeof (conn->to));
		lacf_strlcpy(idl->ident, conn->logon_from, sizeof (idl->ident));
		list_insert_tail(&conn->from_list, idl);

		mutex_enter(&conns_by_from_lock);
		htbl_set(&conns_by_from, idl->ident, conn);
		exec_logon_cmd(idl->ident, conn);
		conns_by_from_changed = true;
		mutex_exit(&conns_by_from_lock);

		cpdlc_msg_set_logon_data(msg, "SUCCESS");
		cpdlc_msg_set_from(msg, "ATN");
	} else {
		conn->logon_status = LOGON_NONE;

		cpdlc_msg_set_logon_data(msg, "FAILURE");
		cpdlc_msg_set_from(msg, "ATN");
	}

	memset(conn->logon_to, 0, sizeof (conn->logon_to));
	memset(conn->logon_from, 0, sizeof (conn->logon_from));

	conn_send_msg(conn, msg);
	cpdlc_msg_free(msg);

	mutex_exit(&conn->lock);
}

/*
 * Runs through all connections and any that have pending logon
 * authentication results will send the required response to the client.
 */
static void
complete_logons(void)
{
	mutex_enter(&conns_tcp_lock);
	for (conn_t *conn = list_head(&conns_tcp); conn != NULL;
	    conn = list_next(&conns_tcp, conn)) {
		complete_logon(conn);
	}
	mutex_exit(&conns_tcp_lock);

	mutex_enter(&conns_lws_lock);
	for (conn_t *conn = list_head(&conns_lws); conn != NULL;
	    conn = list_next(&conns_lws, conn)) {
		complete_logon(conn);
	}
	mutex_exit(&conns_lws_lock);
}

static void
conn_remove_ident(conn_t *conn, const char *ident, bool missing_ok)
{
	ASSERT(conn != NULL);
	ASSERT(ident != NULL);
	for (ident_list_t *idl = list_head(&conn->from_list);
	    idl != NULL; idl = list_next(&conn->from_list, idl)) {
		if (strcmp(idl->ident, ident) == 0) {
			conns_by_from_remove(conn, idl->ident);
			list_remove(&conn->from_list, idl);
			free(idl);
			return;
		}
	}
	if (!missing_ok)
		VERIFY_FAIL();
}

/*
 * Processes an incoming LOGON message. When all conditions to continue
 * with the logon are met, this function fires off a background
 * authentication thread in auth.h to do the actual auth process.
 */
static void
process_logon_msg(conn_t *conn, const cpdlc_msg_t *msg)
{
	ASSERT(conn != NULL);
	ASSERT(msg != NULL);
	ASSERT_CONNS_MUTEX_HELD(conn);

	mutex_enter(&conn->lock);

	if (conn->logon_status == LOGON_STARTED ||
	    conn->logon_status == LOGON_COMPLETING) {
		mutex_exit(&conn->lock);
		send_error_msg(conn, msg, "LOGON ALREADY IN PROGRESS");
		return;
	}
	if (cpdlc_msg_get_from(msg) == NULL) {
		mutex_exit(&conn->lock);
		send_error_msg(conn, msg, "LOGON REQUIRES FROM= HEADER");
		return;
	}
	/* Clear any previous logon on non-ATC connections */
	if (!conn->is_atc) {
		conn_reset_logon(conn);
	} else {
		/*
		 * For ATC connections, just remove the identity we're trying
		 * to remove.
		 */
		conn_remove_ident(conn, cpdlc_msg_get_from(msg), true);
	}
	if (list_count(&conn->from_list) == 0) {
		conn->logoff_time = time(NULL);
		conn->logon_status = LOGON_NONE;
		conn->logon_success = false;
		conn->is_atc = false;
	}
	if (msg->is_logoff) {
		mutex_exit(&conn->lock);
		return;
	}
	if (cpdlc_msg_get_to(msg) != NULL) {
		lacf_strlcpy(conn->logon_to, cpdlc_msg_get_to(msg),
		    sizeof (conn->logon_to));
	}
	lacf_strlcpy(conn->logon_from, cpdlc_msg_get_from(msg),
	    sizeof (conn->logon_from));
	conn->logon_status = LOGON_STARTED;
	conn->logon_min = cpdlc_msg_get_min(msg);

	/* This is async */
	conn->auth_key = auth_sess_open(msg, &conn->sockaddr, logon_done_cb,
	    conn);
	/*
	 * Mustn't touch logon status after this, as logon_done_cb might
	 * have already been called (auth.c does this if it has no
	 * external authenticator configured).
	 */

	mutex_exit(&conn->lock);
}

static void
conn_log_buf(const char *addr_str, const char *buf, bool inout)
{
	time_t now;
	struct tm tm;
	char datebuf[64];

	ASSERT(addr_str != NULL);
	ASSERT(buf != NULL);

	mutex_enter(&msg_log_lock);

	if (msg_log_file == NULL) {
		mutex_exit(&msg_log_lock);
		return;
	}

	now = time(NULL);
	gmtime_r(&now, &tm);
	strftime(datebuf, sizeof (datebuf), "%Y%m%d_%H%M%SZ", &tm);
	fprintf(msg_log_file, "%s|%s|%s|%s", datebuf, addr_str,
	    inout ? "IN" : "OUT", buf);
	fflush(msg_log_file);

	mutex_exit(&msg_log_lock);
}

static void
conn_log_msg(const char *addr_str, const cpdlc_msg_t *msg, bool inout)
{
	char *buf;
	int l;
	cpdlc_msg_t *copymsg = NULL;

	ASSERT(addr_str != NULL);
	ASSERT(msg != NULL);
	/*
	 * LOGON messages are special, we don't to log the logon data, as
	 * that could dump user passwords into the log.
	 */
	if (msg->is_logon) {
		copymsg = cpdlc_msg_copy(msg);
		cpdlc_msg_set_logon_data(copymsg, "hidden");
		msg = copymsg;
	}

	l = cpdlc_msg_encode(msg, NULL, 0);
	buf = safe_malloc(l + 1);
	cpdlc_msg_encode(msg, buf, l + 1);
	conn_log_buf(addr_str, buf, inout);
	free(buf);

	if (copymsg != NULL)
		cpdlc_msg_free(copymsg);
}

/*
 * Prepares a new buffer for transmission to a particular connection.
 * The buffer is queued on the connections `outbuf'. This is later
 * processed by the master output functions.
 */
static void
conn_send_buf(conn_t *conn, const char *buf, size_t buflen)
{
	ASSERT(conn != NULL);
	ASSERT(buf != NULL);
	ASSERT(buflen != 0);

	mutex_enter(&conn->lock);

	conn->outbuf = safe_realloc(conn->outbuf, conn->outbuf_pre_pad +
	    conn->outbuf_sz + buflen + 1);
	lacf_strlcpy((char *)&conn->outbuf[conn->outbuf_pre_pad +
	    conn->outbuf_sz], buf, buflen + 1);
	/* Exclude training NUL char */
	conn->outbuf_sz += buflen;
	if (conn->is_lws) {
		ASSERT(conn->wsi != NULL);
		lws_callback_on_writable(conn->wsi);
	}

	mutex_exit(&conn->lock);
}

/*
 * Takes a message, encodes it into a sendable format and schedules it for
 * sending to a client. The caller retains ownership of the `msg' object.
 */
static void
conn_send_msg(conn_t *conn, const cpdlc_msg_t *msg)
{
	unsigned l;
	char *buf;

	ASSERT(conn != NULL);
	ASSERT(msg != NULL);

	l = cpdlc_msg_encode(msg, NULL, 0);
	buf = safe_malloc(l + 1);
	cpdlc_msg_encode(msg, buf, l + 1);
	conn_log_buf(conn->addr_str, buf, false);
	conn_send_buf(conn, buf, l);
	free(buf);

	/*
	 * Check if the message being sent is a service termination.
	 * In that case, terminate the connection's logon status.
	 */
	for (unsigned i = 0, n = msg->num_segs; i < n; i++) {
		ASSERT(msg->segs[i].info != NULL);
		if (!msg->segs[i].info->is_dl &&
		    msg->segs[i].info->msg_type == CPDLC_UM161_END_SVC) {
			conn_reset_logon(conn);
			conn->logoff_time = time(NULL);
		}
	}
}

/*
 * Generic error-response function for sending errors to clients.
 *
 * @param conn The connection over which to send the error.
 * @param orig_msg If not NULL, the error message will be formatted so as
 *	to respond to this message. If `orig_msg' was an uplink message,
 *	the error message code will be an uplink error message. Otherwise
 *	it will be a downlink error message. The error message will also
 *	have its MRN set appropriately to mark it as a response to
 *	`orig_msg'.
 * @param fmt A printf-style format string (+ arguments) for the free text
 *	details in the error message body.
 */
static void
send_error_msg_v(conn_t *conn, const cpdlc_msg_t *orig_msg, const char *fmt,
    va_list ap)
{
	int l;
	va_list ap2;
	char *buf;
	cpdlc_msg_t *msg;

	ASSERT(conn != NULL);
	ASSERT(fmt != NULL);

	va_copy(ap2, ap);
	l = vsnprintf(NULL, 0, fmt, ap2);
	va_end(ap2);

	buf = safe_malloc(l + 1);
	vsnprintf(buf, l + 1, fmt, ap);

	if (orig_msg != NULL) {
		msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
		cpdlc_msg_set_mrn(msg, cpdlc_msg_get_min(orig_msg));
		if (cpdlc_msg_get_dl(orig_msg)) {
			cpdlc_msg_add_seg(msg, false,
			    CPDLC_UM159_ERROR_description, 0);
		} else {
			cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM62_ERROR_errorinfo, 0);
		}
	} else {
		msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
		cpdlc_msg_add_seg(msg, false, CPDLC_UM159_ERROR_description, 0);
	}
	cpdlc_msg_seg_set_arg(msg, 0, 0, buf, NULL);

	conn_send_msg(conn, msg);

	free(buf);
	cpdlc_msg_free(msg);
}

static void
send_error_msg(conn_t *conn, const cpdlc_msg_t *orig_msg, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	send_error_msg_v(conn, orig_msg, fmt, ap);
	va_end(ap);
}

static void
send_error_msg_to(const char *to, const cpdlc_msg_t *orig_msg,
    const char *fmt, ...)
{
	const list_t *l;

	ASSERT(to != NULL);
	ASSERT(orig_msg != NULL);
	ASSERT(fmt != NULL);

	mutex_enter(&conns_by_from_lock);
	l = htbl_lookup_multi(&conns_by_from, to);
	if (l != NULL && list_count(l) != 0) {
		for (void *mv = list_head(l), *mv_next = NULL; mv != NULL;
		    mv = mv_next) {
			conn_t *conn = HTBL_VALUE_MULTI(mv);
			va_list ap;

			mv_next = list_next(l, mv);
			ASSERT(conn != NULL);
			va_start(ap, fmt);
			send_error_msg_v(conn, orig_msg, fmt, ap);
			va_end(ap);
		}
	}
	mutex_exit(&conns_by_from_lock);
}

/*
 * Sends a UM162 SERVICE UNAVAILABLE message into a connection.
 *
 * @param conn Connection on which to send the message.
 * @param orig_min Original message MIN that caused this error to occur.
 */
static void
send_svc_unavail_msg(conn_t *conn, unsigned orig_min)
{
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
	ASSERT(conn != NULL);
	cpdlc_msg_set_mrn(msg, orig_min);
	cpdlc_msg_add_seg(msg, false, CPDLC_UM162_SVC_UNAVAIL, 0);
	conn_send_msg(conn, msg);
	cpdlc_msg_free(msg);
}

/*
 * Stores a message for later delivery. The message is accounted for
 * in the global memory and individual message quota trackers.
 *
 * @param msg The message to store. The message is stored in encoded
 *	form, so the caller retains ownership of the `msg' object.
 * @param to Recipient ID.
 * @param is_atc True if sender is an ATC station, false if it is an
 *	aircraft station. ATC stations do not have individual quota
 *	applied to their stored messages.
 *
 * @return True if the message was stored for later delivery. False
 *	if storing the message couldn't be performed. This can only
 *	happen when either the global message queue has run out of
 *	space, or if the sender's quota has been exhausted.
 */
static bool
store_msg(const cpdlc_msg_t *msg, const char *to, bool is_atc)
{
	uint64_t bytes = cpdlc_msg_encode(msg, NULL, 0);
	queued_msg_t *qmsg;
	char *buf;

	ASSERT(msg != NULL);
	ASSERT(to != NULL);
	ASSERT(cpdlc_msg_get_from(msg) != NULL);

	if (queued_msg_max_bytes != 0 &&
	    queued_msg_bytes + bytes > queued_msg_max_bytes) {
		logMsg("Cannot queue message from %s, global message queue "
		    "is completely out of space (%lld bytes)",
		    cpdlc_msg_get_from(msg), (long long)queued_msg_max_bytes);
		return (false);
	}
	if (!is_atc && !msgquota_incr(cpdlc_msg_get_from(msg), bytes))
		return (false);

	qmsg = safe_calloc(1, sizeof (*qmsg));
	buf = safe_malloc(bytes + 1);

	cpdlc_msg_encode(msg, buf, bytes + 1);

	qmsg->msg = buf;
	qmsg->created = time(NULL);
	qmsg->is_atc = is_atc;
	lacf_strlcpy(qmsg->from, cpdlc_msg_get_from(msg), sizeof (qmsg->from));
	lacf_strlcpy(qmsg->to, to, sizeof (qmsg->to));

	list_insert_tail(&queued_msgs, qmsg);
	queued_msg_bytes += bytes;

	return (true);
}

/*
 * Returns true if a message is a "NOT CURRENT DATA AUTHORITY" message,
 * or false otherwise. This is used to allow these special errors to
 * propagate from an aircraft station to an ATC station even though the
 * aircraft is not currently logged onto said ATC station.
 */
static bool
msg_is_not_cda(const cpdlc_msg_t *msg)
{
	ASSERT(msg != NULL);
	ASSERT(msg->segs[0].info != NULL);
	return (msg->num_segs == 1 && msg->segs[0].info->is_dl &&
	    msg->segs[0].info->msg_type ==
	    CPDLC_DM63_NOT_CURRENT_DATA_AUTHORITY);
}

/*
 * When an ATC station sends a message with a FROM=AUTO header, the
 * server attempts to figure out which ATC station the target of the
 * message was logged onto and rewrites the FROM header of the
 * message to always match the current aircraft's logon target.
 */
static bool
msg_auto_assign_from(cpdlc_msg_t *msg)
{
	conn_t *acft_conn = NULL;
	const list_t *l;

	mutex_enter(&conns_by_from_lock);

	l = htbl_lookup_multi(&conns_by_from, cpdlc_msg_get_to(msg));
	if (l != NULL && list_count(l) != 0) {
		/*
		 * Pick the last connection, since that's the most likely
		 * one to be the current one. The earlier ones could be
		 * stale connections.
		 */
		acft_conn = HTBL_VALUE_MULTI(list_tail(l));
		if (acft_conn != NULL) {
			cpdlc_msg_set_from(msg, acft_conn->to);
			mutex_exit(&conns_by_from_lock);
			return (true);
		}
	}

	mutex_exit(&conns_by_from_lock);

	return (false);
}

static void
forward_msg_cb(const cpdlc_msg_t *msg, const char *addr_str, void *userinfo)
{
	const char *to;
	const list_t *l;

	ASSERT(msg != NULL);
	to = cpdlc_msg_get_to(msg);
	ASSERT(addr_str != NULL);
	UNUSED(userinfo);

	conn_log_msg(addr_str, msg, true);
	/*
	 * If there is at least one connection matching the identity of
	 * the intended recipient, forward the message without storing it.
	 * Otherwise, we store it for later delivery as soon as the
	 * recipient becomes available, or until the message expires.
	 */
	mutex_enter(&conns_by_from_lock);
	l = htbl_lookup_multi(&conns_by_from, to);
	if (l != NULL && list_count(l) != 0) {
		for (void *mv = list_head(l), *mv_next = NULL; mv != NULL;
		    mv = mv_next) {
			conn_t *tgt_conn = HTBL_VALUE_MULTI(mv);

			mv_next = list_next(l, mv);
			ASSERT(tgt_conn != NULL);
			conn_send_msg(tgt_conn, msg);
		}
	} else {
		if (!store_msg(msg, to, false)) {
			send_error_msg_to(cpdlc_msg_get_from(msg), msg,
			    "TOO MANY QUEUED MESSAGES");
		}
	}
	mutex_exit(&conns_by_from_lock);
}

static void
discard_msg_cb(const cpdlc_msg_t *msg, const char *addr_str, void *userinfo)
{
	ASSERT(msg != NULL);
	ASSERT(addr_str != NULL);
	UNUSED(userinfo);
	/* Only log the message for tracking purposes */
	conn_log_msg(addr_str, msg, true);
}

static void
forward_msg(conn_t *conn, cpdlc_msg_t *msg, char to[CALLSIGN_LEN])
{
	ASSERT(conn != NULL);
	ASSERT(msg != NULL);
	/*
	 * Stamp the message with its sender connection. No matter what
	 * FROM= header the client provided, this overrides it. On ATC
	 * connections, we allow other IDs.
	 */
	ASSERT(list_count(&conn->from_list) != 0);
	if (cpdlc_msg_get_from(msg)[0] == '\0' || !conn->is_atc) {
		ident_list_t *idl = list_head(&conn->from_list);
		cpdlc_msg_set_from(msg, idl->ident);
	}
	msg_router(conn->addr_str, conn->is_atc, conn->is_lws, msg, to,
	    forward_msg_cb, discard_msg_cb, NULL);
}

/*
 * Handles an incoming message from a connection. This performs all
 * necessary permissions checks, logon hooks and message forwarding.
 *
 * @param conn The connection which received the message.
 * @param msg The message to process. The message is consumed by this
 *	function (either storing it, or freeing it), so the caller
 *	relinquishes control of the object.
 */
static void
conn_process_msg(conn_t *conn, cpdlc_msg_t *msg)
{
	char to[CALLSIGN_LEN] = { 0 };

	ASSERT(conn != NULL);
	ASSERT(msg != NULL);
	ASSERT_CONNS_MUTEX_HELD(conn);

	/*
	 * If the user isn't logged on, don't allow anything other than
	 * LOGON through.
	 */
	mutex_enter(&conn->lock);
	if (conn->logon_status != LOGON_COMPLETE && !msg->is_logon) {
		mutex_exit(&conn->lock);
		send_error_msg(conn, msg, "LOGON REQUIRED");
		cpdlc_msg_free(msg);
		return;
	}
	mutex_exit(&conn->lock);

	if (msg->is_logon || msg->is_logoff) {
		/* Logon messages do not get forwarded. */
		conn_log_msg(conn->addr_str, msg, true);
		process_logon_msg(conn, msg);
		cpdlc_msg_free(msg);
		return;
	}
	if (msg->pkt_type == CPDLC_PKT_PING) {
		/* Generate a local PONG message with no further processing */
		cpdlc_msg_t *pong = cpdlc_msg_alloc(CPDLC_PKT_PONG);
		cpdlc_msg_set_mrn(pong, cpdlc_msg_get_min(msg));
		conn_send_msg(conn, pong);
		cpdlc_msg_free(pong);
		cpdlc_msg_free(msg);
		/* These messages do not get logged */
		return;
	}
	if (msg->to[0] != '\0') {
		/*
		 * Aircraft stations can only communicate with their
		 * LOGON target.
		 */
		if (!conn->is_atc && !msg_is_not_cda(msg)) {
			conn_log_msg(conn->addr_str, msg, true);
			send_error_msg(conn, msg,
			    "MESSAGE CANNOT CONTAIN TO= HEADER");
			cpdlc_msg_free(msg);
			return;
		}
		lacf_strlcpy(to, msg->to, sizeof (to));
	} else if (conn->to[0] != '\0') {
		lacf_strlcpy(to, conn->to, sizeof (to));
		/*
		 * Message has no TO= target set, but the connection has
		 * one, so copy that into the message. This makes sure
		 * that ATC stations that do multi-logon can discriminate
		 * their own endpoint.
		 */
		cpdlc_msg_set_to(msg, conn->to);
	} else {
		/*
		 * ATC stations MUST provide a TO= header, as they
		 * otherwise have no default send target.
		 */
		conn_log_msg(conn->addr_str, msg, true);
		send_error_msg(conn, msg, "MESSAGE MISSING TO= HEADER");
		cpdlc_msg_free(msg);
		return;
	}
	ASSERT(msg->num_segs > 0);
	ASSERT(msg->segs[0].info != NULL);
	/*
	 * Make sure the message has the proper uplink/downlink message
	 * type depending on the connection type. We only need to check
	 * the first message segment, as cpdlc_msg_decode has made sure
	 * that all segments have the same is_dl value.
	 */
	if (conn->is_atc && msg->segs[0].info->is_dl) {
		conn_log_msg(conn->addr_str, msg, true);
		send_error_msg(conn, msg,
		    "MESSAGE UPLINK/DOWNLINK MISMATCH");
		cpdlc_msg_free(msg);
		return;
	}
	if (conn->is_atc && strcmp(cpdlc_msg_get_from(msg), "AUTO") == 0 &&
	    !msg_auto_assign_from(msg)) {
		conn_log_msg(conn->addr_str, msg, true);
		send_error_msg(conn, msg, "REMOTE END NOT CONNECTED");
		cpdlc_msg_free(msg);
		return;
	}
	if (!conn->is_atc && !msg->segs[0].info->is_dl) {
		conn_log_msg(conn->addr_str, msg, true);
		send_svc_unavail_msg(conn, cpdlc_msg_get_min(msg));
		cpdlc_msg_free(msg);
		return;
	}
	/* Forwarded messages are only logged after the routing decision */
	forward_msg(conn, msg, to);
}

/*
 * Drains a connection's `inbuf', attempts to construct messages from it
 * and processes them. Any input that isn't a full message yet, will be
 * left in `inbuf'. This function shortens `inbuf' as necessary to adjust
 * it for the consumed messages.
 *
 * @return True if input processing was successful. False if a fatal error
 *	was encountered and the connection must be terminated.
 */
static bool
conn_process_input(conn_t *conn)
{
	int consumed_total = 0;

	ASSERT(conn != NULL);
	ASSERT(conn->inbuf_sz != 0);
	ASSERT_CONNS_MUTEX_HELD(conn);
	ASSERT_MUTEX_HELD(&conn->lock);

	while (consumed_total < (int)conn->inbuf_sz) {
		int consumed;
		cpdlc_msg_t *msg;
		char error[128] = { 0 };

		if (!cpdlc_msg_decode(
		    (const char *)&conn->inbuf[consumed_total], &msg,
		    &consumed, error, sizeof (error))) {
			logMsg("Error decoding message from client %s: %s",
			    conn->addr_str, error);
			return (false);
		}
		/* No more complete messages pending? */
		if (msg == NULL)
			break;
		ASSERT(consumed != 0);
		/* This consumes the msg, so no need to free it */
		conn_process_msg(conn, msg);
		consumed_total += consumed;
		ASSERT3S(consumed_total, <=, conn->inbuf_sz);
	}
	if (consumed_total != 0) {
		/* Adjust `inbuf' to get rid of the consumed message data */
		ASSERT3S(consumed_total, <=, conn->inbuf_sz);
		conn->inbuf_sz -= consumed_total;
		memmove(conn->inbuf, &conn->inbuf[consumed_total],
		    conn->inbuf_sz + 1);
		conn->inbuf = realloc(conn->inbuf, conn->inbuf_sz);
	}

	return (true);
}

/*
 * When TLS client verification is enabled, this function checks if the
 * client's certificate is valid. The CA certificate used to validate
 * the client's certificate is the same CA certificate as is used in our
 * server connection. If a CRL is configured for the server, that is
 * checked as well.
 *
 * @return True if the client's certificate is valid, false if it isn't
 *	(the reason is printed to the log).
 */
static bool
tls_verify_peer(conn_t *conn)
{
	unsigned int status;
	int error;

	ASSERT(conn != NULL);

	error = gnutls_certificate_verify_peers2(conn->session, &status);
	if (error != GNUTLS_E_SUCCESS) {
		logMsg("TLS handshake error: error validating client "
		    "certificate from %s: %s\n", conn->addr_str,
		    gnutls_strerror(error));
		return (false);
	}
	if (status != 0) {
		logMsg("TLS handshake error: client certificate from %s "
		    "failed validation with status 0x%x", conn->addr_str,
		    status);
		return (false);
	}
	return (true);
}

/*
 * Data input validator. All incoming connection data must be plaintext.
 */
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

/*
 * Drains a connection of any pending input bytes and stores them in the
 * `inbuf' cache. This function then calls conn_process_input to turn any
 * available input bytes into messages and process them.
 *
 * @return True if the input read was successful. False if a fatal error
 *	was encountered during input reading or processing of messages,
 *	or the remote end has closed the connection. The caller should call
 *	close_conn.
 */
static bool
conn_read_input(conn_t *conn)
{
	ASSERT(conn != NULL);
	ASSERT_MUTEX_HELD(&conns_tcp_lock);

	for (;;) {
		uint8_t buf[READ_BUF_SZ];
		size_t max_inbuf_sz = (list_count(&conn->from_list) != 0 ?
		    MAX_BUF_SZ : MAX_BUF_SZ_NO_LOGON);
		int bytes;

		if (!conn->tls_handshake_complete) {
			int error = gnutls_handshake(conn->session);

			if (error != GNUTLS_E_SUCCESS) {
				if (error == GNUTLS_E_AGAIN) {
					/* Need more data */
					return (true);
				}
				logMsg("TLS handshake error from %s: %s",
				    conn->addr_str, gnutls_strerror(error));
				return (false);
			}
			if (req_client_cert && !tls_verify_peer(conn))
				return (false);
			/* TLS handshake succeeded */
			conn->tls_handshake_complete = true;
		}

		bytes = gnutls_record_recv(conn->session, buf, sizeof (buf));
		if (bytes < 0) {
			/* Read error, or no more data pending */
			if (bytes == GNUTLS_E_AGAIN)
				return (true);
			if (!gnutls_error_is_fatal(bytes)) {
				logMsg("Soft read error on connection from %s, "
				    "can retry: %s", conn->addr_str,
				    gnutls_strerror(bytes));
				continue;
			}
			logMsg("Fatal read error on connection from %s: %s",
			    conn->addr_str, gnutls_strerror(bytes));
			return (false);
		}
		if (bytes == 0) {
			/* Connection closed */
			return (false);
		}
		if (!sanitize_input(buf, bytes)) {
			logMsg("Invalid input character on connection from "
			    "%s: data MUST be plain text", conn->addr_str);
			return (false);
		}
		if (conn->inbuf_sz + bytes > max_inbuf_sz) {
			logMsg("Input buffer overflow on connection from %s: "
			    "received %d bytes, maximum allowable is %d bytes",
			    conn->addr_str, (int)(conn->inbuf_sz + bytes),
			    (int)max_inbuf_sz);
			return (false);
		}
		/*
		 * Attach the new input bytes to the end of `inbuf'. That's
		 * where conn_process_input will drain it from.
		 */
		mutex_enter(&conn->lock);

		conn->inbuf = safe_realloc(conn->inbuf,
		    conn->inbuf_sz + bytes + 1);
		memcpy(&conn->inbuf[conn->inbuf_sz], buf, bytes);
		conn->inbuf_sz += bytes;
		conn->inbuf[conn->inbuf_sz] = '\0';

		if (!conn_process_input(conn)) {
			mutex_exit(&conn->lock);
			return (false);
		}
		mutex_exit(&conn->lock);
	}
}

/*
 * Sends any pending output data over the associated connection. The
 * function returns when all pending output data has been sent, further
 * sending would block, or if an error is encountered. The `outbuf' of the
 * connection is adjusted to remove the data sent.
 *
 * @return True if sending data was successful. False if a fatal error has
 *	been encountered and the caller should call close_conn.
 */
static bool
conn_write_output(conn_t *conn)
{
	int bytes;

	ASSERT(conn != NULL);
	ASSERT(conn->outbuf_sz != 0);
	ASSERT_MUTEX_HELD(&conns_tcp_lock);

	mutex_enter(&conn->lock);
	/*
	 * We are in non-blocking mode, so we can hold `lock' here safely
	 * during the record send operation.
	 */
	bytes = gnutls_record_send(conn->session,
	    &conn->outbuf[conn->outbuf_pre_pad], conn->outbuf_sz);
	if (bytes < 0) {
		if (bytes != GNUTLS_E_AGAIN) {
			if (gnutls_error_is_fatal(bytes)) {
				logMsg("Fatal send error on connection from "
				    "%s: %s", conn->addr_str,
				    gnutls_strerror(bytes));
				mutex_exit(&conn->lock);
				return (false);
			}
			logMsg("Soft send error on connection from %s: %s",
			    conn->addr_str, gnutls_strerror(bytes));
		}
	} else if (bytes > 0) {
		if ((ssize_t)conn->outbuf_sz > bytes) {
			memmove(&conn->outbuf[conn->outbuf_pre_pad],
			    &conn->outbuf[conn->outbuf_pre_pad + bytes],
			    (conn->outbuf_sz - bytes) + 1);
			conn->outbuf_sz -= bytes;
			conn->outbuf = safe_realloc(conn->outbuf,
			    conn->outbuf_pre_pad + conn->outbuf_sz + 1);
		} else {
			free(conn->outbuf);
			conn->outbuf = NULL;
			conn->outbuf_sz = 0;
		}
	}

	mutex_exit(&conn->lock);

	return (true);
}

/*
 * Polls all sockets for incoming data and if we have any pending data
 * to be written, also polls on client connection sockets for ready-to-send
 * status. This is the main I/O worker function of the server.
 */
static void
poll_sockets(void)
{
	unsigned sock_nr = 0;
	unsigned num_pfds;
	struct pollfd *pfds;
	int poll_res, polls_seen;

	mutex_enter(&conns_tcp_lock);
retry_poll:
	conns_tcp_dirty = false;
	num_pfds = 1 + list_count(&listen_socks) + list_count(&conns_tcp);
	pfds = safe_calloc(num_pfds, sizeof (*pfds));

	/*
	 * The first fd in the poll list is always our poll wakeup pipe.
	 */
	pfds[sock_nr].fd = poll_wakeup_pipe[0];
	pfds[sock_nr].events = POLLIN;
	sock_nr++;

	for (listen_sock_t *ls = list_head(&listen_socks); ls != NULL;
	    ls = list_next(&listen_socks, ls), sock_nr++) {
		pfds[sock_nr].fd = ls->fd;
		pfds[sock_nr].events = POLLIN;
	}
	for (conn_t *conn = list_head(&conns_tcp); conn != NULL;
	    conn = list_next(&conns_tcp, conn), sock_nr++) {
		pfds[sock_nr].fd = conn->fd;
		pfds[sock_nr].events = POLLIN;
		/* If a socket has data to send, poll for output as well */
		if (conn->outbuf_sz != 0)
			pfds[sock_nr].events |= POLLOUT;
	}
	ASSERT3U(sock_nr, ==, num_pfds);
	mutex_exit(&conns_tcp_lock);

	poll_res = poll(pfds, num_pfds, POLL_TIMEOUT);
	if (poll_res == -1) {
		/*
		 * In case conns_tcp was changed and a socket closed before
		 * we attempted to poll, we might end up getting spurious
		 * poll errors.
		 */
		free(pfds);
		return;
	}
	if (poll_res == 0) {
		/* Poll timeout, respin another loop */
		free(pfds);
		return;
	}

	mutex_enter(&conns_tcp_lock);
	/*
	 * If the conns_tcp list has changed while we were polling, we must
	 * retry the poll with a refreshed connections list.
	 */
	if (conns_tcp_dirty) {
		free(pfds);
		goto retry_poll;
	}

	polls_seen = 0;
	sock_nr = 0;

	if (pfds[0].revents & POLLIN) {
		uint8_t buf[4096];

		polls_seen++;
		/*
		 * poll wakeup pipe has been disturbed, drain it.
		 * This avoids us being woken up unnecessarily many times.
		 */
		while (read(poll_wakeup_pipe[0], buf, sizeof (buf)) > 0)
			;
		if (polls_seen == poll_res)
			goto out;
	}
	sock_nr++;
	/*
	 * Handling of newly accepted connections.
	 */
	for (listen_sock_t *ls = list_head(&listen_socks); ls != NULL;
	    ls = list_next(&listen_socks, ls), sock_nr++) {
		if (pfds[sock_nr].revents & (POLLIN | POLLPRI)) {
			handle_accepts(ls);
			polls_seen++;
			/* If we've processed all polls, early exit */
			if (polls_seen >= poll_res)
				goto out;
		}
	}
	/*
	 * This is the primary client connection I/O event loop.
	 */
	for (conn_t *conn = list_head(&conns_tcp), *next_conn = NULL;
	    conn != NULL; conn = next_conn, sock_nr++) {
		/*
		 * Grab the next connection handle now in case
		 * the connection gets closed due to EOF or errors.
		 */
		next_conn = list_next(&conns_tcp, conn);
		if (pfds[sock_nr].revents & (POLLIN | POLLOUT)) {
			polls_seen++;
			if (pfds[sock_nr].revents & POLLIN) {
				if (!conn_read_input(conn)) {
					close_conn(conn);
					continue;
				}
			}
			if (conn->outbuf_sz != 0 &&
			    (pfds[sock_nr].revents & POLLOUT)) {
				if (!conn_write_output(conn)) {
					close_conn(conn);
					continue;
				}
			}
			/* If we've processed all polls, early exit */
			if (polls_seen >= poll_res)
				goto out;
		}
	}
out:
	free(pfds);
	mutex_exit(&conns_tcp_lock);
}

static void
handle_lws_input(void)
{
	mutex_enter(&conns_lws_lock);

	for (conn_t *conn = list_head(&conns_lws); conn != NULL;
	    conn = list_next(&conns_lws, conn)) {
		ASSERT(conn->is_lws);
		ASSERT(conn->wsi != NULL);

		mutex_enter(&conn->lock);
		if (conn->inbuf_sz > 0 && !conn_process_input(conn)) {
			logMsg("Error LWS connection from %s: input "
			    "processing error", conn->addr_str);
			conn->kill_wsi = true;
		}
		mutex_exit(&conn->lock);
	}

	mutex_exit(&conns_lws_lock);
}

/*
 * Removes a queued message it from the delayed-delivery queue (queued_msgs)
 * and adjusts the quota and queue size accounting.
 */
static void
dequeue_msg(queued_msg_t *qmsg)
{
	uint64_t bytes;

	ASSERT(qmsg != NULL);
	bytes = strlen(qmsg->msg);
	ASSERT3U(queued_msg_bytes, >=, bytes);
	queued_msg_bytes -= bytes;
	if (!qmsg->is_atc)
		msgquota_decr(qmsg->from, bytes);
	list_remove(&queued_msgs, qmsg);
	free(qmsg->msg);
	free(qmsg);
	if (list_count(&queued_msgs) == 0)
		ASSERT0(queued_msg_bytes);
}

/*
 * Runs over `queued_msgs' and processes all of the queued messages. Any
 * messages which can be delivered are sent to their respective connections.
 * Alternatively, any messages which have expired are dropped from the queue.
 */
static void
handle_queued_msgs(void)
{
	time_t now = time(NULL);

	for (queued_msg_t *qmsg = list_head(&queued_msgs), *next_qmsg = NULL;
	    qmsg != NULL; qmsg = next_qmsg) {
		const list_t *l;
		/*
		 * Messages might be removed from the list below, so we need
		 * to grab the next message pointer ahead of time.
		 */
		next_qmsg = list_next(&queued_msgs, qmsg);

		mutex_enter(&conns_by_from_lock);
		l = htbl_lookup_multi(&conns_by_from, qmsg->to);
		if (l != NULL && list_count(l) != 0) {
			/*
			 * One or more connections with the identity of the
			 * message's intended recipient have been found, so
			 * deliver the message to them and remove it from
			 * the queue.
			 */
			for (void *mv = list_head(l); mv != NULL;
			    mv = list_next(l, mv)) {
				conn_t *conn = HTBL_VALUE_MULTI(mv);
				conn_send_buf(conn, qmsg->msg,
				strlen(qmsg->msg));
			}
			dequeue_msg(qmsg);
		} else if (now - qmsg->created > QUEUED_MSG_TIMEOUT) {
			/*
			 * Message has timed out, remove it from the queue.
			 */
			dequeue_msg(qmsg);
		}
		mutex_exit(&conns_by_from_lock);
	}
}

/*
 * Runs through existing connections and close ones which are now
 * on the blocklist. This allows for forcibly disconnecting clients
 * that have been added to the blocklist after connecting.
 */
static void
close_blocked_conns(void)
{
	mutex_enter(&conns_tcp_lock);
	for (conn_t *conn = list_head(&conns_tcp), *conn_next = NULL;
	    conn != NULL; conn = conn_next) {
		conn_next = list_next(&conns_tcp, conn);
		if (!blocklist_check(&conn->sockaddr))
			close_conn(conn);
	}
	mutex_exit(&conns_tcp_lock);

	mutex_enter(&conns_lws_lock);
	for (conn_t *conn = list_head(&conns_lws), *conn_next = NULL;
	    conn != NULL; conn = conn_next) {
		conn_next = list_next(&conns_lws, conn);
		if (!blocklist_check(&conn->sockaddr))
			conn->kill_wsi = true;
	}
	mutex_exit(&conns_lws_lock);
}

/*
 * Runs through existing connections and close ones which haven't logged on
 * yet and have timed out.
 */
static void
close_timedout_conns(void)
{
	time_t now = time(NULL);

	mutex_enter(&conns_tcp_lock);
	for (conn_t *conn = list_head(&conns_tcp), *conn_next = NULL;
	    conn != NULL; conn = conn_next) {
		conn_next = list_next(&conns_tcp, conn);
		if (!conn->logon_success &&
		    now - conn->logoff_time > LOGON_GRACE_TIME) {
			close_conn(conn);
		}
	}
	mutex_exit(&conns_tcp_lock);

	mutex_enter(&conns_lws_lock);
	for (conn_t *conn = list_head(&conns_lws), *conn_next = NULL;
	    conn != NULL; conn = conn_next) {
		conn_next = list_next(&conns_lws, conn);
		ASSERT(conn->wsi != NULL);
		if (!conn->logon_success &&
		    now - conn->logoff_time > LOGON_GRACE_TIME) {
			conn->kill_wsi = true;
		}
	}
	mutex_exit(&conns_lws_lock);
}

static void
write_logon_list_cb(const void *key, void *value, void *userinfo)
{
	const char *from;
	const conn_t *conn;
	FILE *fp;

	ASSERT(key != NULL);
	from = key;
	ASSERT(value != NULL);
	conn = value;
	ASSERT(userinfo != NULL);
	fp = userinfo;

	fprintf(fp, "%s\t%s\t%s\t%s\t%s\n", from,
	    conn->to[0] != '\0' ? conn->to : "-",
	    conn->is_atc ? "ATC" : "ACFT",
	    conn->addr_str, conn->is_lws ? "WS" : "TLS");
}

/*
 * Writes the logon list file if it has changed.
 */
static void
write_logon_list(void)
{
	FILE *fp;
	char *tmp_filename;

	if (logon_list_file[0] == '\0' || !conns_by_from_changed)
		return;
	conns_by_from_changed = false;

	tmp_filename = sprintf_alloc("%s.tmp", logon_list_file);
	fp = fopen(tmp_filename, "wb");
	if (fp == NULL) {
		logMsg("Can't write LOGON list file %s: %s", tmp_filename,
		    strerror(errno));
		free(tmp_filename);
		return;
	}
	htbl_foreach(&conns_by_from, write_logon_list_cb, fp);
	fclose(fp);

	if (rename(tmp_filename, logon_list_file) != 0) {
		logMsg("Can't rename LOGON list file %s to %s: %s",
		    tmp_filename, logon_list_file, strerror(errno));
	}
	free(tmp_filename);
}

/*
 * Initializes our global TLS parameters.
 */
static bool
tls_init(void)
{
#define	CHECKFILE(__filename, __kind) \
	do { \
		FILE *fp = fopen((__filename), "r"); \
		if (fp == NULL) { \
			logMsg("cannot open " __kind " file "\
			    "%s: %s\n", (__filename), strerror(errno)); \
			gnutls_global_deinit(); \
			return (false); \
		} \
		fclose(fp); \
	} while (0)
#define	TLS_CHK(op) \
	do { \
		int error = (op); \
		if (error < GNUTLS_E_SUCCESS) { \
			logMsg("%s failed: %s", #op, gnutls_strerror(error)); \
			gnutls_global_deinit(); \
			return (false); \
		} \
	} while (0)

	TLS_CHK(gnutls_global_init());
	TLS_CHK(gnutls_certificate_allocate_credentials(&x509_creds));
	if (tls_cafile[0] != '\0') {
		CHECKFILE(tls_cafile, "CA");
		TLS_CHK(gnutls_certificate_set_x509_trust_file(x509_creds,
		    tls_cafile, GNUTLS_X509_FMT_PEM));
	}
	if (tls_crlfile[0] != '\0') {
		CHECKFILE(tls_crlfile, "CRL");
		TLS_CHK(gnutls_certificate_set_x509_crl_file(x509_creds,
		    tls_crlfile, GNUTLS_X509_FMT_PEM));
	}
	CHECKFILE(tls_keyfile, "private key");
	CHECKFILE(tls_certfile, "certificate");
	TLS_CHK(gnutls_certificate_set_x509_key_file2(x509_creds,
	    tls_certfile, tls_keyfile, GNUTLS_X509_FMT_PEM,
	    tls_keyfile_pass, tls_keyfile_enctype));
#if	GNUTLS_VERSION_NUMBER >= 0x030506
	gnutls_certificate_set_known_dh_params(x509_creds,
	    GNUTLS_SEC_PARAM_HIGH);
#endif	/* GNUTLS_VERSION_NUMBER */
	TLS_CHK(gnutls_priority_init(&prio_cache, NULL, NULL));

	return (true);
#undef	TLS_CHK
#undef	CHECKFILE
}

/*
 * Destroys global TLS parameters.
 */
static void
tls_fini(void)
{
	gnutls_certificate_free_credentials(x509_creds);
	gnutls_priority_deinit(prio_cache);
	gnutls_global_deinit();
}

static void
log_dbg_string(const char *str)
{
	fputs(str, stderr);
}

static void
handle_sighup(int signum)
{
	UNUSED(signum);
	msglog_reopen();
}

int
main(int argc, char *argv[])
{
	int opt;
	const char *conf_path = NULL;
	bool encrypt_silent = false;
	const struct sigaction sa_hup = { .sa_handler = handle_sighup };

	/* Initialize libacfutils' logMsg and crc64 functions */
	log_init(log_dbg_string, "cpdlcd");
	crc64_init();
	/* Initialize cURL's global data structures */
	curl_global_init(CURL_GLOBAL_ALL);

	/* Default certificate names */
	lacf_strlcpy(tls_keyfile, "cpdlcd_key.pem", sizeof (tls_keyfile));
	lacf_strlcpy(tls_certfile, "cpdlcd_cert.pem", sizeof (tls_certfile));

	while ((opt = getopt(argc, argv, "hc:des")) != -1) {
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
		case 'e':
			if (!auth_encrypt_userpwd(encrypt_silent))
				return (1);
			return (0);
		case 's':
			encrypt_silent = true;
			break;
		default:
			print_usage(argv[0], stderr);
			return (1);
		}
	}

	init_structs();
	if (background && !daemonize(true, true))
		return (1);
	if ((conf_path != NULL && !parse_config(conf_path)) ||
	    (conf_path == NULL && !auto_config())) {
		return (1);
	}
	if (!tls_init())
		return (1);
	(void) blocklist_refresh();

	sigaction(SIGHUP, &sa_hup, NULL);

	while (!do_shutdown) {
		poll_sockets();
		handle_lws_input();
		complete_logons();
		handle_queued_msgs();
		if (blocklist_refresh())
			close_blocked_conns();
		close_timedout_conns();
		write_logon_list();
	}

	msgquota_fini();
	auth_fini();
	msg_router_fini();
	tls_fini();
	fini_structs();
	curl_global_cleanup();
	free(logon_cmd);
	free(logoff_cmd);

	if (msg_log_file != NULL) {
		fclose(msg_log_file);
		msg_log_file = NULL;
	}
	mutex_destroy(&msg_log_lock);

	return (0);
}

static void
lws_worker(void *userinfo)
{
	listen_lws_t *lws = userinfo;

	ASSERT(userinfo != NULL);
	ASSERT(lws->ctx != NULL);

	while (!lws->shutdown)
		lws_service(lws->ctx, POLL_TIMEOUT);
}

static bool
filter_lws_conn(int fd)
{
	struct sockaddr_storage sa;
	socklen_t sa_len = sizeof (sa);

	ASSERT(fd != -1);

	memset(&sa, 0, sizeof (sa));
	if (getpeername(fd, (struct sockaddr *)&sa, &sa_len) < 0) {
		logMsg("Error in getpeername: %s", strerror(errno));
		return (true);
	}
	if (!blocklist_check(&sa)) {
		char addr[SOCKADDR_STRLEN];
		sockaddr2str(&sa, addr);
		logMsg("Incoming connection blocked: "
		    "address %s on blocklist.", addr);
		return (true);
	}
	return (false);
}

static void
conn_established_lws(conn_t *conn, struct lws *wsi)
{
	int fd;
	socklen_t sa_len = sizeof (conn->sockaddr);

	ASSERT(conn != NULL);
	ASSERT(wsi != NULL);

	memset(conn, 0, sizeof (*conn));

	conn->is_lws = true;
	conn->wsi = wsi;
	conn->outbuf_pre_pad = P2ROUNDUP(LWS_PRE);
	conn->logoff_time = time(NULL);
	/*
	 * We must have validated the address before already, so we can't
	 * be having trouble grabbing it again here.
	 */
	fd = lws_get_socket_fd(wsi);
	VERIFY(fd != -1);
	VERIFY0(getpeername(fd, (struct sockaddr *)&conn->sockaddr,
	    &sa_len));
	sockaddr2str(&conn->sockaddr, conn->addr_str);

	mutex_init(&conn->lock);
	list_create(&conn->from_list, sizeof (ident_list_t),
	    offsetof(ident_list_t, node));

	mutex_enter(&conns_lws_lock);
	list_insert_tail(&conns_lws, conn);
	mutex_exit(&conns_lws_lock);
}

static bool
do_lws_output(conn_t *conn, struct lws *wsi)
{
	int bytes;

	ASSERT(conn != NULL);
	ASSERT(wsi != NULL);

	if (conn->outbuf_sz == 0)
		return (true);

	bytes = lws_write(wsi, &conn->outbuf[conn->outbuf_pre_pad],
	    conn->outbuf_sz, LWS_WRITE_TEXT);
	if (bytes == -1) {
		logMsg("Write error on connection from %s", conn->addr_str);
		return (false);
	}
	if (bytes == 0) {
		lws_callback_on_writable(wsi);
		return (true);
	}
	free(conn->outbuf);
	conn->outbuf = NULL;
	conn->outbuf_sz = 0;

	return (true);
}

static int
http_lws_cb(struct lws *wsi, enum lws_callback_reasons reason, void *user,
    void *in, size_t len)
{
	CPDLC_UNUSED(wsi);
	CPDLC_UNUSED(user);
	CPDLC_UNUSED(in);
	CPDLC_UNUSED(len);

	switch (reason) {
	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
		/* The `in' pointer is actually the new socket fd */
		return (filter_lws_conn((intptr_t)in));
	default:
		break;
	}

	return (0);
}

static int
cpdlc_lws_cb(struct lws *wsi, enum lws_callback_reasons reason, void *user,
    void *in, size_t len)
{
	conn_t *conn = user;

	ASSERT(wsi != NULL);
	CPDLC_UNUSED(reason);
	CPDLC_UNUSED(in);
	CPDLC_UNUSED(len);

	switch (reason) {
	case LWS_CALLBACK_ESTABLISHED:
		ASSERT(conn != NULL);
		ASSERT(wsi != NULL);
		conn_established_lws(conn, wsi);
		break;
	case LWS_CALLBACK_CLOSED:
		ASSERT(conn != NULL);
		mutex_enter(&conns_lws_lock);
		close_conn(conn);
		mutex_exit(&conns_lws_lock);
		break;
	case LWS_CALLBACK_RECEIVE:
		ASSERT(conn != NULL);
		if (conn->kill_wsi)
			return (-1);
		if (!sanitize_input(in, len)) {
			logMsg("Invalid input character on connection from "
			    "%s: data MUST be plain text", conn->addr_str);
			return (-1);
		}
		mutex_enter(&conn->lock);
		conn->inbuf = safe_realloc(conn->inbuf,
		    conn->inbuf_sz + len + 1);
		memcpy(&conn->inbuf[conn->inbuf_sz], in, len);
		conn->inbuf_sz += len;
		conn->inbuf[conn->inbuf_sz] = '\0';
		mutex_exit(&conn->lock);
		wake_up_main_thread();
		break;
	case LWS_CALLBACK_SERVER_WRITEABLE:
		ASSERT(conn != NULL);
		mutex_enter(&conn->lock);
		if (conn->kill_wsi || !do_lws_output(conn, wsi)) {
			mutex_exit(&conn->lock);
			return (-1);
		}
		mutex_exit(&conn->lock);
		break;
	default:
		break;
	}

	return (0);
}
