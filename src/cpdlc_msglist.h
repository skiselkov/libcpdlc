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

#ifndef	_LIBCPDLC_MSGLIST_H_
#define	_LIBCPDLC_MSGLIST_H_

#include <stdint.h>

#include "cpdlc_client.h"
#include "cpdlc_msg.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct cpdlc_msglist_s cpdlc_msglist_t;

#define	CPDLC_NO_MSG_THR_ID	UINT32_MAX
typedef uint32_t cpdlc_msg_thr_id_t;

typedef void (*cpdlc_msglist_update_cb_t)(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t *updated_threads, unsigned num_updated_threads);
typedef void (*cpdlc_get_time_func_t)(void *userinfo, unsigned *hours,
    unsigned *mins);

typedef enum {
	CPDLC_MSG_THR_OPEN,
	CPDLC_MSG_THR_CLOSED,
	CPDLC_MSG_THR_ACCEPTED,
	CPDLC_MSG_THR_REJECTED,
	CPDLC_MSG_THR_TIMEDOUT,
	CPDLC_MSG_THR_STANDBY,
	CPDLC_MSG_THR_FAILED,
	CPDLC_MSG_THR_PENDING,
	CPDLC_MSG_THR_DISREGARD,
	CPDLC_MSG_THR_ERROR
} cpdlc_msg_thr_status_t;

CPDLC_API cpdlc_msglist_t *cpdlc_msglist_alloc(cpdlc_client_t *cl);
CPDLC_API void cpdlc_msglist_free(cpdlc_msglist_t *msglist);

CPDLC_API void cpdlc_msglist_update(cpdlc_msglist_t *msglist);

CPDLC_API cpdlc_msg_thr_id_t cpdlc_msglist_send(cpdlc_msglist_t *msglist,
    cpdlc_msg_t *msg, cpdlc_msg_thr_id_t thr_id);
CPDLC_API void cpdlc_msglist_get_thr_ids(cpdlc_msglist_t *msglist,
    bool ignore_closed, cpdlc_msg_thr_id_t *thr_ids, unsigned *cap);
CPDLC_API bool cpdlc_msglist_thr_is_done(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t thr_id);
CPDLC_API void cpdlc_msglist_thr_close(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t thr_id);
CPDLC_API void cpdlc_msglist_remove_thr(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t thr_id);

CPDLC_API cpdlc_msg_thr_status_t cpdlc_msglist_get_thr_status(
    cpdlc_msglist_t *msglist, cpdlc_msg_thr_id_t thr_id, bool *dirty);
CPDLC_API void cpdlc_msglist_thr_mark_seen(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t thr_id);
CPDLC_API unsigned cpdlc_msglist_get_thr_msg_count(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t thr_id);
CPDLC_API void cpdlc_msglist_get_thr_msg(cpdlc_msglist_t *msglist,
    cpdlc_msg_thr_id_t thr_id, unsigned msg_nr, const cpdlc_msg_t **msg_p,
    cpdlc_msg_token_t *token_p, unsigned *hours_p, unsigned *mins_p,
    bool *is_sent_p);

CPDLC_API void cpdlc_msglist_set_userinfo(cpdlc_msglist_t *msglist,
    void *userinfo);
CPDLC_API void *cpdlc_msglist_get_userinfo(cpdlc_msglist_t *msglist);

CPDLC_API void cpdlc_msglist_set_update_cb(cpdlc_msglist_t *msglist,
    cpdlc_msglist_update_cb_t update_cb);
CPDLC_API void cpdlc_msglist_set_get_time_func(cpdlc_msglist_t *msglist,
    cpdlc_get_time_func_t func);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_MSGLIST_H_ */
