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

#include "fmsbox_req.h"
#include "fmsbox_req_off.h"
#include "fmsbox_scratchpad.h"
#include "fmsbox_vrfy.h"

static void
print_off(fmsbox_t *box, char *buf, size_t cap)
{
	ASSERT(box != NULL);
	if (box->off_req.nm != 0) {
		snprintf(buf, cap, "%c%.0f",
		    box->off_req.dir == CPDLC_DIR_LEFT ? 'L' : 'R',
		    box->off_req.nm);
	} else if (buf != NULL) {
		buf[0] = '\0';
	}
}

static bool
can_verify_off_req(fmsbox_t *box)
{
	ASSERT(box);
	return (box->off_req.nm != 0 &&
	    fmsbox_step_at_can_send(&box->off_req.step_at));
}

static void
verify_off_req(fmsbox_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg;

	msg = cpdlc_msg_alloc();

	if (box->off_req.step_at.type != STEP_AT_NONE) {
		if (box->off_req.step_at.type == STEP_AT_TIME) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM17_AT_time_REQ_OFFSET_dir_dist_OF_ROUTE, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    &box->off_req.step_at.hrs,
			    &box->off_req.step_at.mins);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM16_AT_pos_REQ_OFFSET_dir_dist_OF_ROUTE, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    box->off_req.step_at.pos, NULL);
		}
		cpdlc_msg_seg_set_arg(msg, seg, 1, &box->off_req.dir, NULL);
		cpdlc_msg_seg_set_arg(msg, seg, 2, &box->off_req.nm, NULL);
	} else {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM15_REQ_OFFSET_dir_dist_OF_ROUTE, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->off_req.dir, NULL);
		cpdlc_msg_seg_set_arg(msg, seg, 1, &box->off_req.nm, NULL);
	}
	fmsbox_req_add_common(box, msg);

	fmsbox_verify_msg(box, msg, "OFF REQ", FMS_PAGE_REQ_OFF);
}

static void
draw_main_page(fmsbox_t *box)
{
	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "OFFSET");
	if (box->off_req.nm == 0) {
		fmsbox_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "____");
	} else {
		char buf[8];
		print_off(box, buf, sizeof (buf));
		fmsbox_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", buf);
	}

	fmsbox_req_draw_due(box, true);

	fmsbox_put_step_at(box, &box->off_req.step_at);
}

void
fmsbox_req_off_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_set_num_subpages(box, 2);

	fmsbox_put_page_title(box, "FANS  OFFSET REQ");
	fmsbox_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fmsbox_req_draw_freetext(box);

	if (can_verify_off_req(box)) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}

	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

static bool
parse_dir(char *buf, cpdlc_dir_t *dir)
{
	char first, last, c;

	ASSERT(buf != NULL);
	ASSERT(strlen(buf) != 0);
	ASSERT(dir != NULL);

	first = buf[0];
	last = buf[strlen(buf) - 1];
	if (!isdigit(first)) {
		c = first;
		memmove(&buf[0], &buf[1], strlen(buf));
	} else if (!isdigit(last)) {
		c = last;
	} else {
		return (false);
	}
	if (c == 'L')
		*dir = CPDLC_DIR_LEFT;
	else if (c == 'R')
		*dir = CPDLC_DIR_RIGHT;
	else
		return (false);
	return (true);
}

bool
fmsbox_req_off_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		char buf[8];

		print_off(box, buf, sizeof (buf));
		fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
		if (strlen(buf) != 0) {
			unsigned nm;
			cpdlc_dir_t dir;

			if (strlen(buf) < 2 || !parse_dir(buf, &dir) ||
			    sscanf(buf, "%d", &nm) != 1 ||
			    nm == 0 || nm > 999) {
				fmsbox_set_error(box, "FORMAT ERROR");
			} else if (buf[0] == 'L') {
				box->off_req.dir = dir;
				box->off_req.nm = nm;
			} else {
				box->off_req.dir = dir;
				box->off_req.nm = nm;
			}
		} else {
			box->off_req.nm = 0;
		}
	} else if (box->subpage == 0 &&
	    (key >= FMS_KEY_LSK_L2 && key <= FMS_KEY_LSK_L4)) {
		fmsbox_req_key_due(box, key);
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_off_req(box))
			verify_off_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	} else if (KEY_IS_REQ_STEP_AT(box, key)) {
		fmsbox_key_step_at(box, key, &box->off_req.step_at);
	} else if (KEY_IS_REQ_FREETEXT(box, key)) {
		fmsbox_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
