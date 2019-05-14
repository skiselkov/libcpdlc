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

#include "fmsbox_impl.h"
#include "fmsbox_scratchpad.h"
#include "fmsbox_vrfy.h"

static bool
can_verify_alt_req(fmsbox_t *box)
{
	ASSERT(box);
	return (box->alt_req.alt[0].alt.alt != 0);
}

static void
construct_alt_req(fmsbox_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg;

	if (box->verify.msg != NULL)
		cpdlc_msg_free(box->verify.msg);
	msg = box->verify.msg = cpdlc_msg_alloc();
	if (strlen(box->alt_req.step_at) != 0) {
		if (box->alt_req.step_at_time) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM13_AT_time_REQ_CLB_TO_alt, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    &box->alt_req.step_hrs, &box->alt_req.step_mins);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM11_AT_pos_REQ_CLB_TO_alt, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    box->alt_req.step_at, NULL);
		}
		cpdlc_msg_seg_set_arg(msg, seg, 1,
		    &box->alt_req.alt[0].alt.fl, &box->alt_req.alt[0].alt.alt);
	} else if (box->alt_req.alt[1].alt.alt != 0) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM7_REQ_BLOCK_alt_TO_alt, 0);
		for (int i = 0; i < 2; i++) {
			cpdlc_msg_seg_set_arg(msg, seg, i,
			    &box->alt_req.alt[i].alt.fl,
			    &box->alt_req.alt[i].alt.alt);
		}
	} else {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM6_REQ_alt, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->alt_req.alt[0].alt.fl,
		    &box->alt_req.alt[0].alt.alt);
	}
	if (box->alt_req.plt_discret) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM75_AT_PILOTS_DISCRETION, 0);
	}
	if (box->alt_req.maint_sep_vmc) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM74_MAINT_OWN_SEPARATION_AND_VMC, 0);
	}
	if (box->alt_req.due_wx) {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM65_DUE_TO_WX, 0);
	} else if (box->alt_req.due_ac) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM66_DUE_TO_ACFT_PERF, 0);
	}
}

void
fmsbox_req_alt_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_put_page_title(box, "FANS  ALTITUDE REQ");

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "ALT/ALT BLOCK");
	if (box->alt_req.alt[0].alt.alt != 0) {
		fmsbox_put_alt(box, LSK1_ROW, 0, &box->alt_req.alt[0]);
	} else {
		fmsbox_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "_____");
	}
	fmsbox_put_str(box, LSK1_ROW, 5, false, FMS_COLOR_CYAN,
	    FMS_FONT_SMALL, "/");
	if (box->alt_req.alt[1].alt.alt != 0) {
		fmsbox_put_alt(box, LSK1_ROW, 6, &box->alt_req.alt[1]);
	} else {
		fmsbox_put_str(box, LSK1_ROW, 6, false, FMS_COLOR_CYAN,
		    FMS_FONT_SMALL, "-----");
	}
	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L2, "DUE TO WX");
	fmsbox_put_altn_selector(box, LSK2_ROW, false, box->alt_req.due_wx,
	    "NO", "YES", NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L3, "DUE TO A/C");
	fmsbox_put_altn_selector(box, LSK3_ROW, false, box->alt_req.due_ac,
	    "NO", "YES", NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R1, "STEP AT");
	if (strlen(box->alt_req.step_at) != 0) {
		fmsbox_put_str(box, LSK1_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "%s", box->alt_req.step_at);
	} else {
		fmsbox_put_str(box, LSK1_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "NONE");
	}

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R4, "PLT DISCRET");
	fmsbox_put_altn_selector(box, LSK4_ROW, true,
	    !box->alt_req.plt_discret, "YES", "NO", NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R5, "MAINT SEP/VMC");
	fmsbox_put_altn_selector(box, LSK5_ROW, true,
	    !box->alt_req.maint_sep_vmc, "YES", "NO", NULL);

	if (can_verify_alt_req(box)) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}

	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fmsbox_req_alt_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L1) {
		fmsbox_scratchpad_xfer_multi(box,
		    (void *)offsetof(fmsbox_t, alt_req.alt),
		    sizeof (cpdlc_arg_t), fmsbox_parse_alt,
		    fmsbox_insert_alt_block, fmsbox_delete_alt_block,
		    fmsbox_read_alt_block);
		if (box->alt_req.alt[1].alt.alt != 0) {
			/* Block altitude requests cannot include a STEP AT */
			memset(box->alt_req.step_at, 0,
			    sizeof (box->alt_req.step_at));
			box->alt_req.step_at_time = false;
		}
	} else if (key == FMS_KEY_LSK_L2) {
		box->alt_req.due_wx = !box->alt_req.due_wx;
		box->alt_req.due_ac = false;
	} else if (key == FMS_KEY_LSK_L3) {
		box->alt_req.due_wx = false;
		box->alt_req.due_ac = !box->alt_req.due_ac;
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_alt_req(box)) {
			construct_alt_req(box);
			fmsbox_verify_send(box, "ALT REQ", FMS_PAGE_REQ_ALT);
		}
	} else if (key == FMS_KEY_LSK_L6) {
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	} else if (key == FMS_KEY_LSK_R1) {
		fmsbox_scratchpad_xfer(box, box->alt_req.step_at,
		    sizeof (box->alt_req.step_at), true);
		box->alt_req.step_at_time =
		    fmsbox_parse_time(box->alt_req.step_at,
		    &box->alt_req.step_hrs, &box->alt_req.step_mins);
		if (strlen(box->alt_req.step_at) != 0) {
			memset(&box->alt_req.alt[1], 0,
			    sizeof (box->alt_req.alt[1]));
		} else {
			box->alt_req.step_at_time = false;
		}
	} else if (key == FMS_KEY_LSK_R4) {
		box->alt_req.plt_discret = !box->alt_req.plt_discret;
	} else if (key == FMS_KEY_LSK_R5) {
		box->alt_req.maint_sep_vmc = !box->alt_req.maint_sep_vmc;
	} else {
		return (false);
	}

	return (true);
}
