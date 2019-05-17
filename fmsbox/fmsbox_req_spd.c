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

#include "../src/cpdlc_assert.h"

#include "fmsbox_req.h"
#include "fmsbox_req_spd.h"
#include "fmsbox_scratchpad.h"
#include "fmsbox_vrfy.h"

static bool
can_verify_spd_req(fmsbox_t *box)
{
	ASSERT(box != NULL);
	return (box->spd_req.spd[0].spd.spd != 0);
}

static void
verify_spd_req(fmsbox_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc();

	if (box->spd_req.spd[1].spd.spd != 0) {
		seg = cpdlc_msg_add_seg(msg, true, 
		    CPDLC_DM19_REQ_spd_TO_spd, 0);
		for (int i = 0; i < 2; i++) {
			cpdlc_msg_seg_set_arg(msg, seg, i,
			    &box->spd_req.spd[i].spd.mach,
			    &box->alt_req.alt[i].spd.spd);
		}
	} else {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM18_REQ_spd, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0,
		    &box->spd_req.spd[0].spd.mach,
		    &box->alt_req.alt[0].spd.spd);
	}
	fmsbox_req_add_common(box, msg);

	fmsbox_verify_msg(box, msg, "SPD REQ", FMS_PAGE_REQ_SPD, true);
}

static void
draw_main_page(fmsbox_t *box)
{
	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "SPD/SPD BLOCK");
	fmsbox_put_spd(box, LSK1_ROW, 0, false, &box->spd_req.spd[0],
	    true, false);

	fmsbox_put_str(box, LSK1_ROW, 3, false, FMS_COLOR_CYAN,
	    FMS_FONT_SMALL, "/");
	fmsbox_put_spd(box, LSK1_ROW, 4, false, &box->spd_req.spd[1],
	    false, false);

	fmsbox_req_draw_due(box, false);
}

void
fmsbox_req_spd_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_set_num_subpages(box, 2);

	fmsbox_put_page_title(box, "FANS  SPEED REQ");
	fmsbox_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fmsbox_req_draw_freetext(box);

	if (can_verify_spd_req(box)) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fmsbox_req_spd_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		fmsbox_scratchpad_xfer_multi(box,
		    (void *)offsetof(fmsbox_t, spd_req.spd),
		    sizeof (cpdlc_arg_t), fmsbox_parse_spd,
		    fmsbox_insert_spd_block, fmsbox_delete_cpdlc_arg_block,
		    fmsbox_read_spd_block);
	} else if (box->subpage == 0 &&
	    (key == FMS_KEY_LSK_L2 || key == FMS_KEY_LSK_L3)) {
		fmsbox_req_key_due(box, key);
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_spd_req(box))
			verify_spd_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fmsbox_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
