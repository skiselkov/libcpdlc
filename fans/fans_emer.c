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

#include <stdio.h>
#include <string.h>

#include "../src/cpdlc_alloc.h"
#include "../src/cpdlc_assert.h"

#include "fans_emer.h"
#include "fans_pos_pick.h"
#include "fans_req.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

static void
verify_emer(fans_t *box)
{
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
	cpdlc_alt_t des;
	fms_time_t fuel;
	char reason[64] = {};

	if (box->emer.pan) {
		cpdlc_msg_add_seg(msg, true, CPDLC_DM55_PAN_PAN_PAN, 0);
	} else {
		cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM56_MAYDAY_MAYDAY_MAYDAY, 0);
	}
	fuel = (box->emer.fuel.set ? box->emer.fuel : box->emer.fuel_auto);
	if (fuel.set || box->emer.souls_set) {
		int seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM57_RMNG_FUEL_AND_POB, 0);
		cpdlc_msg_seg_set_arg(msg, seg_nr, 0, &fuel.hrs, &fuel.mins);
		cpdlc_msg_seg_set_arg(msg, seg_nr, 1, &box->emer.souls, NULL);
	}
	switch (box->emer.reason) {
	case EMER_REASON_NONE:
		break;
	case EMER_REASON_WX:
		snprintf(reason, sizeof (reason), "%sDUE TO WX.",
		    box->emer.pan ? "" : "EMERGENCY ");
		break;
	case EMER_REASON_MED:
		snprintf(reason, sizeof (reason), "MEDICAL EMERGENCY.");
		break;
	case EMER_REASON_CABIN_PRESS:
		snprintf(reason, sizeof (reason), "%sDUE TO CABIN PRESS.",
		    box->emer.pan ? "" : "EMERGENCY ");
		break;
	case EMER_REASON_ENG_LOSS:
		snprintf(reason, sizeof (reason), "%sDUE TO ENGINE LOSS.",
		    box->emer.pan ? "" : "EMERGENCY ");
		break;
	case EMER_REASON_LOW_FUEL:
		snprintf(reason, sizeof (reason), "%sDUE TO LOW FUEL.",
		    box->emer.pan ? "" : "EMERGENCY ");
		break;
	}
	if (box->emer.divert.set) {
		cpdlc_route_t *dct = safe_calloc(1, sizeof (*dct));
		int seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM59_DIVERTING_TO_pos_VIA_route, 0);
		cpdlc_msg_seg_set_arg(msg, seg_nr, 0, &box->emer.divert, NULL);
		cpdlc_msg_seg_set_arg(msg, seg_nr, 1, dct, NULL);
		free(dct);
	} else if (box->emer.off.nm != 0) {
		int seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM60_OFFSETTING_dist_dir_OF_ROUTE, 0);
		cpdlc_msg_seg_set_arg(msg, seg_nr, 0, &box->emer.off.nm, NULL);
		cpdlc_msg_seg_set_arg(msg, seg_nr, 1, &box->emer.off.dir, NULL);
	}
	des = (!CPDLC_IS_NULL_ALT(box->emer.des) ? box->emer.des :
	    box->emer.des_auto);
	if (!CPDLC_IS_NULL_ALT(des)) {
		int seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM61_DESCENDING_TO_alt, 0);
		const bool fl = false;
		cpdlc_msg_seg_set_arg(msg, seg_nr, 0, &fl, &des.alt);
	}

	box->req_common.distress = true;
	fans_req_add_common(box, msg, reason[0] != '\0' ? reason : NULL);

	fans_verify_msg(box, msg, "EMER MSG", FMS_PAGE_EMER, true);
}

static void
set_divert(fans_t *box, const cpdlc_pos_t *pos)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(pos != NULL);
	box->emer.divert = *pos;
	/* Mutually exclusive with offset */
	memset(&box->emer.off, 0, sizeof (box->emer.off));
}

static void
draw_main_page(fans_t *box)
{
	const char *reason = NULL;

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "EMER TYPE");
	fans_put_altn_selector(box, LSK2_ROW, false, box->emer.pan,
	    "MAYDAY", "PAN", NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "FUEL");
	fans_put_time(box, LSK3_ROW, 0, false, &box->emer.fuel,
	    &box->emer.fuel_auto, false, true);
	fans_put_str(box, LSK3_ROW, 6, false, FMS_COLOR_GREEN,
	    FMS_FONT_SMALL, "HH:MM");

	fans_put_lsk_title(box, FMS_KEY_LSK_L4, "SOULS");
	if (box->emer.souls_set) {
		fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%d", box->emer.souls);
	} else {
		fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "---");
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R2, "DESCEND TO");
	fans_put_alt(box, LSK2_ROW, 0, true, &box->emer.des,
	    &box->emer.des_auto, false, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_R3, "OFFSET TO");
	fans_put_off(box, LSK3_ROW, 0, true, &box->emer.off,
	    NULL, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R4, "DIVERT TO");
	fans_put_pos(box, LSK4_ROW, 0, true, &box->emer.divert, NULL, false);

	switch (box->emer.reason) {
	case EMER_REASON_NONE:
		reason = "NONE";
		break;
	case EMER_REASON_WX:
		reason = "WX";
		break;
	case EMER_REASON_MED:
		reason = "MED EMER";
		break;
	case EMER_REASON_CABIN_PRESS:
		reason = "CABIN PRESS";
		break;
	case EMER_REASON_ENG_LOSS:
		reason = "ENG LOSS";
		break;
	case EMER_REASON_LOW_FUEL:
		reason = "LOW FUEL";
		break;
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R5, "REASON");
	fans_put_str(box, LSK5_ROW, 0, true, FMS_COLOR_GREEN,
	    FMS_FONT_LARGE, "%sv", reason);
}

void
fans_emer_init_cb(fans_t *box)
{
	int cur_alt, sel_alt;
	float vvi;
	bool sel_alt_fl;

	/* The EMER page can send freetext as well */
	CPDLC_ASSERT(box != NULL);
	memset(&box->emer, 0, sizeof (box->emer));
	memset(&box->req_common, 0, sizeof (box->req_common));

	fans_get_cur_alt(box, &cur_alt, NULL);
	fans_get_sel_alt(box, &sel_alt, &sel_alt_fl);
	vvi = fans_get_cur_vvi(box);

	box->emer.fuel_auto.set = fans_get_fuel(box, &box->emer.fuel_auto.hrs,
	    &box->emer.fuel_auto.mins);
	if (fans_is_valid_alt(cur_alt) && fans_is_valid_alt(sel_alt) &&
	    !isnan(vvi) && vvi <= 300 && cur_alt >= sel_alt + LVL_ALT_THRESH) {
		box->emer.des_auto.fl = sel_alt_fl;
		box->emer.des_auto.alt = sel_alt;
	} else {
		box->emer.des_auto = CPDLC_NULL_ALT;
	}
	box->emer.souls_set = fans_get_souls(box, &box->emer.souls);
	box->emer.des = CPDLC_NULL_ALT;
	box->emer.divert = CPDLC_NULL_POS;
}

void
fans_emer_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  EMERGENCY MSG");
	fans_put_page_ind(box);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE, "<VERIFY");
}

bool
fans_emer_key_cb(fans_t *box, fms_key_t key)
{
	bool read_back;

	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L2) {
		box->emer.pan = !box->emer.pan;
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L3) {
		if (fans_scratchpad_xfer_time(box, &box->emer.fuel,
		    &box->emer.fuel_auto, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		if (fans_scratchpad_xfer_uint(box, &box->emer.souls,
		    &box->emer.souls_set, 1, 999, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R2) {
		if (fans_scratchpad_xfer_alt(box, &box->emer.des,
		    &box->emer.des_auto, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R3) {
		if (fans_scratchpad_xfer_offset(box, &box->emer.off, NULL,
		    &read_back) && !read_back) {
			fans_scratchpad_clear(box);
			/* Mutually exclusive with diversion */
			box->emer.divert.set = false;
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		if (fans_scratchpad_is_delete(box)) {
			if (box->emer.divert.set) {
				fans_scratchpad_clear(box);
				box->emer.divert = CPDLC_NULL_POS;
			} else {
				fans_set_error(box, FANS_ERR_INVALID_DELETE);
			}
		} else {
			fans_pos_pick_start(box, set_divert, FMS_PAGE_EMER,
			    &box->emer.divert);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R5) {
		box->emer.reason = (box->emer.reason + 1) %
		    (EMER_REASON_LOW_FUEL + 1);
	} else if (key == FMS_KEY_LSK_L5) {
		verify_emer(box);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
