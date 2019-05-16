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

#include <stddef.h>
#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fmsbox_req.h"
#include "fmsbox_req_wcw.h"
#include "fmsbox_scratchpad.h"
#include "fmsbox_vrfy.h"

static bool
can_verify_wcw_req(fmsbox_t *box)
{
	ASSERT(box != NULL);
	return (box->wcw_req.alt.alt.alt != 0 ||
	    box->wcw_req.spd[0].spd.spd != 0 || box->wcw_req.back_on_rte ||
	    box->wcw_req.alt_chg);
}

static void
verify_wcw_req(fmsbox_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc();

	if (box->wcw_req.alt.alt.alt != 0) {
		if (box->wcw_req.crz_clb) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM54_WHEN_CAN_WE_EXPECT_CRZ_CLB_TO_alt, 0);
		} else {
			/* FIXME: need to query current alt for proper msg */
			seg = cpdlc_msg_add_seg(msg, true, 67,
			    CPDLC_DM67h_WHEN_CAN_WE_EXPCT_CLB_TO_alt);
		}
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->wcw_req.alt.alt.fl,
		    &box->wcw_req.alt.alt.alt);
	} else if (box->wcw_req.spd[1].spd.spd != 0) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM50_WHEN_CAN_WE_EXPCT_spd_TO_spd, 0);
		for (int i = 0; i < 2; i++) {
			cpdlc_msg_seg_set_arg(msg, seg, i,
			    &box->wcw_req.spd[i].spd.mach,
			    &box->wcw_req.spd[i].spd.spd);
		}
	} else if (box->wcw_req.spd[0].spd.spd != 0) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM49_WHEN_CAN_WE_EXPCT_spd, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0,
		    &box->wcw_req.spd[0].spd.mach,
		    &box->wcw_req.spd[0].spd.spd);
	} else if (box->wcw_req.back_on_rte) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM51_WHEN_CAN_WE_EXPCT_BACK_ON_ROUTE, 0);
	} else if (box->wcw_req.alt_chg == ALT_CHG_HIGHER) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM53_WHEN_CAN_WE_EXPECT_HIGHER_ALT, 0);
	} else {
		ASSERT3U(box->wcw_req.alt_chg, ==, ALT_CHG_LOWER);
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM52_WHEN_CAN_WE_EXPECT_LOWER_ALT, 0);
	}

	fmsbox_req_add_common(box, msg);

	fmsbox_verify_msg(box, msg, "WHEN", FMS_PAGE_REQ_WCW);
}

static void
draw_main_page(fmsbox_t *box)
{
	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "ALTITUDE");
	if (box->wcw_req.alt.alt.alt != 0) {
		fmsbox_put_alt(box, LSK1_ROW, 0, false, &box->wcw_req.alt);
	} else {
		fmsbox_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, "-----");
	}

	if (box->wcw_req.alt.alt.alt >= 32000) {
		fmsbox_put_lsk_title(box, FMS_KEY_LSK_L2, "CRZ CLB");
		fmsbox_put_altn_selector(box, LSK2_ROW, false,
		    box->wcw_req.crz_clb, "NO", "YES", NULL);
	} else {
		box->wcw_req.crz_clb = false;
	}

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L3, "ALT CHANGE");
	fmsbox_put_altn_selector(box, LSK3_ROW, false, box->wcw_req.alt_chg,
	    "NONE", "HIGHER", "LOWER", NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R1, "SPD/SPD BLOCK");
	if (box->wcw_req.spd[0].spd.spd != 0) {
		fmsbox_put_spd(box, LSK1_ROW, FMSBOX_COLS - 7,
		    &box->wcw_req.spd[0]);
	} else {
		fmsbox_put_str(box, LSK1_ROW, 4, true, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, "---");
	}
	fmsbox_put_str(box, LSK1_ROW, 3, true, FMS_COLOR_CYAN,
	    FMS_FONT_LARGE, "/");
	if (box->wcw_req.spd[1].spd.spd != 0) {
		fmsbox_put_spd(box, LSK1_ROW, FMSBOX_COLS - 3,
		    &box->wcw_req.spd[1]);
	} else {
		fmsbox_put_str(box, LSK1_ROW, 0, true, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, "---");
	}

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R2, "BACK ON RTE");
	fmsbox_put_altn_selector(box, LSK2_ROW, true,
	    !box->wcw_req.back_on_rte, "YES", "NO", NULL);
}

void
fmsbox_req_wcw_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_set_num_subpages(box, 2);

	fmsbox_put_page_title(box, "FANS  WHEN CAN WE");
	fmsbox_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fmsbox_req_draw_freetext(box);

	if (can_verify_wcw_req(box)) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fmsbox_req_wcw_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		char buf[8];
		cpdlc_arg_t arg;
		const char *error;

		memset(&arg, 0, sizeof (arg));
		fmsbox_print_alt(&box->wcw_req.alt, buf, sizeof (buf));
		fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
		if (strlen(buf) == 0) {
			memset(&box->wcw_req.alt, 0, sizeof (box->wcw_req.alt));
		} else if ((error = fmsbox_parse_alt(buf, 0, &arg)) != NULL) {
			fmsbox_set_error(box, error);
		} else {
			box->wcw_req.alt = arg;
			memset(box->wcw_req.spd, 0, sizeof (box->wcw_req.spd));
			box->wcw_req.back_on_rte = false;
			box->wcw_req.alt_chg = ALT_CHG_NONE;
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L2) {
		box->wcw_req.crz_clb = !box->wcw_req.crz_clb;
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L3) {
		memset(&box->wcw_req.alt, 0, sizeof (box->wcw_req.alt));
		memset(box->wcw_req.spd, 0, sizeof (box->wcw_req.spd));
		box->wcw_req.back_on_rte = false;
		box->wcw_req.alt_chg = (box->wcw_req.alt_chg + 1) %
		    (ALT_CHG_LOWER + 1);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R1) {
		if (fmsbox_scratchpad_xfer_multi(box,
		    (void *)offsetof(fmsbox_t, wcw_req.spd),
		    sizeof (cpdlc_arg_t), fmsbox_parse_spd,
		    fmsbox_insert_spd_block, fmsbox_delete_cpdlc_arg_block,
		    fmsbox_read_spd_block)) {
			memset(&box->wcw_req.alt, 0, sizeof (box->wcw_req.alt));
			box->wcw_req.back_on_rte = false;
			box->wcw_req.alt_chg = ALT_CHG_NONE;
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R2) {
		memset(&box->wcw_req.alt, 0, sizeof (box->wcw_req.alt));
		memset(box->wcw_req.spd, 0, sizeof (box->wcw_req.spd));
		box->wcw_req.back_on_rte = !box->wcw_req.back_on_rte;
		box->wcw_req.alt_chg = ALT_CHG_NONE;
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_wcw_req(box))
			verify_wcw_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	} else if (KEY_IS_REQ_FREETEXT(box, key)) {
		fmsbox_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
