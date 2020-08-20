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

#ifndef	_LIBCPDLC_FANS_TEXT_PROC_H_
#define	_LIBCPDLC_FANS_TEXT_PROC_H_

#include <cpdlc_msglist.h>

#ifdef	__cplusplus
extern "C" {
#endif

const char *fans_thr_status2str(cpdlc_msg_thr_status_t st, bool dirty);
void fans_msg2lines(const cpdlc_msg_t *msg, char ***lines_p,
    unsigned *n_lines_p);
void fans_thr2lines(cpdlc_msglist_t *msglist, cpdlc_msg_thr_id_t thr_id,
    char ***lines_p, unsigned *n_lines_p);
void fans_free_lines(char **lines, unsigned n_lines);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FANS_TEXT_PROC_H_ */
