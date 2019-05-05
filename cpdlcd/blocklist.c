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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

#include <netdb.h>
#include <netinet/in.h>

#include <acfutils/assert.h>
#include <acfutils/avl.h>
#include <acfutils/helpers.h>
#include <acfutils/safe_alloc.h>

#include "common.h"
#include "blocklist.h"

typedef struct {
	uint8_t		addr[MAX_ADDR_LEN];
	socklen_t	addr_len;
	int		addr_family;
	avl_node_t	node;
} block_addr_t;

static time_t		update_time = 0;
static char		filename[PATH_MAX] = { 0 };
static avl_tree_t	tree;

static int
block_addr_compar(const void *a, const void *b)
{
	const block_addr_t *ba = a, *bb = b;
	int res;

	if (ba->addr_family < bb->addr_family)
		return (-1);
	if (ba->addr_family > bb->addr_family)
		return (1);
	ASSERT3U(ba->addr_len, ==, bb->addr_len);
	res = memcmp(ba->addr, bb->addr, sizeof (ba->addr_len));
	if (res < 0)
		return (-1);
	if (res > 0)
		return (1);
	return (0);
}

void
blocklist_init(void)
{
	avl_create(&tree, block_addr_compar, sizeof (block_addr_t),
	    offsetof(block_addr_t, node));
}

void
blocklist_fini(void)
{
	void *cookie = NULL;
	block_addr_t *ba;

	while ((ba = avl_destroy_nodes(&tree, &cookie)) != NULL)
		free(ba);
	avl_destroy(&tree);
}

void
blocklist_set_filename(const char *new_filename)
{
	lacf_strlcpy(filename, new_filename, sizeof (filename));
}

static bool
blocklist_refresh_impl(void)
{
	void *cookie = NULL;
	block_addr_t *ba;
	FILE *fp;

	/* Blocklist updated, refresh */
	fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Error refreshing blocklist: open failed: %s\n",
		    strerror(errno));
		return (false);
	}
	/* Empty out the old blocklist data */
	while ((ba = avl_destroy_nodes(&tree, &cookie)) != NULL)
		free(ba);

	while (!feof(fp)) {
		char buf[128];
		int error;
		struct addrinfo *ai_full;

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
		error = getaddrinfo(buf, NULL, NULL, &ai_full);
		if (error != 0) {
			fprintf(stderr, "Cannot resolve blocklist entry %s: "
			    "%s\n", buf, gai_strerror(error));
			continue;
		}
		for (const struct addrinfo *ai = ai_full; ai != NULL;
		    ai = ai->ai_next) {
			avl_index_t where;

			/* Only care about TCP connections. */
			if (ai->ai_protocol != IPPROTO_TCP)
				continue;
			ba = safe_calloc(1, sizeof (*ba));
			ASSERT3U(ai->ai_addrlen, <=, sizeof (ba->addr));
			memcpy(ba->addr, ai->ai_addr, ai->ai_addrlen);
			ba->addr_len = ai->ai_addrlen;
			ba->addr_family = ai->ai_family;

			if (avl_find(&tree, ba, &where) != NULL) {
				fprintf(stderr, "Duplicate blocklist entry: "
				    "%s\n", buf);
				free(ba);
				break;
			}
			avl_insert(&tree, ba, where);
		}
		freeaddrinfo(ai_full);
	}
	fclose(fp);

	return (true);
}

bool
blocklist_refresh(void)
{
	struct stat st;

	if (filename[0] == '\0')
		return (false);
	if (stat(filename, &st) != 0) {
		fprintf(stderr, "Error refreshing blocklist: stat failed: "
		    "%s\n", strerror(errno));
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
blocklist_check(const void *addr, socklen_t addr_len, int addr_family)
{
	block_addr_t ba = { .addr_len = addr_len, .addr_family = addr_family };

	ASSERT(addr != NULL);
	ASSERT(addr_family == AF_INET || addr_family == AF_INET6);

	memcpy(ba.addr, addr, addr_len);
	/*
	 * Before we search, we need to sanitize the lookup to have a
	 * zero port number, otherwise we won't generate an exact match.
	 */
	if (addr_family == AF_INET)
		((struct sockaddr_in *)ba.addr)->sin_port = 0;
	else
		((struct sockaddr_in6 *)ba.addr)->sin6_port = 0;
	return (avl_find(&tree, &ba, NULL) == NULL);
}

