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
#include "fans_pos_pick.h"
#include "fans_scratchpad.h"

void
fans_pos_pick_start(fans_t *box, pos_pick_done_cb_t done_cb,
    unsigned ret_page, const cpdlc_pos_t *old_pos)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(done_cb != NULL);
	CPDLC_ASSERT3U(ret_page, <, FMS_NUM_PAGES);
	CPDLC_ASSERT(ret_page != FMS_PAGE_POS_PICK);
	CPDLC_ASSERT(old_pos != NULL);

	memset(&box->pos_pick, 0, sizeof (box->pos_pick));

	memcpy(&box->pos_pick, old_pos, sizeof (box->pos_pick));
	box->pos_pick.was_set = old_pos->set;
	box->pos_pick.ret_page = ret_page;
	box->pos_pick.done_cb = done_cb;

	fans_set_page(box, FMS_PAGE_POS_PICK, true);
}

void
fans_pos_pick_draw_cb(fans_t *box)
{
	char buf[32];

	CPDLC_ASSERT(box != NULL);

	fans_put_page_title(box, "CPDLC POSITION");

	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "POSITION");
	switch (box->pos_pick.pos.type) {
	case CPDLC_POS_NAVAID:
		fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "vNAVAID");
		fans_put_lsk_title(box, FMS_KEY_LSK_L2, "NAVAID");
		break;
	case CPDLC_POS_AIRPORT:
		fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "vAIRPORT");
		fans_put_lsk_title(box, FMS_KEY_LSK_L2, "AIRPORT");
		break;
	case CPDLC_POS_FIXNAME:
		fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "vFIX");
		fans_put_lsk_title(box, FMS_KEY_LSK_L2, "FIX");
		break;
	case CPDLC_POS_LAT_LON:
		fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "vLAT/LON");
		fans_put_lsk_title(box, FMS_KEY_LSK_L2, "LAT/LON");
		break;
	case CPDLC_POS_PBD:
		fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "vPLACE/BEARING/DISTANCE");
		fans_put_lsk_title(box, FMS_KEY_LSK_L2,
		    "PLACE/BEARING/DISTANCE");
		break;
	default:
		break;
	}

	fans_print_pos(&box->pos_pick.pos, buf, sizeof (buf),
	    POS_PRINT_PRETTY);
	fans_put_str(box, LSK2_ROW, 0, false,
	    FMS_COLOR_WHITE, FMS_FONT_LARGE, "%s", buf);

	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_pos_pick_key_cb(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L1) {
		box->pos_pick.pos.type = (box->pos_pick.pos.type + 1) %
		    (CPDLC_POS_PBD + 1);
		box->pos_pick.pos.set = false;
	} else if (key == FMS_KEY_LSK_L2) {
		bool read_back;
		if (fans_scratchpad_xfer_pos_impl(box, &box->pos_pick.pos,
		    &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (key == FMS_KEY_LSK_L6) {
		CPDLC_ASSERT(box->pos_pick.done_cb != NULL);
		if (box->pos_pick.pos.set || box->pos_pick.was_set)
			box->pos_pick.done_cb(box, &box->pos_pick.pos);
		fans_set_page(box, box->pos_pick.ret_page, false);
	} else {
		return (false);
	}

	return (true);
}
