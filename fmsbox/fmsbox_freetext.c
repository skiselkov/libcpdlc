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

#include "fmsbox_impl.h"
#include "fmsbox_scratchpad.h"

static bool
freetext_msg_ready(fmsbox_t *box)
{
	for (int i = 0; i < MAX_FREETEXT_LINES; i++) {
		if (box->freetext[i][0] != '\0')
			return (true);
	}
	return (false);
}

static void
send_freetext_msg(fmsbox_t *box)
{
	char buf[sizeof (box->freetext) + MAX_FREETEXT_LINES] = { 0 };
	cpdlc_msg_t *msg = cpdlc_msg_alloc();

	ASSERT(box != NULL);
	ASSERT(freetext_msg_ready(box));

	for (int i = 0; i < MAX_FREETEXT_LINES; i++) {
		if (box->freetext[i][0] != '\0') {
			if (strlen(buf) != 0)
				strcat(buf, " ");
			strcat(buf, box->freetext[i]);
		}
	}

	cpdlc_msg_add_seg(msg, true, CPDLC_DM67_FREETEXT_NORMAL_text, 0);
	cpdlc_msg_seg_set_arg(msg, 0, 0, buf, NULL);
	cpdlc_msglist_send(box->msglist, msg, CPDLC_NO_MSG_THR_ID);

	memset(box->freetext, 0, sizeof (box->freetext));
}

void
fmsbox_freetext_draw_cb(fmsbox_t *box)
{
	enum { MAX_LINES = 4 };

	ASSERT(box != NULL);

	fmsbox_set_num_subpages(box, 2);

	fmsbox_put_page_title(box, "FANS  FREE TEXT");
	fmsbox_put_page_ind(box, FMS_COLOR_WHITE);

	for (int row = 0; row < MAX_LINES; row++) {
		int line = row + box->subpage * MAX_LINES;
		if (box->freetext[line][0] != '\0') {
			fmsbox_put_str(box, LSKi_ROW(row), 0, false,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE, "%s",
			    box->freetext[line]);
		} else {
			fmsbox_put_str(box, LSKi_ROW(row), 0, false,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE,
			    "------------------------");
		}
	}

	if (freetext_msg_ready(box)) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_CYAN,
		    "*ERASE");
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "SEND*");
	}
}

bool
fmsbox_freetext_key_cb(fmsbox_t *box, fms_key_t key)
{
	enum { MAX_LINES = 4 };

	ASSERT(box != NULL);

	if (key >= FMS_KEY_LSK_L1 && key <= FMS_KEY_LSK_L4) {
		int line = key - FMS_KEY_LSK_L1 + box->subpage * MAX_LINES;
		fmsbox_scratchpad_xfer(box, box->freetext[line],
		    sizeof (box->freetext[line]), true);
	} else if (key == FMS_KEY_LSK_L5) {
		memset(box->freetext, 0, sizeof (box->freetext));
	} else if (key == FMS_KEY_LSK_R5) {
		if (freetext_msg_ready(box)) {
			send_freetext_msg(box);
			fmsbox_set_page(box, FMS_PAGE_MAIN_MENU);
		}
	} else {
		return (false);
	}
	return (true);
}
