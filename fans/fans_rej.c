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

#include <math.h>

#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_string.h"

#include "fans_impl.h"
#include "fans_scratchpad.h"

void
fans_rej(fans_t *box, bool unable_or_neg, unsigned ret_page)
{
	ASSERT(box != NULL);
	ASSERT(box->thr_id != CPDLC_NO_MSG_THR_ID);
	ASSERT3U(ret_page, <, FMS_NUM_PAGES);
	ASSERT(ret_page != FMS_PAGE_REJ);

	memset(&box->rej, 0, sizeof (box->rej));
	box->rej.unable_or_neg = unable_or_neg;
	box->rej.ret_page = ret_page;
	fans_set_page(box, FMS_PAGE_REJ, true);
}

void
fans_rej_draw_cb(fans_t *box)
{
	enum { MAX_LINES = 8 };
	char **lines = NULL;
	unsigned n_lines = 0;

	ASSERT(box != NULL);

	fans_put_page_title(box, "CPDLC REJECT");

	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "DUE TO");
	fans_put_altn_selector(box, LSK1_ROW, false, box->rej.due,
	    "NONE", "WX", "AC", "UNLOADABLE", NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "REMARKS");
	for (int i = 0; i < REJ_FREETEXT_LINES; i++) {
		if (strlen(box->rej.freetext[i]) != 0) {
			fans_put_str(box, LSKi_ROW(i + 1), 0, false,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE, "%s",
			    box->rej.freetext[i]);
		} else {
			fans_put_str(box, LSKi_ROW(i + 1), 0, false,
			    FMS_COLOR_CYAN, FMS_FONT_LARGE,
			    "------------------------");
		}
	}

	for (int i = 0; i < MAX_LINES; i++) {
		int line = i + box->subpage * MAX_LINES;

		if (line >= (int)n_lines)
			break;
		fans_put_str(box, LSK1_ROW + i, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", lines[line]);
	}

	fans_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN, "SEND*");
	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_rej_key_cb(fans_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L1) {
		box->rej.due = (box->rej.due + 1) %
		    (REJ_DUE_UNLOADABLE + 1);
	} else if (key >= FMS_KEY_LSK_L2 && key <= FMS_KEY_LSK_L4) {
		fans_scratchpad_xfer(box,
		    box->rej.freetext[key - FMS_KEY_LSK_L2],
		    sizeof (box->rej.freetext[key - FMS_KEY_LSK_L2]), true);
	} else if (key == FMS_KEY_LSK_R5) {
		cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
		unsigned seg;
		char freetext[sizeof (box->rej.freetext)] = { 0 };

		if (box->rej.unable_or_neg) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM1_UNABLE, 0);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM5_NEGATIVE, 0);
		}
		switch (box->rej.due) {
		case REJ_DUE_NONE:
			break;
		case REJ_DUE_WX:
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM65_DUE_TO_WX, 0);
			break;
		case REJ_DUE_AC:
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM66_DUE_TO_ACFT_PERF, 0);
			break;
		case REJ_DUE_UNLOADABLE:
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM67_FREETEXT_NORMAL_text, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    "DUE TO DATA UNLOADABLE", NULL);
			break;
		}
		for (int i = 0; i < REJ_FREETEXT_LINES; i++)
			strcat(freetext, box->rej.freetext[i]);
		if (strlen(freetext) != 0) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM67_FREETEXT_NORMAL_text, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0, freetext, NULL);
		}

		ASSERT(box->thr_id != CPDLC_NO_MSG_THR_ID);
		cpdlc_msglist_send(box->msglist, msg, box->thr_id);

		fans_set_page(box, box->rej.ret_page, false);
	} else if (key == FMS_KEY_LSK_L6) {
		fans_set_page(box, box->rej.ret_page, false);
	} else {
		return (false);
	}

	return (true);
}
