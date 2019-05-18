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
#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fans_emer.h"
#include "fans_pos_pick.h"
#include "fans_req.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

static void
verify_emer(fans_t *box)
{
	char buf[512];
	unsigned l = 0;
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc();

	if (box->emer.pan)
		APPEND_SNPRINTF(buf, l, "PAN PAN PAN.");
	else
		APPEND_SNPRINTF(buf, l, "MAYDAY MAYDAY MAYDAY.");
	switch (box->emer.reason) {
	case EMER_REASON_NONE:
		break;
	case EMER_REASON_WX:
		APPEND_SNPRINTF(buf, l, " %sDUE TO WX.",
		    box->emer.pan ? "" : "EMERGENCY ");
		break;
	case EMER_REASON_MED:
		APPEND_SNPRINTF(buf, l, " MEDICAL EMERGENCY.");
		break;
	case EMER_REASON_CABIN_PRESS:
		APPEND_SNPRINTF(buf, l, " %sDUE TO CABIN PRESS.",
		    box->emer.pan ? "" : "EMERGENCY ");
		break;
	case EMER_REASON_ENG_LOSS:
		APPEND_SNPRINTF(buf, l, " %sDUE TO ENGINE LOSS.",
		    box->emer.pan ? "" : "EMERGENCY ");
		break;
	case EMER_REASON_LOW_FUEL:
		APPEND_SNPRINTF(buf, l, " %sDUE TO LOW FUEL.",
		    box->emer.pan ? "" : "EMERGENCY ");
		break;
	}
	if (box->emer.des.alt.alt != 0) {
		char altbuf[128];
		fans_print_alt(&box->emer.des, altbuf, sizeof (altbuf));
		APPEND_SNPRINTF(buf, l, " DESCENDING TO %s%s.", altbuf,
		    box->emer.des.alt.fl ? "" : " FT");
	}
	if (box->emer.off.nm != 0) {
		APPEND_SNPRINTF(buf, l, " OFFSETTING %.0f NM %c OF ROUTE.",
		    box->emer.off.nm,
		    box->emer.off.dir == CPDLC_DIR_LEFT ? 'L' : 'R');
	}
	if (box->emer.fuel.set) {
		if (box->emer.fuel.hrs != 0) {
			APPEND_SNPRINTF(buf, l, " %d HRS", box->emer.fuel.hrs);
		}
		/* Handles the 00:00 fuel figure */
		if (box->emer.fuel.mins != 0 || box->emer.fuel.hrs == 0) {
			APPEND_SNPRINTF(buf, l, " %d MINS",
			    box->emer.fuel.mins);
		}
		APPEND_SNPRINTF(buf, l, " OF FUEL REMAINING");

		if (box->emer.souls_set)
			APPEND_SNPRINTF(buf, l, " AND");
		else
			APPEND_SNPRINTF(buf, l, ".");
	}
	if (box->emer.souls_set)
		APPEND_SNPRINTF(buf, l, " %d SOULS ON BOARD.", box->emer.souls);
	if (box->emer.divert.set) {
		char posbuf[16];

		fans_print_pos(&box->emer.divert, posbuf, sizeof (posbuf),
		    POS_PRINT_NORM);
		APPEND_SNPRINTF(buf, l, " DIVERTING TO %s VIA DCT.", posbuf);
	}

	seg = cpdlc_msg_add_seg(msg, true,
	    CPDLC_DM68_FREETEXT_DISTRESS_text, 0);
	cpdlc_msg_seg_set_arg(msg, seg, 0, buf, NULL);

	box->req_common.distress = true;
	fans_req_add_common(box, msg);

	fans_verify_msg(box, msg, "EMER MSG", FMS_PAGE_EMER, true);
}

static void
set_divert(fans_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->emer.divert, pos, sizeof (*pos));
}

static void
draw_main_page(fans_t *box)
{
	const char *reason = NULL;

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "EMER TYPE");
	fans_put_altn_selector(box, LSK2_ROW, false, box->emer.pan,
	    "MAYDAY", "PAN", NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "FUEL");
	fans_put_time(box, LSK3_ROW, 0, false, &box->emer.fuel, false, true);
	fans_put_str(box, LSK3_ROW, 6, false, FMS_COLOR_GREEN,
	    FMS_FONT_SMALL, "HH:MM");

	fans_put_lsk_title(box, FMS_KEY_LSK_L4, "SOULS");
	if (box->emer.souls_set) {
		fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%d", box->emer.souls);
	} else {
		fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, "---");
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R2, "DESCEND TO");
	fans_put_alt(box, LSK2_ROW, 0, true, &box->emer.des, false, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_R3, "OFFSET TO");
	fans_put_off(box, LSK3_ROW, 0, true, &box->emer.off, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R4, "DIVERT TO");
	fans_put_str(box, LSK4_ROW, 0, true, FMS_COLOR_WHITE,
	    FMS_FONT_LARGE, "POS>");
	if (box->emer.divert.set) {
		char buf[16];
		fans_print_pos(&box->emer.divert, buf, sizeof (buf),
		    POS_PRINT_COMPACT);
		fans_put_str(box, LSK4_ROW, 5, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "%s", buf);
	} else {
		fans_put_str(box, LSK4_ROW, 5, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "----------");
	}

	switch (box->emer.reason) {
	case EMER_REASON_NONE:
		reason = "NONE";
		break;
	case EMER_REASON_WX:
		reason = "WX";
		break;
	case EMER_REASON_MED:
		reason = "MED EMER";
		break;
	case EMER_REASON_CABIN_PRESS:
		reason = "CABIN PRESS";
		break;
	case EMER_REASON_ENG_LOSS:
		reason = "ENG LOSS";
		break;
	case EMER_REASON_LOW_FUEL:
		reason = "LOW FUEL";
		break;
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R5, "REASON");
	fans_put_str(box, LSK5_ROW, 0, true, FMS_COLOR_GREEN,
	    FMS_FONT_LARGE, "%sv", reason);
}

void
fans_emer_init_cb(fans_t *box)
{
	/* The EMER page can send freetext as well */
	ASSERT(box != NULL);
	memset(&box->emer, 0, sizeof (box->emer));
	memset(&box->req_common, 0, sizeof (box->req_common));
}

void
fans_emer_draw_cb(fans_t *box)
{
	ASSERT(box != NULL);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  EMERGENCY MSG");
	fans_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE, "<VERIFY");
}

bool
fans_emer_key_cb(fans_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L2) {
		box->emer.pan = !box->emer.pan;
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L3) {
		fans_scratchpad_xfer_time(box, &box->emer.fuel);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		fans_scratchpad_xfer_uint(box, &box->emer.souls,
		    &box->emer.souls_set, 1, 999);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R2) {
		fans_scratchpad_xfer_alt(box, &box->emer.des);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R3) {
		fans_scratchpad_xfer_offset(box, &box->emer.off);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		if (fans_scratchpad_is_delete(box)) {
			fans_scratchpad_clear(box);
			memset(&box->emer.divert, 0, sizeof (box->emer.divert));
		} else {
			fans_pos_pick_start(box, set_divert, FMS_PAGE_EMER,
			    &box->emer.divert);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R5) {
		box->emer.reason = (box->emer.reason + 1) %
		    (EMER_REASON_LOW_FUEL + 1);
	} else if (key == FMS_KEY_LSK_L5) {
		verify_emer(box);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
