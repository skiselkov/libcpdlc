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
#include "minilist.h"

typedef struct msg_bucket_s {
	cpdlc_msg_t		*msg;
	cpdlc_msg_token_t	tok;
	bool			sent;
	list_node_t		node;
} msg_bucket_t;

typedef struct msg_thr_s {
	cpdlc_msg_thr_id_t	thr_id;
	cpdlc_msg_thr_status_t	status;
	list_t			buckets;
	list_node_t		node;
} msg_thr_t;

struct cpdlc_msglist_s {
	cpdlc_client_t			*cl;
	mutex_t				lock;

	list_t				thr;

	unsigned			min;
	unsigned			mrn;
	cpdlc_msg_thr_id_t		next_thr_id;

	cpdlc_msglist_update_cb_t	update_cb;
	void				*userinfo;
};

static msg_thr_t *
find_msg_thr(cpdlc_msglist_t *msglist, cpdlc_msg_thr_id_t thr_id)
{
	ASSERT(msglist != NULL);

	if (thr_id != CPDLC_NO_MSG_THR_ID) {
		for (msg_thr_t *thr = list_head(&msglist->thr); thr != NULL;
		    thr = list_next(&msglist->thr, thr)) {
			if (thr->thr_id == thr_id)
				return (thr);
		}
		VERIFY_MSG(0, "Invalid message thread ID %x", thr_id);
	} else {
		msg_thr_t *thr = safe_calloc(1, sizeof (*thr));
		thr->thr_id = msglist->next_thr_id++;
		list_create(&thr->buckets, sizeof (msg_bucket_t),
		    offsetof(msg_bucket_t, node));
		list_insert_tail(&msglist->thr, thr);
		return (thr);
	}
}

static void
free_msg_thr(msg_thr_t *thr)
{
	msg_bucket_t *bucket;

	ASSERT(thr != NULL);

	while ((bucket = list_remove_head(&thr->buckets)) != NULL) {
		ASSERT(bucket->msg != NULL);
		cpdlc_msg_free(bucket->msg);
		free(bucket);
	}
	list_destroy(&thr->buckets);
	free(thr);
}

msg_thr_t *
msg_thr_find_by_mrn(cpdlc_msglist_t *msglist, unsigned mrn)
{
	ASSERT(msglist != NULL);
	for (msg_thr_t *thr = list_tail(&msglist->thr); thr != NULL;
	    thr = list_prev(&msglist->thr, thr)) {
		for (msg_bucket_t *bucket = list_tail(&thr->buckets);
		    bucket != NULL; bucket = list_prev(&thr->buckets, bucket)) {
			ASSERT(bucket->msg != NULL);
			if (!bucket->sent &&
			    cpdlc_msg_get_min(bucket->msg) == mrn) {
				return (thr);
			}
		}
	}
	return (NULL);
}

static void
msg_recv_cb(cpdlc_client_t *cl)
{
	cpdlc_msglist_t *msglist;
	cpdlc_msg_t *msg;
	cpdlc_msg_thr_id_t *upd_thrs = NULL;
	unsigned num_upd_thrs = 0;
	cpdlc_msglist_update_cb_t update_cb;

	ASSERT(cl != NULL);
	msglist = cpdlc_client_get_cb_userinfo(cl);
	ASSERT(msglist != NULL);

	mutex_enter(&msglist->lock);

	update_cb = msglist->update_cb;

	while ((msg = cpdlc_client_recv_msg(cl)) != NULL) {
		msg_thr_t *thr = msg_thr_find_by_mrn(msglist,
		    cpdlc_msg_get_mrn(msg));
		msg_bucket_t *bucket;

		if (thr == NULL)
			thr = find_msg_thr(msglist, CPDLC_NO_MSG_THR_ID);
		bucket = safe_calloc(1, sizeof (*bucket));
		bucket->msg = msg;
		bucket->tok = CPDLC_INVALID_MSG_TOKEN;

		list_insert_tail(&thr->buckets, bucket);

		if (update_cb != NULL) {
			upd_thrs = safe_realloc(upd_thrs, (num_upd_thrs + 1) *
			    sizeof (*upd_thrs));
			upd_thrs[num_upd_thrs] = thr->thr_id;
			num_upd_thrs++;
		}
	}
	mutex_exit(&msglist->lock);
	/*
	 * Call this outside of locking context to avoid locking inversions.
	 */
	if (update_cb != NULL) {
		update_cb(msglist, upd_thrs, num_upd_thrs);
		free(upd_thrs);
	}
}

cpdlc_msglist_t *
cpdlc_msglist_alloc(cpdlc_client_t *cl)
{
	cpdlc_msglist_t *msglist = safe_calloc(1, sizeof (*msglist));

	ASSERT(cl != NULL);

	cpdlc_client_set_msg_recv_cb(cl, msg_recv_cb);
	cpdlc_client_set_cb_userinfo(cl, msglist);

	mutex_init(&msglist->lock);
	list_create(&msglist->thr, sizeof (msg_thr_t),
	    offsetof(msg_thr_t, node));
	msglist->cl = cl;

	return (msglist);
}

void
cpdlc_msglist_free(cpdlc_msglist_t *msglist)
{
	msg_thr_t *thr;

	ASSERT(msglist != NULL);

	while ((thr = list_remove_head(&msglist->thr)) != NULL)
		free_msg_thr(thr);
	mutex_destroy(&msglist->lock);
	free(msglist);
}

cpdlc_msg_thr_id_t
cpdlc_msglist_send(cpdlc_msglist_t *msglist, cpdlc_msg_t *msg,
    cpdlc_msg_thr_id_t thr_id)
{
	msg_thr_t		*thr;
	msg_bucket_t		*bucket;

	ASSERT(msglist != NULL);
	ASSERT(msg != NULL);

	mutex_enter(&msglist->lock);

	thr = find_msg_thr(msglist, thr_id);
	thr_id = thr->thr_id;

	/* Assign the appropriate MIN and MRN flags */
	for (msg_bucket_t *bucket = list_tail(&thr->buckets); bucket != NULL;
	    bucket = list_prev(&thr->buckets, bucket)) {
		if (cpdlc_msg_get_dl(bucket->msg) != cpdlc_msg_get_dl(msg)) {
			cpdlc_msg_set_mrn(msg, cpdlc_msg_get_min(bucket->msg));
			break;
		}
	}
	cpdlc_msg_set_min(msg, msglist->min++);

	bucket = safe_calloc(1, sizeof (*bucket));
	bucket->msg = msg;
	bucket->tok = cpdlc_client_send_msg(msglist->cl, msg);
	bucket->sent = true;
	list_insert_tail(&thr->buckets, bucket);

	mutex_exit(&msglist->lock);

	return (thr_id);
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
cpdlc_msglist_get_thr_msg_count(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t thr_id)
{
	unsigned count;
	msg_thr_t *thr;

	ASSERT(msglist != NULL);
	ASSERT(thr_id != CPDLC_NO_MSG_THR_ID);

	mutex_enter(&msglist->lock);
	thr = find_msg_thr(msglist, thr_id);
	count = list_count(&thr->buckets);
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
	ASSERT3U(msg_nr, <, list_count(&thr->buckets));
	bucket = list_head(&thr->buckets);
	for (unsigned i = 0; i < msg_nr; i++)
		bucket = list_next(&thr->buckets, bucket);
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
	thr = find_msg_thr(msglist, thr_id);
	list_remove(&msglist->thr, thr);
	mutex_exit(&msglist->lock);

	free_msg_thr(thr);
}

void
cpdlc_msglist_set_userinfo(cpdlc_msglist_t *msglist, void *userinfo)
{
	ASSERT(msglist != NULL);
	mutex_enter(&msglist->lock);
	msglist->userinfo = userinfo;
	mutex_exit(&msglist->lock);
}

void *
cpdlc_msglist_get_userinfo(cpdlc_msglist_t *msglist)
{
	ASSERT(msglist != NULL);
	/* Reading a pointer is atomic, no locking req'd */
	return (msglist->userinfo);
}

void
cpdlc_msglist_set_update_cb(cpdlc_msglist_t *msglist,
    cpdlc_msglist_update_cb_t update_cb)
{
	ASSERT(msglist != NULL);
	mutex_enter(&msglist->lock);
	msglist->update_cb = update_cb;
	mutex_exit(&msglist->lock);
}
