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

#include "fans_req.h"
#include "fans_req_vmc.h"
#include "fans_vrfy.h"

static void
verify_vmc_req(fans_t *box)
{
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);

	cpdlc_msg_add_seg(msg, true, CPDLC_DM69_REQ_VMC_DES, 0);
	fans_req_add_common(box, msg);
	fans_verify_msg(box, msg, "VMC REQ", FMS_PAGE_REQ_VMC, true);
}

static void
draw_main_page(fans_t *box)
{
	fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_CYAN,
	    FMS_FONT_SMALL, "VMC DESCENT");
	fans_req_draw_due(box, false);
}

void
fans_req_vmc_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  VMC REQ");
	fans_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE, "<VERIFY");
	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_req_vmc_key_cb(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 &&
	    (key == FMS_KEY_LSK_L2 || key == FMS_KEY_LSK_L3)) {
		fans_req_key_due(box, key);
	} else if (key == FMS_KEY_LSK_L5) {
		verify_vmc_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fans_set_page(box, FMS_PAGE_REQUESTS, false);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
