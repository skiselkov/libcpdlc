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
#include "fmsbox_req.h"
#include "fmsbox_req_alt.h"
#include "fmsbox_scratchpad.h"
#include "fmsbox_vrfy.h"

static bool
can_verify_alt_req(fmsbox_t *box)
{
	ASSERT(box);
	return (box->alt_req.alt[0].alt.alt != 0 &&
	    fmsbox_step_at_can_send(&box->alt_req.step_at));
}

static void
verify_alt_req(fmsbox_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc();

	if (box->alt_req.step_at.type != STEP_AT_NONE) {
		if (box->alt_req.step_at.type == STEP_AT_TIME) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM13_AT_time_REQ_CLB_TO_alt, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    &box->alt_req.step_at.tim.hrs,
			    &box->alt_req.step_at.tim.mins);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM11_AT_pos_REQ_CLB_TO_alt, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    box->alt_req.step_at.pos, NULL);
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
		if (box->alt_req.crz_clb) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM8_REQ_CRZ_CLB_TO_alt, 0);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM6_REQ_alt, 0);
		}
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
	fmsbox_req_add_common(box, msg);

	fmsbox_verify_msg(box, msg, "ALT REQ", FMS_PAGE_REQ_ALT, true);
}

static void
draw_main_page(fmsbox_t *box)
{
	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "ALT/ALT BLOCK");
	if (box->alt_req.alt[0].alt.alt != 0) {
		fmsbox_put_alt(box, LSK1_ROW, 0, false, &box->alt_req.alt[0]);
	} else {
		fmsbox_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "_____");
	}
	fmsbox_put_str(box, LSK1_ROW, 5, false, FMS_COLOR_CYAN,
	    FMS_FONT_SMALL, "/");
	if (box->alt_req.alt[1].alt.alt != 0) {
		fmsbox_put_alt(box, LSK1_ROW, 6, false, &box->alt_req.alt[1]);
	} else {
		fmsbox_put_str(box, LSK1_ROW, 6, false, FMS_COLOR_CYAN,
		    FMS_FONT_SMALL, "-----");
	}
	fmsbox_req_draw_due(box, false);

	fmsbox_put_step_at(box, &box->alt_req.step_at);

	if (box->alt_req.alt[0].alt.alt >= CRZ_CLB_THRESHOLD) {
		fmsbox_put_lsk_title(box, FMS_KEY_LSK_L4, "CRZ CLB");
		fmsbox_put_altn_selector(box, LSK4_ROW, false,
		    box->alt_req.crz_clb, "NO", "YES", NULL);
	}

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R4, "PLT DISCRET");
	fmsbox_put_altn_selector(box, LSK4_ROW, true,
	    !box->alt_req.plt_discret, "YES", "NO", NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R5, "MAINT SEP/VMC");
	fmsbox_put_altn_selector(box, LSK5_ROW, true,
	    !box->alt_req.maint_sep_vmc, "YES", "NO", NULL);
}

void
fmsbox_req_alt_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_set_num_subpages(box, 2);

	fmsbox_put_page_title(box, "FANS  ALTITUDE REQ");
	fmsbox_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fmsbox_req_draw_freetext(box);

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

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		fmsbox_scratchpad_xfer_multi(box,
		    (void *)offsetof(fmsbox_t, alt_req.alt),
		    sizeof (cpdlc_arg_t), fmsbox_parse_alt,
		    fmsbox_insert_alt_block, fmsbox_delete_cpdlc_arg_block,
		    fmsbox_read_alt_block);
		if (box->alt_req.alt[1].alt.alt != 0) {
			/* Block altitude requests cannot include a STEP AT */
			box->alt_req.step_at.type = STEP_AT_NONE;
			box->alt_req.crz_clb = false;
		}
		if (box->alt_req.alt[0].alt.alt < CRZ_CLB_THRESHOLD ||
		    box->alt_req.alt[1].alt.alt != 0) {
			/*
			 * If the altitude is below the CRZ CLB threshold,
			 * or a block altitude is entered, reset the CRZ CLB.
			 */
			box->alt_req.crz_clb = false;
		}
	} else if (box->subpage == 0 &&
	    (key == FMS_KEY_LSK_L2 || key == FMS_KEY_LSK_L3)) {
		fmsbox_req_key_due(box, key);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		if (box->alt_req.alt[0].alt.alt >= CRZ_CLB_THRESHOLD) {
			box->alt_req.crz_clb = !box->alt_req.crz_clb;
			/* cruise climbs cannot be block or STEP AT requests */
			memset(&box->alt_req.alt[1], 0,
			    sizeof (box->alt_req.alt[1]));
			box->alt_req.step_at.type = STEP_AT_NONE;
		}
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_alt_req(box))
			verify_alt_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	} else if (KEY_IS_REQ_STEP_AT(box, key)) {
		fmsbox_key_step_at(box, key, &box->alt_req.step_at);
		if (box->alt_req.step_at.type != STEP_AT_NONE) {
			memset(&box->alt_req.alt[1], 0,
			    sizeof (box->alt_req.alt[1]));
			/* STEP AT requests cannot be CRZ CLB */
			box->alt_req.crz_clb = false;
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		box->alt_req.plt_discret = !box->alt_req.plt_discret;
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R5) {
		box->alt_req.maint_sep_vmc = !box->alt_req.maint_sep_vmc;
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fmsbox_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
