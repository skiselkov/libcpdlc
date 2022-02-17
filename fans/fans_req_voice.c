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

#include <string.h>
#include <stdio.h>

#include "../src/cpdlc_assert.h"

#include "fans_req.h"
#include "fans_req_voice.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

static void
verify_voice_req(fans_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);

	if (box->voice_req.freq_set) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM21_REQ_VOICE_CTC_ON_freq, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->voice_req.freq, NULL);
	} else {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM20_REQ_VOICE_CTC, 0);
	}
	fans_req_add_common(box, msg);

	fans_verify_msg(box, msg, "VOICE", FMS_PAGE_REQ_VOICE, true);
}

static void
draw_main_page(fans_t *box)
{
	fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_CYAN, FMS_FONT_SMALL,
	    "REQUEST VOICE CONTACT");
	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "FREQUENCY");

	if (box->voice_req.freq_set) {
		char buf[8];
		int l;

		if (box->voice_req.freq <= 21) {
			/* HF */
			l = snprintf(buf, sizeof (buf), "%.04f",
			    box->voice_req.freq);
		} else {
			/* VHF/UHF */
			l = snprintf(buf, sizeof (buf), "%.03f",
			    box->voice_req.freq);
		}
		fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", buf);
		fans_put_str(box, LSK2_ROW, l + 1, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "MHZ");
	} else {
		fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_SMALL, "------------");
	}

	/* Can't use standardized drawing, as these are offset by 1 row */
	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "DUE TO WX");
	fans_put_altn_selector(box, LSK3_ROW, false, box->req_common.due_wx,
	    "NO", "YES", NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L4, "DUE TO A/C");
	fans_put_altn_selector(box, LSK4_ROW, false, box->req_common.due_ac,
	    "NO", "YES", NULL);
}

void
fans_req_voice_init_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	memset(&box->voice_req, 0, sizeof (box->voice_req));
}

void
fans_req_voice_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  VOICE REQ");
	fans_put_page_ind(box);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE, "<VERIFY");
	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_req_voice_key_cb(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L2) {
		char buf[8] = { 0 };
		bool read_back;

		if (box->voice_req.freq_set) {
			if (box->voice_req.freq <= 21) {
				/* HF */
				snprintf(buf, sizeof (buf), "%.04f",
				    box->voice_req.freq);
			} else {
				/* VHF/UHF */
				snprintf(buf, sizeof (buf), "%.03f",
				    box->voice_req.freq);
			}
		}
		if (!fans_scratchpad_xfer(box, buf, sizeof (buf), true,
		    &read_back)) {
			return (true);
		}
		if (buf[0] != '\0') {
			double freq;
			if (sscanf(buf, "%lf", &freq) != 1 || freq < 1 ||
			    (freq >= 500 && freq < 1000) || freq >= 500000 ||
			    (freq >= 21 && freq < 118)) {
				fans_set_error(box, FANS_ERR_INVALID_ENTRY);
			} else {
				if (freq >= 1000)
					freq /= 1000;
				box->voice_req.freq_set = true;
				box->voice_req.freq = freq;
				if (!read_back)
					fans_scratchpad_clear(box);
			}
		} else {
			box->voice_req.freq_set = false;
			if (!read_back)
				fans_scratchpad_clear(box);
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
		fans_set_page(box, FMS_PAGE_REQUESTS, false);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
