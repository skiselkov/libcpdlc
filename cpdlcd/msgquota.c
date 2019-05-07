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
#include <stdlib.h>

#include <acfutils/assert.h>
#include <acfutils/avl.h>
#include <acfutils/helpers.h>
#include <acfutils/safe_alloc.h>

#include "common.h"
#include "msgquota.h"

typedef struct {
	char		callsign[CALLSIGN_LEN];
	uint64_t	bytes;
	avl_node_t	node;
} msgquota_t;

static bool		inited = false;
static avl_tree_t	tree;
static uint64_t		msgquota_max = 128 << 10;	/* 128 KiB */

static int
msgquota_compar(const void *a, const void *b)
{
	const msgquota_t *mqa = a, *mqb = b;
	int res = strcmp(mqa->callsign, mqb->callsign);
	if (res < 0)
		return (-1);
	if (res > 0)
		return (1);
	return (0);
}

void
msgquota_init(uint64_t max_bytes)
{
	ASSERT(!inited);
	inited = true;

	avl_create(&tree, msgquota_compar, sizeof (msgquota_t),
	    offsetof(msgquota_t, node));
	msgquota_max = max_bytes;
}

void
msgquota_fini(void)
{
	msgquota_t *mq;
	void *cookie = NULL;

	if (!inited)
		return;
	inited = false;

	while ((mq = avl_destroy_nodes(&tree, &cookie)) != NULL)
		free(mq);
	avl_destroy(&tree);
}

static msgquota_t *
mq_get(const char *callsign)
{
	msgquota_t srch;
	msgquota_t *mq;
	avl_index_t where;

	ASSERT(callsign != NULL);

	lacf_strlcpy(srch.callsign, callsign, sizeof (srch.callsign));
	mq = avl_find(&tree, &srch, &where);
	if (mq == NULL) {
		mq = safe_calloc(1, sizeof (*mq));
		lacf_strlcpy(mq->callsign, callsign, sizeof (mq->callsign));
		avl_insert(&tree, mq, where);
	}

	return (mq);
}

bool
msgquota_incr(const char *callsign, uint64_t bytes)
{
	msgquota_t *mq;

	ASSERT(callsign != 0);
	ASSERT(bytes > 0);

	mq = mq_get(callsign);
	if (msgquota_max > 0 && mq->bytes + bytes > msgquota_max)
		return (false);
	mq->bytes += bytes;
	return (true);
}

void
msgquota_decr(const char *callsign, uint64_t bytes)
{
	msgquota_t *mq;

	ASSERT(callsign != 0);
	ASSERT(bytes > 0);

	mq = mq_get(callsign);
	ASSERT3U(mq->bytes, >=, bytes);
	mq->bytes -= bytes;
	if (mq->bytes == 0) {
		avl_remove(&tree, mq);
		free(mq);
	}
}
