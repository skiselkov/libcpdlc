/*
 * Copyright 2023 Saso Kiselkov
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

#include <string.h>

#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_alloc.h"

#include "fans_req.h"
#include "fans_scratchpad.h"

void
fans_requests_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_put_page_title(box, "FANS  REQUESTS");

	fans_put_lsk_action(box, FMS_KEY_LSK_L1, FMS_COLOR_WHITE,
	    "<ALTITUDE");
	fans_put_lsk_action(box, FMS_KEY_LSK_L2, FMS_COLOR_WHITE,
	    "<OFFSET");
	fans_put_lsk_action(box, FMS_KEY_LSK_L3, FMS_COLOR_WHITE,
	    "<SPEED");
	fans_put_lsk_action(box, FMS_KEY_LSK_L4, FMS_COLOR_WHITE,
	    "<ROUTE");
	fans_put_lsk_action(box, FMS_KEY_LSK_R1, FMS_COLOR_WHITE,
	    "CLEARANCE>");
	fans_put_lsk_action(box, FMS_KEY_LSK_R2, FMS_COLOR_WHITE,
	    "VMC DESCENT>");
	fans_put_lsk_action(box, FMS_KEY_LSK_R3, FMS_COLOR_WHITE,
	    "WHEN CAN WE>");
	fans_put_lsk_action(box, FMS_KEY_LSK_R4, FMS_COLOR_WHITE,
	    "VOICE REQ>");
}

bool
fans_requests_key_cb(fans_t *box, fms_key_t key)
{
	enum { MAX_LINES = 4 };

	CPDLC_ASSERT(box != NULL);

	memset(&box->req_common, 0, sizeof (box->req_common));

	switch (key) {
	case FMS_KEY_LSK_L1:
		fans_set_page(box, FMS_PAGE_REQ_ALT, true);
		break;
	case FMS_KEY_LSK_L2:
		fans_set_page(box, FMS_PAGE_REQ_OFF, true);
		break;
	case FMS_KEY_LSK_L3:
		fans_set_page(box, FMS_PAGE_REQ_SPD, true);
		break;
	case FMS_KEY_LSK_L4:
		fans_set_page(box, FMS_PAGE_REQ_RTE, true);
		break;
	case FMS_KEY_LSK_R1:
		fans_set_page(box, FMS_PAGE_REQ_CLX, true);
		break;
	case FMS_KEY_LSK_R2:
		fans_set_page(box, FMS_PAGE_REQ_VMC, true);
		break;
	case FMS_KEY_LSK_R3:
		fans_set_page(box, FMS_PAGE_REQ_WCW, true);
		break;
	case FMS_KEY_LSK_R4:
		fans_set_page(box, FMS_PAGE_REQ_VOICE, true);
		break;
	default:
		return (false);
	}

	return (true);
}

void
fans_req_add_common(fans_t *box, cpdlc_msg_t *msg, const char *extra_prepend)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(msg != NULL);
	/* extra_prepend can be NULL */

	if (box->req_common.due_wx) {
		cpdlc_msg_add_seg(msg, true, CPDLC_DM65_DUE_TO_WX, 0);
	} else if (box->req_common.due_ac) {
		cpdlc_msg_add_seg(msg, true, CPDLC_DM66_DUE_TO_ACFT_PERF, 0);
	} else if (box->req_common.due_tfc) {
		unsigned seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM67_FREETEXT_NORMAL_text, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, "DUE TO TRAFFIC", NULL);
	}
	if (box->req_common.freetext[0][0] != '\0' ||
	    box->req_common.freetext[1][0] != '\0' ||
	    box->req_common.freetext[2][0] != '\0' ||
	    box->req_common.freetext[3][0] != '\0' ||
	    extra_prepend != NULL) {
		unsigned prepend_len = (extra_prepend != NULL ?
		    strlen(extra_prepend) : 0) + 1;
		unsigned bufsz = sizeof (box->req_common.freetext) +
		    REQ_FREETEXT_LINES + prepend_len + 1;
		char *buf = safe_malloc(bufsz);
		unsigned seg;

		memset(buf, 0, bufsz);
		if (extra_prepend != NULL) {
			strcat(buf, extra_prepend);
			strcat(buf, " ");
		}
		for (int i = 0; i < REQ_FREETEXT_LINES; i++) {
			if (box->req_common.freetext[i][0] != '\0')
				strcat(buf, box->req_common.freetext[i]);
		}
		if (box->req_common.distress) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM68_FREETEXT_DISTRESS_text, 0);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM67_FREETEXT_NORMAL_text, 0);
		}
		cpdlc_msg_seg_set_arg(msg, seg, 0, buf, NULL);
		free(buf);
	}
}

void
fans_req_draw_due(fans_t *box, bool due_tfc)
{
	CPDLC_ASSERT(box != NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "DUE TO WX");
	fans_put_altn_selector(box, LSK2_ROW, false, box->req_common.due_wx,
	    "NO", "YES", NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "DUE TO A/C");
	fans_put_altn_selector(box, LSK3_ROW, false, box->req_common.due_ac,
	    "NO", "YES", NULL);

	if (due_tfc) {
		fans_put_lsk_title(box, FMS_KEY_LSK_L4, "DUE TO TRAFFIC");
		fans_put_altn_selector(box, LSK4_ROW, false,
		    box->req_common.due_tfc, "NO", "YES", NULL);
	}
}

void
fans_req_key_due(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(key >= FMS_KEY_LSK_L2 && key <= FMS_KEY_LSK_L4);
	if (key == FMS_KEY_LSK_L2) {
		box->req_common.due_wx = !box->req_common.due_wx;
		box->req_common.due_ac = false;
		box->req_common.due_tfc = false;
	} else if (key == FMS_KEY_LSK_L3) {
		box->req_common.due_wx = false;
		box->req_common.due_ac = !box->req_common.due_ac;
		box->req_common.due_tfc = false;
	} else {
		box->req_common.due_wx = false;
		box->req_common.due_ac = false;
		box->req_common.due_tfc = !box->req_common.due_tfc;
	}
}

void
fans_req_draw_freetext(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "REMARKS");

	for (int i = 0; i < REQ_FREETEXT_LINES; i++) {
		if (box->req_common.freetext[i][0] != '\0') {
			fans_put_str(box, LSKi_ROW(i), 0, false,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE, "%s",
			    box->req_common.freetext[i]);
		} else {
			fans_put_str(box, LSKi_ROW(i), 0, false,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE,
			    "------------------------");
		}
	}
}

void
fans_req_key_freetext(fans_t *box, fms_key_t key)
{
	int line;
	bool read_back;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(key >= FMS_KEY_LSK_L1 && key <= FMS_KEY_LSK_L4);

	line = key - FMS_KEY_LSK_L1;
	if (fans_scratchpad_xfer(box, box->req_common.freetext[line],
	    sizeof (box->req_common.freetext[line]), true, &read_back) &&
	    !read_back) {
		fans_scratchpad_clear(box);
	}
}
