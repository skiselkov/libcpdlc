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

static void
fans_strtoupper(char *buf)
{
	CPDLC_ASSERT(buf != NULL);
	for (int i = 0, n = strlen(buf); i < n; i++)
		buf[i] = toupper(buf[i]);
}

static bool
can_send_logon(const fans_t *box, cpdlc_logon_status_t st)
{
	CPDLC_ASSERT(box != NULL);
	return ((box->flt_id[0] != '\0' || box->flt_id_auto[0] != '\0') &&
	    box->to[0] != '\0' &&
	    (st == CPDLC_LOGON_NONE || st == CPDLC_LOGON_LINK_AVAIL));
}

static void
send_logon(fans_t *box)
{
	char flt_id[32], secret[32];

	CPDLC_ASSERT(box != NULL);

	if (strlen(box->flt_id) != 0)
		cpdlc_strlcpy(flt_id, box->flt_id, sizeof (flt_id));
	else
		cpdlc_strlcpy(flt_id, box->flt_id_auto, sizeof (flt_id));

	switch (box->net) {
	case FANS_NETWORK_CUSTOM:
		cpdlc_client_set_host(box->cl, box->hostname);
		cpdlc_client_set_port(box->cl, box->port);
		if (strcmp(box->secret, "") == 0)
			cpdlc_strlcpy(secret, flt_id, sizeof (secret));
		else
			cpdlc_strlcpy(secret, box->secret, sizeof (secret));
		break;
	case FANS_NETWORK_PILOTEDGE:
		cpdlc_client_set_host(box->cl, "vspro.pilotedge.net");
		cpdlc_client_set_port(box->cl, 0);
		cpdlc_strlcpy(secret, flt_id, sizeof (secret));
		break;
	}
	cpdlc_client_logon(box->cl, secret, flt_id, box->to);
	memset(secret, 0, sizeof (secret));
}

static void
send_logoff(fans_t *box)
{
	cpdlc_client_logoff(box->cl, NULL);
}

static void
draw_page1(fans_t *box, cpdlc_logon_status_t st)
{
	char cda[8], nda[8];

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

	fans_put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, false,
	    FMS_COLOR_WHITE, FMS_FONT_SMALL, "FLT ID");
	if (box->flt_id[0] != '\0') {
		if (st <= CPDLC_LOGON_LINK_AVAIL) {
			fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "%s", box->flt_id);
		} else {
			fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_GREEN,
			    FMS_FONT_SMALL, "%s", box->flt_id);
		}
	} else if (box->flt_id_auto[0] != '\0') {
		fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", box->flt_id_auto);
	} else {
		fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, "_______");
	}
	fans_put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, true,
	    FMS_COLOR_WHITE, FMS_FONT_SMALL, "LOGON TO");
	if (box->to[0] != '\0') {
		if (st <= CPDLC_LOGON_LINK_AVAIL) {
			fans_put_str(box, LSK4_ROW, 0, true, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "%s", box->to);
		} else {
			fans_put_str(box, LSK4_ROW, 0, true, FMS_COLOR_GREEN,
			    FMS_FONT_SMALL, "%s", box->to);
		}
	} else {
		fans_put_str(box, LSK4_ROW, 0, true, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, "____");
	}
}

static void
draw_page2(fans_t *box)
{
	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "NETWORK");
	switch (box->net) {
	case FANS_NETWORK_CUSTOM:
		fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "vCUSTOM");

		fans_put_lsk_title(box, FMS_KEY_LSK_L2, "HOSTNAME");
		if (strcmp(box->hostname, "localhost") == 0) {
			fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_GREEN,
			    FMS_FONT_SMALL, "LOCALHOST");
		} else {
			char buf[32];
			cpdlc_strlcpy(buf, box->hostname, sizeof (buf));
			fans_strtoupper(buf);
			fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "%s", buf);
		}

		fans_put_lsk_title(box, FMS_KEY_LSK_L3, "PORT");
		if (box->port != 0) {
			fans_put_str(box, LSK3_ROW, 0, false, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "%d", box->port);
		} else {
			fans_put_str(box, LSK3_ROW, 0, false, FMS_COLOR_GREEN,
			    FMS_FONT_SMALL, "DEFAULT");
		}

		fans_put_lsk_title(box, FMS_KEY_LSK_L4, "SECRET");
		if (strlen(box->secret) != 0) {
			fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "********");
		} else {
			fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "--------");
		}
		break;
	case FANS_NETWORK_PILOTEDGE:
		fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "vPILOTEDGE");
		break;
	}
}

void
fans_logon_status_init_cb(fans_t *box)
{
	if (box->funcs.get_flt_id != NULL &&
	    !box->funcs.get_flt_id(box->userinfo, box->flt_id_auto)) {
		memset(box->flt_id_auto, 0, sizeof (box->flt_id_auto));
	}
}

void
fans_logon_status_draw_cb(fans_t *box)
{
	char logon_failure[128];
	cpdlc_logon_status_t st;

	CPDLC_ASSERT(box != NULL);
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
		char cda[8];
		cpdlc_client_get_cda(box->cl, cda, sizeof (cda));
		fans_put_str(box, LSK_HEADER_ROW(LSK5_ROW), 3, false,
		    FMS_COLOR_GREEN, FMS_FONT_SMALL, "LOGGED ON TO %s", cda);
	} else {
		fans_put_str(box, LSK_HEADER_ROW(LSK5_ROW), 5, false,
		    FMS_COLOR_GREEN, FMS_FONT_SMALL, "LOGON REQUIRED");
	}

	if (box->subpage == 0)
		draw_page1(box, st);
	else
		draw_page2(box);
}

bool
fans_logon_status_key_cb(fans_t *box, fms_key_t key)
{
	cpdlc_logon_status_t st = cpdlc_client_get_logon_status(box->cl, NULL);

	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		fans_scratchpad_xfer_auto(box, box->flt_id, box->flt_id_auto,
		    sizeof (box->flt_id), st <= CPDLC_LOGON_LINK_AVAIL);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		fans_scratchpad_xfer(box, box->to, sizeof (box->to),
		    st <= CPDLC_LOGON_LINK_AVAIL);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L1) {
		box->net++;
		if (box->net > FANS_NETWORK_PILOTEDGE)
			box->net = FANS_NETWORK_CUSTOM;
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L2 &&
	    box->net == FANS_NETWORK_CUSTOM) {
		const char *host = cpdlc_client_get_host(box->cl);
		char buf[32];

		cpdlc_strlcpy(buf, host, sizeof (buf));
		for (int i = 0, n = strlen(buf); i < n; i++)
			buf[i] = toupper(buf[i]);
		fans_scratchpad_xfer(box, buf, sizeof (buf), true);
		if (strlen(buf) != 0) {
			for (int i = 0, n = strlen(buf); i < n; i++)
				buf[i] = tolower(buf[i]);
			cpdlc_strlcpy(box->hostname, buf,
			    sizeof (box->hostname));
		} else {
			cpdlc_strlcpy(box->hostname, "localhost",
			    sizeof (box->hostname));
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L3 &&
	    box->net == FANS_NETWORK_CUSTOM) {
		bool set = true;
		unsigned port = cpdlc_client_get_port(box->cl);
		fans_scratchpad_xfer_uint(box, &port, &set, 0, UINT16_MAX);
		if (set)
			box->port = port;
		else
			box->port = 0;
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L4 &&
	    box->net == FANS_NETWORK_CUSTOM) {
		if (fans_scratchpad_is_delete(box)) {
			memset(box->secret, 0, sizeof (box->secret));
		} else if (!fans_scratchpad_is_empty(box)) {
			cpdlc_strlcpy(box->secret, fans_scratchpad_get(box),
			    sizeof (box->secret));
		}
		fans_scratchpad_clear(box);
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
