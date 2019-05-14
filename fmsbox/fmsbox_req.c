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

#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fmsbox_req.h"
#include "fmsbox_scratchpad.h"

void
fmsbox_requests_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_put_page_title(box, "FANS  REQUESTS");

	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L1, FMS_COLOR_WHITE,
	    "<ALTITUDE");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L2, FMS_COLOR_WHITE,
	    "<OFFSET");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L3, FMS_COLOR_WHITE,
	    "<SPEED");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L4, FMS_COLOR_WHITE,
	    "<ROUTE");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_R1, FMS_COLOR_WHITE,
	    "CLEARANCE>");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_R2, FMS_COLOR_WHITE,
	    "VMC DESCENT>");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_R3, FMS_COLOR_WHITE,
	    "WHEN CAN WE>");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_R4, FMS_COLOR_WHITE,
	    "VOICE REQ>");
}

bool
fmsbox_requests_key_cb(fmsbox_t *box, fms_key_t key)
{
	enum { MAX_LINES = 4 };

	ASSERT(box != NULL);

	memset(&box->req_common, 0, sizeof (box->req_common));

	if (key == FMS_KEY_LSK_L1) {
		fmsbox_set_page(box, FMS_PAGE_REQ_ALT);
		memset(&box->alt_req, 0, sizeof (box->alt_req));
	} else if (key == FMS_KEY_LSK_L2) {
		fmsbox_set_page(box, FMS_PAGE_REQ_OFF);
		memset(&box->off_req, 0, sizeof (box->off_req));
	} else if (key == FMS_KEY_LSK_L3) {
		fmsbox_set_page(box, FMS_PAGE_REQ_SPD);
		memset(&box->spd_req, 0, sizeof (box->spd_req));
	} else if (key == FMS_KEY_LSK_L4) {
		fmsbox_set_page(box, FMS_PAGE_REQ_RTE);
	} else if (key == FMS_KEY_LSK_R1) {
		fmsbox_set_page(box, FMS_PAGE_REQ_CLX);
	} else if (key == FMS_KEY_LSK_R2) {
		fmsbox_set_page(box, FMS_PAGE_REQ_VMC);
	} else if (key == FMS_KEY_LSK_R3) {
		fmsbox_set_page(box, FMS_PAGE_REQ_WCW);
	} else if (key == FMS_KEY_LSK_R4) {
		fmsbox_set_page(box, FMS_PAGE_REQ_VOICE);
	} else {
		return (false);
	}

	return (true);
}

void
fmsbox_req_add_common(fmsbox_t *box, cpdlc_msg_t *msg)
{
	ASSERT(box != NULL);
	ASSERT(msg != NULL);

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
	    box->req_common.freetext[3][0] != '\0') {
		char buf[sizeof (box->req_common.freetext) +
		    REQ_FREETEXT_LINES] = { 0 };
		unsigned seg;

		for (int i = 0; i < REQ_FREETEXT_LINES; i++) {
			if (box->req_common.freetext[i][0] != '\0') {
				if (strlen(buf) != 0)
					strcat(buf, " ");
				strcat(buf, box->req_common.freetext[i]);
			}
		}
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM67_FREETEXT_NORMAL_text, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, buf, NULL);
	}
}

void
fmsbox_req_draw_due(fmsbox_t *box, bool due_tfc)
{
	ASSERT(box != NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L2, "DUE TO WX");
	fmsbox_put_altn_selector(box, LSK2_ROW, false, box->req_common.due_wx,
	    "NO", "YES", NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L3, "DUE TO A/C");
	fmsbox_put_altn_selector(box, LSK3_ROW, false, box->req_common.due_ac,
	    "NO", "YES", NULL);

	if (due_tfc) {
		fmsbox_put_lsk_title(box, FMS_KEY_LSK_L4, "DUE TO TRAFFIC");
		fmsbox_put_altn_selector(box, LSK4_ROW, false,
		    box->req_common.due_tfc, "NO", "YES", NULL);
	}
}

void
fmsbox_req_key_due(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);
	ASSERT(key >= FMS_KEY_LSK_L2 && key <= FMS_KEY_LSK_L4);
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
fmsbox_req_draw_freetext(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "REMARKS");

	for (int i = 0; i < REQ_FREETEXT_LINES; i++) {
		if (box->req_common.freetext[i][0] != '\0') {
			fmsbox_put_str(box, LSKi_ROW(i), 0, false,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE, "%s",
			    box->req_common.freetext[i]);
		} else {
			fmsbox_put_str(box, LSKi_ROW(i), 0, false,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE,
			    "------------------------");
		}
	}
}

void
fmsbox_req_key_freetext(fmsbox_t *box, fms_key_t key)
{
	int line;

	ASSERT(box != NULL);
	ASSERT(key >= FMS_KEY_LSK_L1 && key <= FMS_KEY_LSK_L4);

	line = key - FMS_KEY_LSK_L1;
	fmsbox_scratchpad_xfer(box, box->req_common.freetext[line],
	    sizeof (box->req_common.freetext[line]), true);
}
