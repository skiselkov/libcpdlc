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
	box->logon_timed_out = false;
	box->logon_rejected = false;
	box->logon_started = time(NULL);
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
	fms_color_t cust_color, dfl_color;
	cpdlc_logon_status_t st;
	char mod_char;
	fms_font_t font;

	CPDLC_ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);
	font = st == CPDLC_LOGON_NONE ? FMS_FONT_LARGE : FMS_FONT_SMALL;

	if (st == CPDLC_LOGON_NONE) {
		cust_color = FMS_COLOR_WHITE;
		dfl_color = FMS_COLOR_GREEN;
		mod_char = 'v';
	} else {
		cust_color = FMS_COLOR_WHITE;
		dfl_color = FMS_COLOR_WHITE;
		mod_char = ' ';
	}

	if (box->net_select)
		fans_put_lsk_title(box, FMS_KEY_LSK_L1, "NETWORK");
	switch (box->net) {
	case FANS_NETWORK_CUSTOM:
		if (box->net_select) {
			fans_put_str(box, LSK1_ROW, 0, false, cust_color,
			    font, "%cCUSTOM", mod_char);
		}

		fans_put_lsk_title(box, FMS_KEY_LSK_L2, "HOSTNAME");
		if (strcmp(box->hostname, "localhost") == 0) {
			fans_put_str(box, LSK2_ROW, 0, false, dfl_color,
			    FMS_FONT_SMALL, "LOCALHOST");
		} else {
			char buf[32];
			cpdlc_strlcpy(buf, box->hostname, sizeof (buf));
			fans_strtoupper(buf);
			fans_put_str(box, LSK2_ROW, 0, false, cust_color,
			    font, "%s", buf);
		}

		fans_put_lsk_title(box, FMS_KEY_LSK_L3, "PORT");
		if (box->port != 0) {
			fans_put_str(box, LSK3_ROW, 0, false, cust_color,
			    font, "%d", box->port);
		} else {
			fans_put_str(box, LSK3_ROW, 0, false, dfl_color,
			    FMS_FONT_SMALL, "DEFAULT");
		}

		fans_put_lsk_title(box, FMS_KEY_LSK_L4, "SECRET");
		if (strlen(box->secret) != 0) {
			fans_put_str(box, LSK4_ROW, 0, false, cust_color,
			    FMS_FONT_LARGE, "********");
		} else {
			fans_put_str(box, LSK4_ROW, 0, false, cust_color,
			    FMS_FONT_LARGE, "--------");
		}
		break;
	case FANS_NETWORK_PILOTEDGE:
		if (box->net_select) {
			fans_put_str(box, LSK1_ROW, 0, false, cust_color,
			    FMS_FONT_LARGE, "vPILOTEDGE");
		}
		break;
	}
	if (box->show_volume) {
		fans_put_lsk_title(box, FMS_KEY_LSK_R1, "ALERT VOLUME");
		fans_put_str(box, LSK1_ROW, 0, true, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%d%%", (int)(box->volume * 100));
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
	char msg[32];
	cpdlc_logon_status_t st;

	CPDLC_ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);

	fans_set_num_subpages(box, box->logon_has_page2 ? 2 : 0);

	fans_put_page_title(box, "FANS  LOGON/STATUS");
	fans_put_page_ind(box);

	if (can_send_logon(box, st)) {
		fans_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "SEND LOGON*");
	} else if (st == CPDLC_LOGON_CONNECTING_LINK ||
	    st == CPDLC_LOGON_HANDSHAKING_LINK || st == CPDLC_LOGON_IN_PROG) {
		fans_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "CANCEL*");
	} else if (st == CPDLC_LOGON_COMPLETE) {
		fans_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "LOG OFF*");
	}
	switch (st) {
	case CPDLC_LOGON_NONE:
		if (box->logon_timed_out)
			cpdlc_strlcpy(msg, "TIMED OUT", sizeof (msg));
		else if (box->logon_rejected)
			snprintf(msg, sizeof (msg), "REJECTED BY %s", box->to);
		else
			cpdlc_strlcpy(msg, "LOGON REQUIRED", sizeof (msg));
		break;
	case CPDLC_LOGON_LINK_AVAIL:
		cpdlc_strlcpy(msg, "LOGON REQUIRED", sizeof (msg));
		break;
	case CPDLC_LOGON_CONNECTING_LINK:
	case CPDLC_LOGON_HANDSHAKING_LINK:
	case CPDLC_LOGON_IN_PROG:
		snprintf(msg, sizeof (msg), "CONTACTING %s", box->to);
		break;
	case CPDLC_LOGON_COMPLETE:
		snprintf(msg, sizeof (msg), "LOGGED ON TO %s", box->to);
		break;
	default:
		CPDLC_VERIFY(0);
	}
	fans_put_str(box, LSK_HEADER_ROW(LSK5_ROW),
	    (FMS_COLS - strlen(msg)) / 2, false, FMS_COLOR_GREEN,
	    FMS_FONT_SMALL, "%s", msg);

	if (box->subpage == 0)
		draw_page1(box, st);
	else
		draw_page2(box);
}

bool
fans_logon_status_key_cb(fans_t *box, fms_key_t key)
{
	cpdlc_logon_status_t st = cpdlc_client_get_logon_status(box->cl, NULL);
	bool read_back;

	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		if (fans_scratchpad_xfer_auto(box, box->flt_id,
		    box->flt_id_auto, sizeof (box->flt_id),
		    st <= CPDLC_LOGON_LINK_AVAIL, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		if (fans_scratchpad_xfer(box, box->to, sizeof (box->to),
		    st <= CPDLC_LOGON_LINK_AVAIL, &read_back) && !read_back) {
			box->logon_rejected = false;
			box->logon_timed_out = false;
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L1 &&
	    box->net_select) {
		if (st == CPDLC_LOGON_NONE) {
			box->net++;
			if (box->net > FANS_NETWORK_PILOTEDGE)
				box->net = FANS_NETWORK_CUSTOM;
		} else {
			fans_set_error(box, FANS_ERR_BUTTON_PUSH_IGNORED);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L2 &&
	    box->net == FANS_NETWORK_CUSTOM) {
		const char *host = cpdlc_client_get_host(box->cl);
		char buf[32];

		cpdlc_strlcpy(buf, host, sizeof (buf));
		for (int i = 0, n = strlen(buf); i < n; i++)
			buf[i] = toupper(buf[i]);
		if (fans_scratchpad_xfer(box, buf, sizeof (buf),
		    st == CPDLC_LOGON_NONE, &read_back)) {
			if (strlen(buf) != 0) {
				for (int i = 0, n = strlen(buf); i < n; i++)
					buf[i] = tolower(buf[i]);
				cpdlc_strlcpy(box->hostname, buf,
				    sizeof (box->hostname));
			} else {
				cpdlc_strlcpy(box->hostname, "localhost",
				    sizeof (box->hostname));
			}
			if (!read_back)
				fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L3 &&
	    box->net == FANS_NETWORK_CUSTOM) {
		if (st == CPDLC_LOGON_NONE) {
			bool set = true;
			unsigned port = cpdlc_client_get_port(box->cl);

			if (fans_scratchpad_xfer_uint(box, &port, &set, 0,
			    UINT16_MAX, &read_back)) {
				if (set)
					box->port = port;
				else
					box->port = 0;
				if (!read_back)
					fans_scratchpad_clear(box);
			}
		} else {
			fans_set_error(box, FANS_ERR_NO_ENTRY_ALLOWED);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L4 &&
	    box->net == FANS_NETWORK_CUSTOM) {
		if (st == CPDLC_LOGON_NONE) {
			if (fans_scratchpad_is_delete(box)) {
				memset(box->secret, 0, sizeof (box->secret));
			} else if (!fans_scratchpad_is_empty(box)) {
				cpdlc_strlcpy(box->secret,
				    fans_scratchpad_get(box),
				    sizeof (box->secret));
			}
			fans_scratchpad_clear(box);
		} else {
			fans_set_error(box, FANS_ERR_NO_ENTRY_ALLOWED);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R1 &&
	    box->show_volume) {
		unsigned vol = box->volume * 100;
		bool set;

		if (fans_scratchpad_xfer_uint(box, &vol, &set, 0, 100,
		    &read_back)) {
			if (set)
				box->volume = vol / 100.0;
			if (!read_back)
				fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R3 &&
	    strcmp(fans_scratchpad_get(box), "DEBUG") == 0) {
		fans_set_page(box, FMS_PAGE_FMS_DATA, true);
		fans_scratchpad_clear(box);
	} else if (key == FMS_KEY_LSK_R5) {
		if (can_send_logon(box, st))
			send_logon(box);
		else if (st != CPDLC_LOGON_NONE)
			cpdlc_client_logoff(box->cl, NULL);
	} else {
		return (false);
	}

	return (true);
}
