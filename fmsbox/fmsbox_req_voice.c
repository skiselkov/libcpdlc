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
#include <stdio.h>

#include "../src/cpdlc_assert.h"

#include "fmsbox_req.h"
#include "fmsbox_req_voice.h"
#include "fmsbox_scratchpad.h"
#include "fmsbox_vrfy.h"

static void
verify_voice_req(fmsbox_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc();

	if (box->voice_req.freq_set) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM21_REQ_VOICE_CTC_ON_freq, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->voice_req.freq, NULL);
	} else {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM20_REQ_VOICE_CTC, 0);
	}
	fmsbox_req_add_common(box, msg);

	fmsbox_verify_msg(box, msg, "VOICE REQ", FMS_PAGE_REQ_VOICE);
}

static void
draw_main_page(fmsbox_t *box)
{
	fmsbox_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_CYAN, FMS_FONT_SMALL,
	    "REQUEST VOICE CONTACT");
	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L2, "FREQUENCY");

	if (box->voice_req.freq_set) {
		char buf[8];
		int l;

		l = snprintf(buf, sizeof (buf), "%.03f", box->voice_req.freq);
		fmsbox_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", buf);
		fmsbox_put_str(box, LSK2_ROW, l + 1, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "MHZ");
	} else {
		fmsbox_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_CYAN,
		    FMS_FONT_SMALL, "------------");
	}

	/* Can't use standardized drawing, as these are offset by 1 row */
	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L3, "DUE TO WX");
	fmsbox_put_altn_selector(box, LSK3_ROW, false, box->req_common.due_wx,
	    "NO", "YES", NULL);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L4, "DUE TO A/C");
	fmsbox_put_altn_selector(box, LSK4_ROW, false, box->req_common.due_ac,
	    "NO", "YES", NULL);
}

void
fmsbox_req_voice_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_set_num_subpages(box, 2);

	fmsbox_put_page_title(box, "FANS  VOICE REQ");
	fmsbox_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fmsbox_req_draw_freetext(box);

	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE, "<VERIFY");
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fmsbox_req_voice_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L2) {
		char buf[8] = { 0 };

		if (box->voice_req.freq_set) {
			snprintf(buf, sizeof (buf), "%.03f",
			    box->voice_req.freq);
		}
		fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
		if (buf[0] != '\0') {
			double freq;
			if (sscanf(buf, "%lf", &freq) != 1 || freq < 1 ||
			    (freq >= 500 && freq < 1000) || freq >= 500000) {
				fmsbox_set_error(box, "FORMAT ERROR");
			} else {
				if (freq >= 1000)
					freq /= 1000;
				box->voice_req.freq_set = true;
				box->voice_req.freq = freq;
			}
		} else {
			box->voice_req.freq_set = false;
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L3) {
		box->req_common.due_wx = !box->req_common.due_wx;
		box->req_common.due_ac = false;
	} else if (key == FMS_KEY_LSK_L4) {
		box->req_common.due_wx = false;
		box->req_common.due_ac = !box->req_common.due_ac;
	} else if (key == FMS_KEY_LSK_L5) {
		verify_voice_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	} else if (KEY_IS_REQ_FREETEXT(box, key)) {
		fmsbox_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
