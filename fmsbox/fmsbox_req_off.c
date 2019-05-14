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

#include <stdio.h>
#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fmsbox_req_off.h"
#include "fmsbox_scratchpad.h"
#include "fmsbox_vrfy.h"

static void
print_off(fmsbox_t *box, char *buf, size_t cap)
{
	ASSERT(box != NULL);
	if (box->off_req.nm != 0) {
		snprintf(buf, cap, "%c%.0f",
		    box->off_req.dir == CPDLC_DIR_LEFT ? 'L' : 'R',
		    box->off_req.nm);
	} else if (buf != NULL) {
		buf[0] = '\0';
	}
}

static bool
can_verify_off_req(fmsbox_t *box)
{
	ASSERT(box);
	return (box->off_req.nm != 0 &&
	    fmsbox_step_at_can_send(&box->off_req.step_at));
}

static void
verify_off_req(fmsbox_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg;

	msg = cpdlc_msg_alloc();

	if (box->off_req.step_at.type != STEP_AT_NONE) {
		if (box->off_req.step_at.type == STEP_AT_TIME) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM17_AT_time_REQ_OFFSET_dir_dist_OF_ROUTE, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    &box->off_req.step_at.hrs,
			    &box->off_req.step_at.mins);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM16_AT_pos_REQ_OFFSET_dir_dist_OF_ROUTE, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    box->off_req.step_at.pos, NULL);
		}
		cpdlc_msg_seg_set_arg(msg, seg, 1, &box->off_req.dir, NULL);
		cpdlc_msg_seg_set_arg(msg, seg, 2, &box->off_req.nm, NULL);
	} else {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM15_REQ_OFFSET_dir_dist_OF_ROUTE, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->off_req.dir, NULL);
		cpdlc_msg_seg_set_arg(msg, seg, 1, &box->off_req.nm, NULL);
	}
	if (box->off_req.due_wx) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM65_DUE_TO_WX, 0);
	} else if (box->off_req.due_ac) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM66_DUE_TO_ACFT_PERF, 0);
	} else if (box->off_req.due_tfc) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM67_FREETEXT_NORMAL_text, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, "DUE TO TRAFFIC", NULL);
	}

	fmsbox_verify_msg(box, msg, "OFF REQ", FMS_PAGE_REQ_OFF);
}

void
fmsbox_req_off_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_put_page_title(box, "FANS  OFFSET REQ");

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "OFFSET");
	if (box->off_req.nm == 0) {
		fmsbox_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "____");
	} else {
		char buf[8];
		print_off(box, buf, sizeof (buf));
		fmsbox_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", buf);
	}

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L2, "DUE TO WX");
	fmsbox_put_altn_selector(box, LSK2_ROW, false, box->off_req.due_wx,
	    "NO", "YES", NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L3, "DUE TO A/C");
	fmsbox_put_altn_selector(box, LSK3_ROW, false, box->off_req.due_ac,
	    "NO", "YES", NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L4, "DUE TO TRAFFIC");
	fmsbox_put_altn_selector(box, LSK4_ROW, false, box->off_req.due_tfc,
	    "NO", "YES", NULL);

	fmsbox_put_step_at(box, &box->off_req.step_at);

	if (can_verify_off_req(box)) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}

	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fmsbox_req_off_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L1) {
		char buf[8];

		print_off(box, buf, sizeof (buf));
		fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
		if (strlen(buf) != 0) {
			unsigned nm;

			if ((buf[0] != 'L' && buf[0] != 'R') ||
			    sscanf(&buf[1], "%d", &nm) != 1 ||
			    nm == 0 || nm > 999) {
				fmsbox_set_error(box, "FORMAT ERROR");
			} else if (buf[0] == 'L') {
				box->off_req.dir = CPDLC_DIR_LEFT;
				box->off_req.nm = nm;
			} else {
				box->off_req.dir = CPDLC_DIR_RIGHT;
				box->off_req.nm = nm;
			}
		} else {
			box->off_req.nm = 0;
		}
	} else if (key == FMS_KEY_LSK_L2) {
		box->off_req.due_wx = !box->off_req.due_wx;
		box->off_req.due_ac = false;
		box->off_req.due_tfc = false;
	} else if (key == FMS_KEY_LSK_L3) {
		box->off_req.due_wx = false;
		box->off_req.due_ac = !box->off_req.due_ac;
		box->off_req.due_tfc = false;
	} else if (key == FMS_KEY_LSK_L4) {
		box->off_req.due_wx = false;
		box->off_req.due_ac = false;
		box->off_req.due_tfc = !box->off_req.due_tfc;
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_off_req(box))
			verify_off_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	} else if (key == FMS_KEY_LSK_R1 || key == FMS_KEY_LSK_R2) {
		fmsbox_key_step_at(box, key, &box->off_req.step_at);
	} else {
		return (false);
	}

	return (true);
}
