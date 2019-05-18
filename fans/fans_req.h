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

#ifndef	_LIBCPDLC_FANS_REQ_H_
#define	_LIBCPDLC_FANS_REQ_H_

#include "fans_impl.h"

#ifdef	__cplusplus
extern "C" {
#endif

void fans_requests_draw_cb(fans_t *box);
bool fans_requests_key_cb(fans_t *box, fms_key_t key);

void fans_req_add_common(fans_t *box, cpdlc_msg_t *msg);

void fans_req_draw_due(fans_t *box, bool due_tfc);
void fans_req_key_due(fans_t *box, fms_key_t key);

#define	KEY_IS_REQ_FREETEXT(__box, __key, __tgt_subpage) \
	((__box)->subpage == (__tgt_subpage) && (__key) >= FMS_KEY_LSK_L1 && \
	    (__key) <= FMS_KEY_LSK_L4)
#define	KEY_IS_REQ_STEP_AT(__box, __key) \
	((__box)->subpage == 0 && \
	    ((__key) == FMS_KEY_LSK_R1 || (__key) == FMS_KEY_LSK_R2))
void fans_req_draw_freetext(fans_t *box);
void fans_req_key_freetext(fans_t *box, fms_key_t key);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FANS_REQ_H_ */
