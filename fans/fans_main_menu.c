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

#include "fans_impl.h"

void
fans_main_menu_draw_cb(fans_t *box)
{
	cpdlc_logon_status_t st;

	CPDLC_ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);

	fans_put_page_title(box, "FANS  MAIN MENU");
	fans_put_lsk_action(box, FMS_KEY_LSK_L1, FMS_COLOR_WHITE,
	    "<LOGON/STATUS");
	fans_put_lsk_action(box, FMS_KEY_LSK_R1, FMS_COLOR_WHITE, "MSG LOG>");
	if (st == CPDLC_LOGON_COMPLETE) {
		fans_put_lsk_action(box, FMS_KEY_LSK_L2, FMS_COLOR_WHITE,
		    "<REQUESTS");
		fans_put_lsk_action(box, FMS_KEY_LSK_R2, FMS_COLOR_WHITE,
		    "EMERGENCY>");
		fans_put_lsk_action(box, FMS_KEY_LSK_L3, FMS_COLOR_WHITE,
		    "<POS REP");
		fans_put_lsk_action(box, FMS_KEY_LSK_R3, FMS_COLOR_WHITE,
		    "REPORTS DUE>");
		fans_put_lsk_action(box, FMS_KEY_LSK_L4, FMS_COLOR_WHITE,
		    "<FREE TEXT");
	}
}

bool
fans_main_menu_key_cb(fans_t *box, fms_key_t key)
{
	cpdlc_logon_status_t st;

	CPDLC_ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);

	if (key == FMS_KEY_LSK_L1)
		fans_set_page(box, FMS_PAGE_LOGON_STATUS, true);
	else if (key == FMS_KEY_LSK_L2 && st == CPDLC_LOGON_COMPLETE)
		fans_set_page(box, FMS_PAGE_REQUESTS, true);
	else if (key == FMS_KEY_LSK_L3 && st == CPDLC_LOGON_COMPLETE)
		fans_set_page(box, FMS_PAGE_POS_REP, true);
	else if (key == FMS_KEY_LSK_R1)
		fans_set_page(box, FMS_PAGE_MSG_LOG, true);
	else if (key == FMS_KEY_LSK_R2 && st == CPDLC_LOGON_COMPLETE)
		fans_set_page(box, FMS_PAGE_EMER, true);
	else if (key == FMS_KEY_LSK_R3 && st == CPDLC_LOGON_COMPLETE)
		fans_set_page(box, FMS_PAGE_REPORTS_DUE, true);
	else if (key == FMS_KEY_LSK_L4 && st == CPDLC_LOGON_COMPLETE)
		fans_set_page(box, FMS_PAGE_FREETEXT, true);
	else
		return (false);

	return (true);
}
