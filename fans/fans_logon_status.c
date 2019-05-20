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
#include "../src/cpdlc_string.h"

#include "fans_impl.h"
#include "fans_scratchpad.h"

static bool
can_send_logon(const fans_t *box, cpdlc_logon_status_t st)
{
	ASSERT(box != NULL);
	return (box->flt_id[0] != '\0' && box->to[0] != '\0' &&
	    (st == CPDLC_LOGON_NONE || st == CPDLC_LOGON_LINK_AVAIL));
}

static void
send_logon(fans_t *box)
{
	char buf[64];

	ASSERT(box != NULL);

	snprintf(buf, sizeof (buf), "%s", box->flt_id);
	cpdlc_client_logon(box->cl, buf, box->flt_id, box->to);
}

static void
send_logoff(fans_t *box)
{
	cpdlc_client_logoff(box->cl);
}

static void
draw_page1(fans_t *box, cpdlc_logon_status_t st)
{
	char cda[32], nda[32];

	cpdlc_client_get_cda(box->cl, cda, sizeof (cda));
	cpdlc_client_get_nda(box->cl, nda, sizeof (nda));

	fans_put_str(box, 1, 1, false, FMS_COLOR_WHITE, FMS_FONT_SMALL,
	    "CDA");
	fans_put_str(box, 1, 5, false, FMS_COLOR_GREEN, FMS_FONT_SMALL, "%s",
	    (st == CPDLC_LOGON_COMPLETE && cda[0] != '\0' ? cda : "--------"));

	fans_put_str(box, 2, 1, false, FMS_COLOR_WHITE, FMS_FONT_SMALL,
	    "NDA");
	fans_put_str(box, 2, 5, false, FMS_COLOR_GREEN, FMS_FONT_SMALL, "%s",
	    (nda[0] != '\0' ? nda : "--------"));

	fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE, FMS_FONT_LARGE,
	    "------------------------");

	if (st == CPDLC_LOGON_NONE) {
		fans_put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, false,
		    FMS_COLOR_WHITE, FMS_FONT_SMALL, "FLT ID");
		if (box->flt_id[0] != '\0') {
			fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "%s", box->flt_id);
		} else {
			fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_CYAN,
			    FMS_FONT_LARGE, "________");
		}
		fans_put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, true,
		    FMS_COLOR_WHITE, FMS_FONT_SMALL, "LOGON TO");
		if (box->to[0] != '\0') {
			fans_put_str(box, LSK4_ROW, 0, true, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "%s", box->to);
		} else {
			fans_put_str(box, LSK4_ROW, 0, true, FMS_COLOR_CYAN,
			    FMS_FONT_LARGE, "____");
		}
	}
}

static void
draw_page2(fans_t *box)
{
	cpdlc_client_t *cl = box->cl;
	const char *host = cpdlc_client_get_host(cl);
	unsigned port = cpdlc_client_get_port(cl);

	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "NETWORK");
	fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
	    FMS_FONT_LARGE, "vCUSTOM");

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "HOSTNAME");
	ASSERT(host != NULL);
	if (strcmp(host, "localhost") == 0) {
		fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "LOCALHOST");
	} else {
		char buf[32];
		cpdlc_strlcpy(buf, host, sizeof (buf));
		for (int i = 0, n = strlen(buf); i < n; i++)
			buf[i] = toupper(buf[i]);
		fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", buf);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "PORT");
	if (port != 0) {
		fans_put_str(box, LSK3_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%d", port);
	} else {
		fans_put_str(box, LSK3_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "DEFAULT");
	}
}

void
fans_logon_status_draw_cb(fans_t *box)
{
	char logon_failure[128];
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, logon_failure);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  LOGON/STATUS");
	fans_put_page_ind(box, FMS_COLOR_WHITE);

	if (logon_failure[0] != '\0') {
		for (int i = 0, n = strlen(logon_failure); i < n; i++)
			logon_failure[i] = toupper(logon_failure[i]);
		fans_set_error(box, logon_failure);
		cpdlc_client_reset_logon_failure(box->cl);
	}

	if (can_send_logon(box, st)) {
		fans_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "SEND LOGON*");
	} else if (st == CPDLC_LOGON_CONNECTING_LINK ||
	    st == CPDLC_LOGON_HANDSHAKING_LINK || st == CPDLC_LOGON_IN_PROG) {
		fans_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_WHITE,
		    "IN PROGRESS");
	} else if (st == CPDLC_LOGON_COMPLETE) {
		fans_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "LOG OFF*");
	}
	if (st == CPDLC_LOGON_COMPLETE) {
		fans_put_str(box, LSK_HEADER_ROW(LSK5_ROW), 5, true,
		    FMS_COLOR_GREEN, FMS_FONT_SMALL, "LOGGED ON TO %s",
		    box->to);
	} else {
		fans_put_str(box, LSK_HEADER_ROW(LSK5_ROW), 5, true,
		    FMS_COLOR_GREEN, FMS_FONT_SMALL, "LOGON REQUIRED");
	}

	fans_put_atc_status(box);

	if (box->subpage == 0)
		draw_page1(box, st);
	else
		draw_page2(box);
}

bool
fans_logon_status_key_cb(fans_t *box, fms_key_t key)
{
	cpdlc_logon_status_t st = cpdlc_client_get_logon_status(box->cl, NULL);

	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		fans_scratchpad_xfer(box, box->flt_id, sizeof (box->flt_id),
		    st <= CPDLC_LOGON_LINK_AVAIL);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		fans_scratchpad_xfer(box, box->to, sizeof (box->to),
		    st <= CPDLC_LOGON_LINK_AVAIL);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L2) {
		const char *host = cpdlc_client_get_host(box->cl);
		char buf[32];

		cpdlc_strlcpy(buf, host, sizeof (buf));
		for (int i = 0, n = strlen(buf); i < n; i++)
			buf[i] = toupper(buf[i]);
		fans_scratchpad_xfer(box, buf, sizeof (buf), true);
		if (strlen(buf) != 0) {
			for (int i = 0, n = strlen(buf); i < n; i++)
				buf[i] = tolower(buf[i]);
			cpdlc_client_set_host(box->cl, buf);
		} else {
			cpdlc_client_set_host(box->cl, "localhost");
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L3) {
		bool set = true;
		unsigned port = cpdlc_client_get_port(box->cl);
		fans_scratchpad_xfer_uint(box, &port, &set, 0, UINT16_MAX);
		if (set)
			cpdlc_client_set_port(box->cl, port);
		else
			cpdlc_client_set_port(box->cl, 0);
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
