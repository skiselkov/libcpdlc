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

#include <stdio.h>

#include "../src/cpdlc_assert.h"

#include "fmsbox_impl.h"
#include "fmsbox_scratchpad.h"

static bool
can_send_logon(const fmsbox_t *box, cpdlc_logon_status_t st)
{
	ASSERT(box != NULL);
	return (box->flt_id[0] != '\0' && box->to[0] != '\0' &&
	    (st == CPDLC_LOGON_NONE || st == CPDLC_LOGON_LINK_AVAIL));
}

static void
send_logon(fmsbox_t *box)
{
	char buf[64];
    
	ASSERT(box != NULL);

	snprintf(buf, sizeof (buf), "%s", box->flt_id);
	cpdlc_client_logon(box->cl, buf, box->flt_id, box->to);
}

static void
send_logoff(fmsbox_t *box)
{
	cpdlc_client_logoff(box->cl);
}

void
fmsbox_logon_status_draw_cb(fmsbox_t *box)
{
	cpdlc_logon_status_t st;
	bool logon_failed;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, &logon_failed);

	fmsbox_put_page_title(box, "FANS  LOGON/STATUS");
	if (logon_failed) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "LOGON FAILED*");
	} else if (can_send_logon(box, st)) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "SEND LOGON*");
	} else if (st == CPDLC_LOGON_CONNECTING_LINK ||
	    st == CPDLC_LOGON_HANDSHAKING_LINK || st == CPDLC_LOGON_IN_PROG) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_WHITE,
		    "IN PROGRESS");
	} else if (st == CPDLC_LOGON_COMPLETE) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "LOG OFF*");
	}

	fmsbox_put_str(box, 1, 1, false, FMS_COLOR_WHITE, FMS_FONT_SMALL,
	    "CDA");
	if (st == CPDLC_LOGON_COMPLETE) {
		fmsbox_put_str(box, 1, 5, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", box->to);
	} else {
		fmsbox_put_str(box, 1, 5, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "--------");
	}
	fmsbox_put_str(box, 2, 1, false, FMS_COLOR_WHITE, FMS_FONT_SMALL,
	    "NDA");
	fmsbox_put_str(box, 2, 5, false, FMS_COLOR_GREEN, FMS_FONT_SMALL,
	    "--------");
	fmsbox_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE, FMS_FONT_LARGE,
	    "------------------------");
	fmsbox_put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, false, FMS_COLOR_WHITE,
	    FMS_FONT_SMALL, "FLT ID");
	if (box->flt_id[0] != '\0') {
		fmsbox_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", box->flt_id);
	} else {
		fmsbox_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "--------");
	}
	fmsbox_put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, true, FMS_COLOR_WHITE,
	    FMS_FONT_SMALL, "LOGON TO");
	if (box->to[0] != '\0') {
		fmsbox_put_str(box, LSK4_ROW, 0, true, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", box->to);
	} else {
		fmsbox_put_str(box, LSK4_ROW, 0, true, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "----");
	}
	if (st == CPDLC_LOGON_COMPLETE) {
		fmsbox_put_str(box, LSK_HEADER_ROW(LSK5_ROW), 5, true,
		    FMS_COLOR_GREEN, FMS_FONT_SMALL, "LOGGED ON TO %s",
		    box->to);
	} else {
		fmsbox_put_str(box, LSK_HEADER_ROW(LSK5_ROW), 5, true,
		    FMS_COLOR_GREEN, FMS_FONT_SMALL, "LOGON REQUIRED");
	}

	fmsbox_put_atc_status(box);
}

bool
fmsbox_logon_status_key_cb(fmsbox_t *box, fms_key_t key)
{
	cpdlc_logon_status_t st = cpdlc_client_get_logon_status(box->cl, NULL);

	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L4) {
		fmsbox_scratchpad_xfer(box, box->flt_id, sizeof (box->flt_id),
		    st <= CPDLC_LOGON_LINK_AVAIL);
	} else if (key == FMS_KEY_LSK_R4) {
		fmsbox_scratchpad_xfer(box, box->to, sizeof (box->to),
		    st <= CPDLC_LOGON_LINK_AVAIL);
	} else if (key == FMS_KEY_LSK_R5) {
		if (can_send_logon(box, st)) {
			send_logon(box);
		} else if (st == CPDLC_LOGON_COMPLETE) {
			send_logoff(box);
		}
	} else {
		return (false);
	}

	return (true);
}
