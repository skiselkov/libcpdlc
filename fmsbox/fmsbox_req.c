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

#include "fmsbox_req.h"

void
fmsbox_requests_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_put_page_title(box, "FANS  REQUESTS");

	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L1, FMS_COLOR_WHITE,
	    "<ALTITUDE");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L2, FMS_COLOR_WHITE,
	    "<OFFSET");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L3, FMS_COLOR_WHITE,
	    "<SPEED");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L4, FMS_COLOR_WHITE,
	    "<ROUTE");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_R1, FMS_COLOR_WHITE,
	    "CLEARANCE>");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_R2, FMS_COLOR_WHITE,
	    "VMC DESCENT>");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_R3, FMS_COLOR_WHITE,
	    "WHEN CAN WE>");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_R4, FMS_COLOR_WHITE,
	    "VOICE REQ>");
}

bool
fmsbox_requests_key_cb(fmsbox_t *box, fms_key_t key)
{
	enum { MAX_LINES = 4 };

	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L1) {
		fmsbox_set_page(box, FMS_PAGE_REQ_ALT);
		memset(&box->alt_req, 0, sizeof (box->alt_req));
	} else if (key == FMS_KEY_LSK_L2) {
		fmsbox_set_page(box, FMS_PAGE_REQ_OFF);
		memset(&box->off_req, 0, sizeof (box->off_req));
	} else if (key == FMS_KEY_LSK_L3) {
		fmsbox_set_page(box, FMS_PAGE_REQ_SPD);
	} else if (key == FMS_KEY_LSK_L4) {
		fmsbox_set_page(box, FMS_PAGE_REQ_RTE);
	} else if (key == FMS_KEY_LSK_R1) {
		fmsbox_set_page(box, FMS_PAGE_REQ_CLX);
	} else if (key == FMS_KEY_LSK_R2) {
		fmsbox_set_page(box, FMS_PAGE_REQ_VMC);
	} else if (key == FMS_KEY_LSK_R3) {
		fmsbox_set_page(box, FMS_PAGE_REQ_WCW);
	} else if (key == FMS_KEY_LSK_R4) {
		fmsbox_set_page(box, FMS_PAGE_REQ_VOICE);
	} else {
		return (false);
	}

	return (true);
}
