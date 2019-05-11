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

#include <stdlib.h>

#include "cpdlc_alloc.h"
#include "cpdlc_assert.h"
#include "cpdlc_msglist.h"
#include "cpdlc_thread.h"

typedef struct msg_bucket_s {
	cpdlc_msg_t		*msg;
	cpdlc_msg_token_t	tok;

	struct msg_bucket_t	*next;
	struct msg_bucket_t	*prev;
} msg_bucket_t;

typedef struct msg_thr_s {
	cpdlc_msg_thr_id_t	thr_id;
	cpdlc_msg_thr_status_t	status;

	msg_bucket_t		*buckets_head;
	msg_bucket_t		*buckets_tail;
	unsigned		num_msgs;

	struct msg_thr_s	*next;
	struct msg_thr_s	*prev;
} msg_thr_t;

typedef struct {
	cpdlc_client_t	*cl;
	mutex_t		lock;

	msg_thr_t	*thr_head;
	msg_thr_t	*thr_tail;

	unsigned	min;
	unsigned	mrn;
	msg_thr_t	next_mthr;
} cpdlc_msglist_t;

static msg_thr_t *
find_msg_thr(cpdlc_msglist_t *msglist, cpdlc_msg_thr_id_t thr_id)
{
	ASSERT(msglist != NULL);

	if (thr_id != CPDLC_NO_MSG_THR_ID) {
		for (msg_thr_t *thr = msglist->thr_head; thr != NULL;
		    thr = thr->next) {
			if (thr->thr_id == thr_id)
				return (thr);
		}
		VERIFY_MSG(0, "Invalid message thread ID %x", mthr);
	} else {
		msg_thr_t *thr = safe_calloc(1, sizeof (*thr));
		thr->thr_id = msglist->next_mthr++;
		LIST_INSERT_TAIL(msglist->thr_head, msglist->thr_tail, thr);
		return (thr);
	}
}

static void
free_msg_thr(cpdlc_msglist_t *msglist, msg_thr_t *thr)
{
	ASSERT(msglist != NULL);
	ASSERT(thr != NULL);

	for (msg_bucket_t *bucket = thr->buckets_head;
	    thr->buckets_head != NULL; bucket = thr->buckets_head) {
		LIST_REMOVE(thr->buckets_head, thr->buckets_tail, bucket);
		ASSERT(bucket->msg != NULL);
		cpdlc_msg_free(bucket->msg);
		free(bucket);
	}

	LIST_REMOVE(msglist->thr_head, msglist->thr_tail, thr);
	free(thr);
}

cpdlc_msglist_t *
cpdlc_msglist_alloc(cpdlc_client_t *cl)
{
	cpdlc_msglist_t *msglist = safe_calloc(1, sizeof (*mlist));

	ASSERT(cl != NULL);

	mutex_init(&msglist->lock);
	msglist->cl = cl;

	return (msglist);
}

void
cpdlc_msglist_free(cpdlc_msglist_t *msglist)
{
	ASSERT(msglist != NULL);

	while (msglist->thr_head != NULL)
		free_msg_thr(msglist, msglist->thr_head);

	mutex_destroy(&mlist->lock);
	free(mlist);
}

msg_thr_t
cpdlc_msglist_send(cpdlc_msglist_t *msglist, cpdlc_msg_t *msg,
    cpdlc_msg_thr_id_t thr_id)
{
	msg_thr_t	*thr;
	msg_bucket_t	*bucket;

	ASSERT(msglist != NULL);
	ASSERT(msg != NULL);

	mutex_enter(&msglist->lock);

	thr = find_msg_thr(msglist, thr_id);

	/* Assign the appropriate MIN and MRN flags */
	for (msg_bucket_t *bucket = bucket->buckets_tail; bucket != NULL;
	    bucket = bucket->prev) {
		if (cpdlc_msg_get_dl(bucket->msg) != cpdlc_msg_get_dl(msg)) {
			cpdlc_msg_set_mrn(msg, cpdlc_msg_get_min(bucket->msg));
			break;
		}
	}
	cpdlc_msg_set_min(msg, msglist->min++);

	bucket = safe_calloc(1, sizeof (*bucket));
	bucket->msg = msg;
	bucket->tok = cpdlc_client_send_msg(msglist->cl, msg);
	LIST_INSERT_TAIL(thr->buckets_head, thr->buckets_tail, bucket);
	thr->num_msgs++;

	mutex_exit(&msglist->lock);
}

cpdlc_msg_thr_status_t
cpdlc_msglist_get_thr_status(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t thr_id)
{
	msg_thr_t		*thr;
	cpdlc_msg_thr_status_t	status;

	ASSERT(msglist != NULL);
	ASSERT(thr_id != CPDLC_NO_MSG_THR_ID);

	mutex_enter(&msglist->lock);
	thr = find_msg_thr(msglist, thr_id);
	status = thr->status;
	mutex_exit(&msglist->lock);

	return (status);
}

unsigned
cpdlc_msglist_get_thr_count(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t thr_id)
{
	unsigned count;
	msg_thr_t *thr;

	ASSERT(msglist != NULL);
	ASSERT(thr_id != CPDLC_NO_MSG_THR_ID);

	mutex_enter(&msglist->lock);
	thr = find_msg_thr(msglist, thr_id);
	count = thr->num_msgs;
	mutex_exit(&msglist->lock);

	return (count);
}

void
cpdlc_msglist_get_thr_msg(cpdlc_msglist_t *msglist, cpdlc_msg_thr_id_t thr_id,
    unsigned msg_nr, const cpdlc_msg_t **msg_p, cpdlc_msg_token_t *token_p)
{
	msg_thr_t *thr;
	msg_bucket_t *bucket;

	ASSERT(msglist != NULL);
	ASSERT(thr_id != CPDLC_NO_MSG_THR_ID);

	mutex_enter(&msglist->lock);

	thr = find_msg_thr(msglist, thr_id);
	ASSERT3U(msg_nr, <, thr->num_msgs);
	bucket = thr->buckets_head;
	for (unsigned i = 0; i < msg_nr; i++, bucket = bucket->next)
		;
	ASSERT(bucket != NULL);
	if (msg_p != NULL)
		*msg_p = bucket->msg;
	if (token_p != NULL)
		*token_p = bucket->tok;

	mutex_exit(&msglist->lock);
}

void
cpdlc_msglist_remove_thr(cpdlc_msglist_t *msglist, cpdlc_msg_thr_id_t thr_id)
{
	msg_thr_t *thr;

	ASSERT(msglist != NULL);
	ASSERT(thr_id != CPDLC_NO_MSG_THR_ID);

	mutex_enter(&msglist->lock);
	free_msg_thr(msglist, find_msg_thr(msglist, thr_id));
	mutex_exit(&msglist->lock);
}
