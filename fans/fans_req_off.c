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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fans_parsing.h"
#include "fans_req.h"
#include "fans_req_off.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

static bool
can_verify_off_req(fans_t *box)
{
	ASSERT(box);
	return (box->off_req.off.nm != 0 &&
	    fans_step_at_can_send(&box->off_req.step_at));
}

static void
verify_off_req(fans_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg;

	msg = cpdlc_msg_alloc();

	if (box->off_req.step_at.type != STEP_AT_NONE) {
		if (box->off_req.step_at.type == STEP_AT_TIME) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM17_AT_time_REQ_OFFSET_dir_dist_OF_ROUTE, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    &box->off_req.step_at.tim.hrs,
			    &box->off_req.step_at.tim.mins);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM16_AT_pos_REQ_OFFSET_dir_dist_OF_ROUTE, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    box->off_req.step_at.pos, NULL);
		}
		cpdlc_msg_seg_set_arg(msg, seg, 1, &box->off_req.off.dir, NULL);
		cpdlc_msg_seg_set_arg(msg, seg, 2, &box->off_req.off.nm, NULL);
	} else {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM15_REQ_OFFSET_dir_dist_OF_ROUTE, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->off_req.off.dir, NULL);
		cpdlc_msg_seg_set_arg(msg, seg, 1, &box->off_req.off.nm, NULL);
	}
	fans_req_add_common(box, msg);

	fans_verify_msg(box, msg, "OFF REQ", FMS_PAGE_REQ_OFF, true);
}

static void
draw_main_page(fans_t *box)
{
	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "OFFSET");
	
	fans_put_off(box, LSK1_ROW, 0, false, &box->off_req.off, true);

	fans_req_draw_due(box, true);

	fans_put_step_at(box, &box->off_req.step_at);
}

void
fans_req_off_init_cb(fans_t *box)
{
	ASSERT(box != NULL);
	memset(&box->off_req, 0, sizeof (box->off_req));
}

void
fans_req_off_draw_cb(fans_t *box)
{
	ASSERT(box != NULL);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  OFFSET REQ");
	fans_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	if (can_verify_off_req(box)) {
		fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}

	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_req_off_key_cb(fans_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		fans_scratchpad_xfer_offset(box, &box->off_req.off);
	} else if (box->subpage == 0 &&
	    (key >= FMS_KEY_LSK_L2 && key <= FMS_KEY_LSK_L4)) {
		fans_req_key_due(box, key);
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_off_req(box))
			verify_off_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fans_set_page(box, FMS_PAGE_REQUESTS, false);
	} else if (KEY_IS_REQ_STEP_AT(box, key)) {
		fans_key_step_at(box, key, &box->off_req.step_at);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
