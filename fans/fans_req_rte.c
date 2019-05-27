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

#include "fans_pos_pick.h"
#include "fans_req.h"
#include "fans_req_rte.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

static void
draw_main_page(fans_t *box)
{
	ASSERT(box != NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "DIRECT TO");
	fans_put_pos(box, LSK1_ROW, 0, false, &box->rte_req.dct, false, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "WX DEV TO");
	fans_put_pos(box, LSK2_ROW, 0, false, &box->rte_req.wx_dev,
	    false, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R1, "HEADING");
	fans_put_hdg(box, LSK1_ROW, 0, true, &box->rte_req.hdg, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R2, "GROUND TRK");
	fans_put_hdg(box, LSK2_ROW, 0, true, &box->rte_req.trk, false);
}

static bool
can_verify_rte_req(fans_t *box)
{
	ASSERT(box != NULL);
	return (box->rte_req.dct.set || box->rte_req.wx_dev.set ||
	    box->rte_req.hdg.set || box->rte_req.trk.set);
}

static void
verify_rte_req(fans_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
	char buf[32];

	if (box->rte_req.dct.set) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM22_REQ_DIR_TO_pos, 0);
		fans_print_pos(&box->rte_req.dct, buf, sizeof (buf),
		    POS_PRINT_NORM);
		cpdlc_msg_seg_set_arg(msg, seg, 0, buf, NULL);
	} else if (box->rte_req.wx_dev.set) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM26_REQ_WX_DEVIATION_TO_pos_VIA_route, 0);
		fans_print_pos(&box->rte_req.wx_dev, buf, sizeof (buf),
		    POS_PRINT_NORM);
		cpdlc_msg_seg_set_arg(msg, seg, 0, buf, NULL);
		cpdlc_msg_seg_set_arg(msg, seg, 1, "DCT", NULL);
	} else if (box->rte_req.hdg.set) {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM70_REQ_HDG_deg, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->rte_req.hdg.hdg,
		    &box->rte_req.hdg.tru);
	} else {
		ASSERT(box->rte_req.trk.set);
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM71_REQ_GND_TRK_deg, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->rte_req.trk.hdg,
		    &box->rte_req.trk.tru);
	}

	fans_verify_msg(box, msg, "RTE REQ", FMS_PAGE_REQ_RTE, true);
}

static void
set_dct(fans_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->rte_req.dct, pos, sizeof (box->rte_req.dct));
	box->rte_req.wx_dev.set = false;
	box->rte_req.hdg.set = false;
	box->rte_req.trk.set = false;
}

static void
set_wx_dev(fans_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	box->rte_req.dct.set = false;
	memcpy(&box->rte_req.wx_dev, pos, sizeof (box->rte_req.wx_dev));
	box->rte_req.hdg.set = false;
	box->rte_req.trk.set = false;
}

void
fans_req_rte_init_cb(fans_t *box)
{
	ASSERT(box != NULL);
	memset(&box->rte_req, 0, sizeof (box->rte_req));
}

void
fans_req_rte_draw_cb(fans_t *box)
{
	ASSERT(box != NULL);

	fans_put_page_title(box, "FANS  ROUTE REQ");

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  ROUTE REQ");
	fans_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	if (can_verify_rte_req(box)) {
		fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}
	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_req_rte_key_cb(fans_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		fans_scratchpad_xfer_pos(box, &box->rte_req.dct,
		    FMS_PAGE_REQ_RTE, set_dct);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L2) {
		fans_scratchpad_xfer_pos(box, &box->rte_req.wx_dev,
		    FMS_PAGE_REQ_RTE, set_wx_dev);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R1) {
		fans_scratchpad_xfer_hdg(box, &box->rte_req.hdg);
		if (box->rte_req.hdg.set) {
			box->rte_req.dct.set = false;
			box->rte_req.wx_dev.set = false;
			box->rte_req.trk.set = false;
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R2) {
		fans_scratchpad_xfer_hdg(box, &box->rte_req.trk);
		if (box->rte_req.trk.set) {
			box->rte_req.dct.set = false;
			box->rte_req.wx_dev.set = false;
			box->rte_req.hdg.set = false;
		}
	} else if (key == FMS_KEY_LSK_L5) {
	if (can_verify_rte_req(box))
		verify_rte_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fans_set_page(box, FMS_PAGE_REQUESTS, false);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
