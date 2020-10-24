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
#include <string.h>
#include <stdio.h>

#include "cpdlc_alloc.h"
#include "cpdlc_assert.h"
#include "cpdlc_string.h"

#include "fans_impl.h"
#include "fans_text_proc.h"

#define	ADD_LINE(__lines, __n_lines, __start, __len, __line_prefix) \
	do { \
		unsigned prefix_l = strlen(__line_prefix); \
		unsigned l = (__len) + prefix_l + 1; \
		(__lines) = safe_realloc((__lines), \
		    ((__n_lines) + 1) * sizeof (*(__lines))); \
		(__lines)[(__n_lines)] = safe_calloc(1, l); \
		cpdlc_strlcpy((__lines)[(__n_lines)], (__line_prefix), l); \
		cpdlc_strlcpy(&(__lines)[(__n_lines)][prefix_l], (__start), \
		    l - prefix_l); \
		(__n_lines)++; \
	} while (0)

void
fans_msg2lines(const cpdlc_msg_t *msg, char ***lines_p, unsigned *n_lines_p,
    const char *line_prefix, unsigned max_line_len)
{
	char buf[1024];
	const char *start, *cur, *end, *last_sp;

	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(lines_p != NULL);
	CPDLC_ASSERT(n_lines_p != NULL);
	CPDLC_ASSERT(line_prefix != NULL);
	CPDLC_ASSERT(max_line_len != 0);

	if (line_prefix == NULL)
		line_prefix = "";

	cpdlc_msg_readable(msg, buf, sizeof (buf));
	last_sp = strchr(buf, ' ');
	for (start = buf, cur = buf, end = buf + strlen(buf);; cur++) {
		if (last_sp == NULL)
			last_sp = end;
		if (cur == end) {
			ADD_LINE(*lines_p, *n_lines_p, start, cur - start,
			    line_prefix);
			break;
		}
		if (cur - start >= max_line_len) {
			ADD_LINE(*lines_p, *n_lines_p, start, last_sp - start,
			    line_prefix);
			if (last_sp == end)
				break;
			start = last_sp + 1;
			last_sp = strchr(start, ' ');
		} else if (isspace(cur[0])) {
			last_sp = cur;
		}
	}
}

static bool
is_short_response(const cpdlc_msg_t *msg)
{
	int msg_type = msg->segs[0].info->msg_type;
	return (msg_type >= CPDLC_DM0_WILCO && msg_type <= CPDLC_DM5_NEGATIVE);
}

void
fans_thr2lines(cpdlc_msglist_t *msglist, cpdlc_msg_thr_id_t thr_id,
    char ***lines_p, unsigned *n_lines_p, const char *line_prefix)
{
	CPDLC_ASSERT(msglist != NULL);
	CPDLC_ASSERT(thr_id != CPDLC_NO_MSG_THR_ID);
	CPDLC_ASSERT(lines_p != NULL);
	CPDLC_ASSERT(n_lines_p != NULL);

	for (unsigned i = 0, n = cpdlc_msglist_get_thr_msg_count(msglist,
	    thr_id); i < n; i++) {
		const cpdlc_msg_t *msg;
		bool sent;

		cpdlc_msglist_get_thr_msg(msglist, thr_id, i, &msg, NULL,
		    NULL, NULL, &sent);
		/*
		 * Skip the "WILCO" or "STANDBY" messages we sent. Those will
		 * show up as message status at the bottom of the page.
		 */
		if (sent && is_short_response(msg))
			continue;
		if (i > 0) {
			ADD_LINE(*lines_p, *n_lines_p,
			    "------------------------", 24, line_prefix);
		}
		fans_msg2lines(msg, lines_p, n_lines_p, line_prefix, FMS_COLS);
	}
}

void
fans_free_lines(char **lines, unsigned n_lines)
{
	for (unsigned i = 0; i < n_lines; i++)
		free(lines[i]);
	free(lines);
}
