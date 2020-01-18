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

#include "fans_req.h"
#include "fans_req_wcw.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

static bool
can_verify_wcw_req(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->wcw_req.alt.alt.alt != 0 ||
	    box->wcw_req.spd[0].spd.spd != 0 || box->wcw_req.back_on_rte ||
	    box->wcw_req.alt_chg);
}

static void
verify_wcw_req(fans_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);

	if (box->wcw_req.alt.alt.alt != 0) {
		if (box->wcw_req.crz_clb) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM54_WHEN_CAN_WE_EXPECT_CRZ_CLB_TO_alt, 0);
		} else {
			int cur_alt = fans_get_cur_alt(box);

			if (box->wcw_req.alt.alt.alt >= cur_alt) {
				seg = cpdlc_msg_add_seg(msg, true, 67,
				    CPDLC_DM67h_WHEN_CAN_WE_EXPCT_CLB_TO_alt);
			} else {
				seg = cpdlc_msg_add_seg(msg, true, 67,
				    CPDLC_DM67i_WHEN_CAN_WE_EXPCT_DES_TO_alt);
			}
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
		CPDLC_ASSERT3U(box->wcw_req.alt_chg, ==, ALT_CHG_LOWER);
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM52_WHEN_CAN_WE_EXPECT_LOWER_ALT, 0);
	}

	fans_req_add_common(box, msg);

	fans_verify_msg(box, msg, "WHEN", FMS_PAGE_REQ_WCW, true);
}

static void
draw_main_page(fans_t *box)
{
	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "ALTITUDE");
	fans_put_alt(box, LSK1_ROW, 0, false, &box->wcw_req.alt,
	    false, true, false);

	if (box->wcw_req.alt.alt.alt >= CRZ_CLB_THRESH) {
		fans_put_lsk_title(box, FMS_KEY_LSK_L2, "CRZ CLB");
		fans_put_altn_selector(box, LSK2_ROW, false,
		    box->wcw_req.crz_clb, "NO", "YES", NULL);
	} else {
		box->wcw_req.crz_clb = false;
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "ALT CHANGE");
	fans_put_altn_selector(box, LSK3_ROW, false, box->wcw_req.alt_chg,
	    "NONE", "HIGHER", "LOWER", NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_R1, "SPD/SPD BLOCK");
	fans_put_spd(box, LSK1_ROW, 4, true, &box->wcw_req.spd[0],
	    false, false, false);
	fans_put_str(box, LSK1_ROW, 3, true, FMS_COLOR_CYAN,
	    FMS_FONT_LARGE, "/");
	fans_put_spd(box, LSK1_ROW, 0, true, &box->wcw_req.spd[1],
	    false, false, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R2, "BACK ON RTE");
	fans_put_altn_selector(box, LSK2_ROW, true,
	    !box->wcw_req.back_on_rte, "YES", "NO", NULL);
}

void
fans_req_wcw_init_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	memset(&box->wcw_req, 0, sizeof (box->wcw_req));
}

void
fans_req_wcw_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  WHEN CAN WE");
	fans_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	if (can_verify_wcw_req(box)) {
		fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<CPDLC_VERIFY");
	}
	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_req_wcw_key_cb(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		char buf[8];
		cpdlc_arg_t arg;
		const char *error;

		memset(&arg, 0, sizeof (arg));
		fans_print_alt(&box->wcw_req.alt, buf, sizeof (buf));
		fans_scratchpad_xfer(box, buf, sizeof (buf), true);
		if (strlen(buf) == 0) {
			memset(&box->wcw_req.alt, 0, sizeof (box->wcw_req.alt));
		} else if ((error = fans_parse_alt(buf, 0, &arg)) != NULL) {
			fans_set_error(box, error);
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
		if (fans_scratchpad_xfer_multi(box,
		    (void *)offsetof(fans_t, wcw_req.spd),
		    sizeof (cpdlc_arg_t), fans_parse_spd,
		    fans_insert_spd_block, fans_delete_cpdlc_arg_block,
		    fans_read_spd_block)) {
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
		fans_set_page(box, FMS_PAGE_REQUESTS, false);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
