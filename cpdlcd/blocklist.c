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
#include <stddef.h>

#include <sys/stat.h>

#include <netdb.h>
#include <netinet/in.h>

#include <acfutils/assert.h>
#include <acfutils/avl.h>
#include <acfutils/helpers.h>
#include <acfutils/htbl.h>
#include <acfutils/safe_alloc.h>
#include <acfutils/thread.h>

#include "common.h"
#include "blocklist.h"

#define	BLOCK_ADDR_LEN	MAX(sizeof (struct in6_addr), sizeof (struct in_addr))

typedef struct {
	uint8_t		addr[BLOCK_ADDR_LEN];
	int		addr_family;
} block_addr_t;

static time_t	update_time = 0;
static char	filename[PATH_MAX] = { 0 };
static mutex_t	lock;
static bool	table_inited = false;
static htbl_t	table;
static bool	blocklist_entry = true;

void
blocklist_init(void)
{
	mutex_init(&lock);
}

void
blocklist_fini(void)
{
	if (table_inited) {
		htbl_empty(&table, NULL, NULL);
		htbl_destroy(&table);
		table_inited = false;
	}
	mutex_destroy(&lock);
}

void
blocklist_set_filename(const char *new_filename)
{
	lacf_strlcpy(filename, new_filename, sizeof (filename));
}

static bool
blocklist_refresh_impl(void)
{
	FILE *fp;
	unsigned num_lines = 0;
	char *line = NULL;
	size_t linecap = 0;

	/* Blocklist updated, refresh */
	fp = fopen(filename, "r");
	if (fp == NULL) {
		logMsg("Error refreshing blocklist: open failed: %s",
		    strerror(errno));
		return (false);
	}
	/* Count lines */
	for (num_lines = 0; !feof(fp); num_lines++)
		getline(&line, &linecap, fp);
	free(line);
	line = NULL;
	linecap = 0;
	rewind(fp);

	mutex_enter(&lock);

	/* Empty out the old blocklist data */
	if (table_inited) {
		htbl_empty(&table, NULL, NULL);
		htbl_destroy(&table);
		table_inited = false;
	}
	htbl_create(&table, num_lines, sizeof (block_addr_t), 0);
	table_inited = true;

	while (!feof(fp)) {
		char buf[128];
		int error;
		struct addrinfo *ai_full;
		struct addrinfo hints = {
		    .ai_family = AF_UNSPEC,
		    .ai_socktype = SOCK_STREAM,
		    .ai_protocol = IPPROTO_TCP
		};

		if (fscanf(fp, "%127s", buf) != 1)
			continue;
		/* Skip comments (words starting with '#') */
		if (buf[0] == '#') {
			int c;
			do {
				c = fgetc(fp);
			} while (c != EOF && c != '\n');
			continue;
		}
		/*
		 * Resolve the word to turn it into a binary representation.
		 * This automatically takes care of IPv4 and IPv6 addresses
		 * for us without having to worry about format.
		 */
		error = getaddrinfo(buf, NULL, &hints, &ai_full);
		if (error != 0) {
			logMsg("Cannot resolve blocklist entry %s: %s",
			    buf, gai_strerror(error));
			continue;
		}
		for (const struct addrinfo *ai = ai_full; ai != NULL;
		    ai = ai->ai_next) {
			block_addr_t ba;

			if (ai->ai_family != AF_INET &&
			    ai->ai_family != AF_INET6) {
				continue;
			}

			/* We only care about TCP connections. */
			ASSERT3U(ai->ai_protocol, ==, IPPROTO_TCP);
			ASSERT3U(ai->ai_addrlen, <=, sizeof (ba.addr));

			memset(&ba, 0, sizeof (ba));
			if (ai->ai_family == AF_INET) {
				const struct sockaddr_in *sa =
				    (struct sockaddr_in *)ai->ai_addr;
				memcpy(ba.addr, &sa->sin_addr,
				    sizeof (sa->sin_addr));
			} else {
				const struct sockaddr_in6 *sa =
				    (struct sockaddr_in6 *)ai->ai_addr;
				memcpy(ba.addr, &sa->sin6_addr,
				    sizeof (sa->sin6_addr));
			}
			ba.addr_family = ai->ai_family;

			if (htbl_lookup(&table, &ba) != NULL) {
				logMsg("Duplicate blocklist entry: %s", buf);
				break;
			}
			htbl_set(&table, &ba, &blocklist_entry);
		}
		freeaddrinfo(ai_full);
	}
	fclose(fp);

	mutex_exit(&lock);

	return (true);
}

bool
blocklist_refresh(void)
{
	struct stat st;

	if (filename[0] == '\0')
		return (false);
	if (stat(filename, &st) != 0) {
		logMsg("Error refreshing blocklist %s: stat failed: %s",
		    filename, strerror(errno));
		return (false);
	}
	if (st.st_mtime == update_time) {
		/* Blocklist up to date */
		return (false);
	}
	if (!blocklist_refresh_impl())
		return (false);
	update_time = st.st_mtime;

	return (true);
}

bool
blocklist_check_impl(const void *addr, size_t addr_len, int addr_family)
{
	block_addr_t ba;
	bool result;

	ASSERT(addr != NULL);
	ASSERT(addr_len != 0);
	ASSERT(addr_family == AF_INET || addr_family == AF_INET6);

	memset(&ba, 0, sizeof (ba));
	memcpy(ba.addr, addr, addr_len);
	ba.addr_family = addr_family;

	mutex_enter(&lock);
	result = (htbl_lookup(&table, &ba) == NULL);
	mutex_exit(&lock);

	return (result);
}

bool
blocklist_check(const void *sockaddr)
{
	sa_family_t addr_family;

	ASSERT(sockaddr != NULL);
	addr_family = ((struct sockaddr *)sockaddr)->sa_family;
	ASSERT(addr_family == AF_INET || addr_family == AF_INET6);

	if (addr_family == AF_INET) {
		const struct sockaddr_in *sa = sockaddr;
		return (blocklist_check_impl(&sa->sin_addr,
		    sizeof (sa->sin_addr), AF_INET));
	} else {
		const struct sockaddr_in6 *sa = sockaddr;
		return (blocklist_check_impl(&sa->sin6_addr,
		    sizeof (sa->sin6_addr), AF_INET6));
	}
}
