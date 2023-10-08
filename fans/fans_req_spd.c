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

#include <stddef.h>
#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fans_req.h"
#include "fans_req_spd.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

static bool
can_verify_spd_req(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (!CPDLC_IS_NULL_SPD(box->spd_req.spd[0]));
}

static void
verify_spd_req(fans_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);

	if (!CPDLC_IS_NULL_SPD(box->spd_req.spd[1])) {
		seg = cpdlc_msg_add_seg(msg, true, 
		    CPDLC_DM19_REQ_spd_TO_spd, 0);
		for (int i = 0; i < 2; i++) {
			cpdlc_msg_seg_set_arg(msg, seg, i,
			    &box->spd_req.spd[i].mach,
			    &box->spd_req.spd[i].spd);
		}
	} else {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM18_REQ_spd, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0,
		    &box->spd_req.spd[0].mach,
		    &box->spd_req.spd[0].spd);
	}
	fans_req_add_common(box, msg, NULL);

	fans_verify_msg(box, msg, "SPD REQ", FMS_PAGE_REQ_SPD, true);
}

static void
draw_main_page(fans_t *box)
{
	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "SPD/SPD BLOCK");
	fans_put_spd(box, LSK1_ROW, 0, false, &box->spd_req.spd[0],
	    NULL, true, false, false);

	fans_put_str(box, LSK1_ROW, 3, false, FMS_COLOR_CYAN,
	    FMS_FONT_SMALL, "/");
	fans_put_spd(box, LSK1_ROW, 4, false, &box->spd_req.spd[1],
	    NULL, false, false, false);

	fans_req_draw_due(box, false);
}

void
fans_req_spd_init_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	memset(&box->req_common, 0, sizeof (box->req_common));
	memset(&box->spd_req, 0, sizeof (box->spd_req));
}

void
fans_req_spd_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  SPEED REQ");
	fans_put_page_ind(box);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	if (can_verify_spd_req(box)) {
		fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}
	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_req_spd_key_cb(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		bool read_back;
		if (fans_scratchpad_xfer_multi(box,
		    (void *)offsetof(fans_t, spd_req.spd),
		    sizeof (cpdlc_spd_t), fans_parse_spd,
		    fans_insert_spd_block, fans_delete_cpdlc_spd_block,
		    fans_read_spd_block, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 &&
	    (key == FMS_KEY_LSK_L2 || key == FMS_KEY_LSK_L3)) {
		fans_req_key_due(box, key);
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_spd_req(box))
			verify_spd_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fans_set_page(box, FMS_PAGE_REQUESTS, false);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
