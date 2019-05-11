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

typedef struct cpdlc_msg_thr_s {
	struct cpdlc_msg_thr_s	*next;
	struct cpdlc_msg_thr_s	*prev;
} cpdlc_msg_thr_t;

typedef struct {
	cpdlc_client_t	*cl;
	mutex_t		lock;

	cpdlc_msg_thr_t	*thr_head;
	cpdlc_msg_thr_t	*thr_tail;

	unsigned	min;
	unsigned	mrn;
} cpdlc_msglist_t;

cpdlc_msglist_t *
cpdlc_msglist_alloc(cpdlc_client_t *cl)
{
	cpdlc_msglist_t *mlist = safe_calloc(1, sizeof (*mlist));

	mutex_init(&mlist->lock);

	return (mlist);
}

void
cpdlc_msglist_free(cpdlc_msglist_t *msglist)
{
	mutex_destroy(&mlist->lock);
	free(mlist);
}
