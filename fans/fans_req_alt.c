/*
 * Copyright 2022 Saso Kiselkov
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
#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fans_impl.h"
#include "fans_req.h"
#include "fans_req_alt.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

typedef enum {
    CLB_CLASS_CLB = 1,
    CLB_CLASS_LVL = 0,
    CLB_CLASS_DES = -1
} clb_class_t;

static bool
can_verify_alt_req(fans_t *box)
{
	CPDLC_ASSERT(box);
	return (!CPDLC_IS_NULL_ALT(box->alt_req.alt[0]) &&
	    fans_step_at_can_send(&box->alt_req.step_at));
}

static clb_class_t
get_clb_class(fans_t *box)
{
	int cur_alt;

	CPDLC_ASSERT(box != NULL);

	fans_get_cur_alt(box, &cur_alt, NULL);
	if (fans_is_valid_alt(cur_alt) &&
	    box->alt_req.alt[0].alt >= cur_alt + LVL_ALT_THRESH) {
		return (CLB_CLASS_CLB);
	} else if (fans_is_valid_alt(cur_alt) &&
	    box->alt_req.alt[0].alt <= cur_alt - LVL_ALT_THRESH) {
		return (CLB_CLASS_DES);
	} else {
		return (CLB_CLASS_LVL);
	}
}

static void
verify_alt_req(fans_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg;
	clb_class_t clb;

	CPDLC_ASSERT(box != NULL);

	msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
	clb = get_clb_class(box);
	if (box->alt_req.step_at.type != STEP_AT_NONE) {
		if (box->alt_req.step_at.type == STEP_AT_TIME) {
			seg = cpdlc_msg_add_seg(msg, true,
			    clb >= CLB_CLASS_LVL ?
			    CPDLC_DM13_AT_time_REQ_CLB_TO_alt :
			    CPDLC_DM14_AT_time_REQ_DES_TO_alt, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    &box->alt_req.step_at.tim.hrs,
			    &box->alt_req.step_at.tim.mins);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    clb >= CLB_CLASS_LVL ?
			    CPDLC_DM11_AT_pos_REQ_CLB_TO_alt :
			    CPDLC_DM12_AT_pos_REQ_DES_TO_alt, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    box->alt_req.step_at.pos, NULL);
		}
		cpdlc_msg_seg_set_arg(msg, seg, 1,
		    &box->alt_req.alt[0].fl, &box->alt_req.alt[0].alt);
	} else if (box->alt_req.alt[1].alt != 0) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM7_REQ_BLOCK_alt_TO_alt, 0);
		for (int i = 0; i < 2; i++) {
			cpdlc_msg_seg_set_arg(msg, seg, i,
			    &box->alt_req.alt[i].fl,
			    &box->alt_req.alt[i].alt);
		}
	} else {
		if (box->alt_req.crz_clb) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM8_REQ_CRZ_CLB_TO_alt, 0);
		} else if (clb == CLB_CLASS_CLB) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM9_REQ_CLB_TO_alt, 0);
		} else if (clb == CLB_CLASS_DES) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM10_REQ_DES_TO_alt, 0);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM6_REQ_alt, 0);
		}
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->alt_req.alt[0].fl,
		    &box->alt_req.alt[0].alt);
	}
	if (box->alt_req.plt_discret) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM75_AT_PILOTS_DISCRETION, 0);
	}
	if (box->alt_req.maint_sep_vmc) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM74_MAINT_OWN_SEPARATION_AND_VMC, 0);
	}
	fans_req_add_common(box, msg, NULL);

	fans_verify_msg(box, msg, "ALT REQ", FMS_PAGE_REQ_ALT, true);
}

static void
draw_main_page(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "ALT/ALT BLOCK");
	fans_put_alt(box, LSK1_ROW, 0, false, &box->alt_req.alt[0],
	    NULL, true, false);
	fans_put_str(box, LSK1_ROW, 5, false, FMS_COLOR_WHITE,
	    FMS_FONT_LARGE, "/");
	fans_put_alt(box, LSK1_ROW, 6, false, &box->alt_req.alt[1],
	    NULL, false, false);

	fans_req_draw_due(box, false);

	fans_put_step_at(box, &box->alt_req.step_at);

	if (box->alt_req.alt[0].alt >= CRZ_CLB_THRESH &&
	    get_clb_class(box) == CLB_CLASS_CLB) {
		fans_put_lsk_title(box, FMS_KEY_LSK_L4, "CRZ CLB");
		fans_put_altn_selector(box, LSK4_ROW, false,
		    box->alt_req.crz_clb, "NO", "YES", NULL);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R4, "PLT DISCRET");
	fans_put_altn_selector(box, LSK4_ROW, true,
	    !box->alt_req.plt_discret, "YES", "NO", NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_R5, "MAINT SEP/VMC");
	fans_put_altn_selector(box, LSK5_ROW, true,
	    !box->alt_req.maint_sep_vmc, "YES", "NO", NULL);
}

void
fans_req_alt_reset(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	memset(&box->alt_req, 0, sizeof (box->alt_req));
	box->alt_req.alt[0] = CPDLC_NULL_ALT;
	box->alt_req.alt[1] = CPDLC_NULL_ALT;
}

void
fans_req_alt_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  ALTITUDE REQ");
	fans_put_page_ind(box);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	if (can_verify_alt_req(box)) {
		fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}
	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_req_alt_key_cb(fans_t *box, fms_key_t key)
{
	bool read_back;

	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		if (!fans_scratchpad_xfer_multi(box,
		    (void *)offsetof(fans_t, alt_req.alt),
		    sizeof (cpdlc_alt_t), fans_parse_alt,
		    fans_insert_alt_block, fans_delete_cpdlc_alt_block,
		    fans_read_alt_block, &read_back)) {
			return (true);
		}
		if (!CPDLC_IS_NULL_ALT(box->alt_req.alt[1])) {
			/* Block altitude requests cannot include a STEP AT */
			box->alt_req.step_at.type = STEP_AT_NONE;
			box->alt_req.crz_clb = false;
		}
		if (box->alt_req.alt[0].alt < CRZ_CLB_THRESH ||
		    !CPDLC_IS_NULL_ALT(box->alt_req.alt[1])) {
			/*
			 * If the altitude is below the CRZ CLB threshold,
			 * or a block altitude is entered, reset the CRZ CLB.
			 */
			box->alt_req.crz_clb = false;
		}
		if (!read_back)
			fans_scratchpad_clear(box);
	} else if (box->subpage == 0 &&
	    (key == FMS_KEY_LSK_L2 || key == FMS_KEY_LSK_L3)) {
		fans_req_key_due(box, key);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		if (box->alt_req.alt[0].alt >= CRZ_CLB_THRESH &&
		    get_clb_class(box) == CLB_CLASS_CLB) {
			box->alt_req.crz_clb = !box->alt_req.crz_clb;
			/* cruise climbs cannot be block or STEP AT requests */
			box->alt_req.alt[1] = CPDLC_NULL_ALT;
			box->alt_req.step_at.type = STEP_AT_NONE;
		}
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_alt_req(box))
			verify_alt_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fans_set_page(box, FMS_PAGE_REQUESTS, false);
	} else if (KEY_IS_REQ_STEP_AT(box, key)) {
		fans_key_step_at(box, key, &box->alt_req.step_at);
		if (box->alt_req.step_at.type != STEP_AT_NONE) {
			box->alt_req.alt[1] = CPDLC_NULL_ALT;
			/* STEP AT requests cannot be CRZ CLB */
			box->alt_req.crz_clb = false;
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		box->alt_req.plt_discret = !box->alt_req.plt_discret;
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R5) {
		box->alt_req.maint_sep_vmc = !box->alt_req.maint_sep_vmc;
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
