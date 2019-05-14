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

#include "fmsbox_impl.h"

void
fmsbox_main_menu_draw_cb(fmsbox_t *box)
{
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);

	fmsbox_put_page_title(box, "FANS  MAIN MENU");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L1, FMS_COLOR_WHITE,
	    "<LOGON/STATUS");
	if (st == CPDLC_LOGON_COMPLETE) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_R1, FMS_COLOR_WHITE,
		    "MSG LOG>");
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L2, FMS_COLOR_WHITE,
		    "<REQUESTS");
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_R2, FMS_COLOR_WHITE,
		    "EMERGENCY>");
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L3, FMS_COLOR_WHITE,
		    "<POS REP");
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_R3, FMS_COLOR_WHITE,
		    "REPORTS DUE>");
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L4, FMS_COLOR_WHITE,
		    "<FREE TEXT");
	}

	fmsbox_put_atc_status(box);
}

bool
fmsbox_main_menu_key_cb(fmsbox_t *box, fms_key_t key)
{
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);

	if (key == FMS_KEY_LSK_L1)
		fmsbox_set_page(box, FMS_PAGE_LOGON_STATUS);
	else if (key == FMS_KEY_LSK_L2)
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	else if (key == FMS_KEY_LSK_R1 && st == CPDLC_LOGON_COMPLETE)
		fmsbox_set_page(box, FMS_PAGE_MSG_LOG);
	else if (key == FMS_KEY_LSK_L4 && st == CPDLC_LOGON_COMPLETE)
		fmsbox_set_page(box, FMS_PAGE_FREETEXT);
	else
		return (false);

	return (true);
}
