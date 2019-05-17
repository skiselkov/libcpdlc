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
#include "fmsbox_req.h"
#include "fmsbox_req_alt.h"
#include "fmsbox_scratchpad.h"
#include "fmsbox_vrfy.h"

static void
set_rpt_wpt(fmsbox_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->pos_rep.rpt_wpt, pos, sizeof (*pos));
}

static void
set_nxt_fix(fmsbox_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->pos_rep.nxt_fix, pos, sizeof (*pos));
}

static void
set_nxt_fix1(fmsbox_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->pos_rep.nxt_fix1, pos, sizeof (*pos));
}

static void
set_cur_pos(fmsbox_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->pos_rep.cur_pos, pos, sizeof (*pos));
}

static bool
can_verify_pos_rep(fmsbox_t *box)
{
	ASSERT(box != NULL);
	return (true);
}

static void
verify_pos_rep(fmsbox_t *box)
{
	UNUSED(box);
}

static void
draw_page1(fmsbox_t *box)
{
	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "RPT WPT");
	fmsbox_put_pos(box, LSK1_ROW, 0, false, &box->pos_rep.rpt_wpt, false);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L2, "WPT ALT");
	fmsbox_put_alt(box, LSK2_ROW, 0, false, &box->pos_rep.wpt_alt,
	    false, true);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L3, "NXT FIX");
	fmsbox_put_pos(box, LSK3_ROW, 0, false, &box->pos_rep.nxt_fix, false);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L4, "NXT FIX+1");
	fmsbox_put_pos(box, LSK4_ROW, 0, false, &box->pos_rep.nxt_fix1, false);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R1, "WPT TIME");
	fmsbox_put_time(box, LSK1_ROW, 0, true, &box->pos_rep.wpt_time,
	    false, false);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R2, "SPEED");
	fmsbox_put_spd(box, LSK2_ROW, 0, true, &box->pos_rep.spd, false, true);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R3, "NXT FIX TIME");
	fmsbox_put_time(box, LSK3_ROW, 0, true, &box->pos_rep.nxt_fix_time,
	    false, false);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R4, "TEMP");
	fmsbox_put_temp(box, LSK4_ROW, 0, true, &box->pos_rep.temp, false);
}

static void
draw_page2(fmsbox_t *box)
{
	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "WINDS ALOFT");

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L2, "CURR POS");
	fmsbox_put_pos(box, LSK2_ROW, 0, false, &box->pos_rep.cur_pos, true);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L3, "ALT");
	fmsbox_put_alt(box, LSK3_ROW, 0, false, &box->pos_rep.alt,
	    true, true);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L4, "CLB/DES");
	fmsbox_put_alt(box, LSK4_ROW, 0, false, &box->pos_rep.clb_des,
	    false, true);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R2, "POS TIME");
	fmsbox_put_time(box, LSK2_ROW, 0, true, &box->pos_rep.pos_time,
	    true, false);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R3, "TIME AT DEST");
	fmsbox_put_time(box, LSK3_ROW, 0, true, &box->pos_rep.time_at_dest,
	    false, false);

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R4, "OFFSET");
	fmsbox_put_off(box, LSK4_ROW, 0, true, &box->pos_rep.off, false);
}

void
fmsbox_pos_rep_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_set_num_subpages(box, 3);

	fmsbox_put_page_title(box, "FANS  POS REPORT");
	fmsbox_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_page1(box);
	else if (box->subpage == 1)
		draw_page2(box);
	else
		fmsbox_req_draw_freetext(box);

	if (can_verify_pos_rep(box)) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fmsbox_pos_rep_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		fmsbox_scratchpad_xfer_pos(box, &box->pos_rep.rpt_wpt,
		    FMS_PAGE_POS_REP, set_rpt_wpt);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L2) {
		fmsbox_scratchpad_xfer_alt(box, &box->pos_rep.wpt_alt);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L3) {
		fmsbox_scratchpad_xfer_pos(box, &box->pos_rep.nxt_fix,
		    FMS_PAGE_POS_REP, set_nxt_fix);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		fmsbox_scratchpad_xfer_pos(box, &box->pos_rep.nxt_fix1,
		    FMS_PAGE_POS_REP, set_nxt_fix1);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R1) {
		fmsbox_scratchpad_xfer_time(box, &box->pos_rep.wpt_time);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R2) {
		fmsbox_scratchpad_xfer_spd(box, &box->pos_rep.spd);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R3) {
		fmsbox_scratchpad_xfer_time(box, &box->pos_rep.nxt_fix_time);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		fmsbox_scratchpad_xfer_temp(box, &box->pos_rep.temp);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L1) {
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L2) {
		fmsbox_scratchpad_xfer_pos(box, &box->pos_rep.cur_pos,
		    FMS_PAGE_POS_REP, set_cur_pos);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L3) {
		fmsbox_scratchpad_xfer_alt(box, &box->pos_rep.alt);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L4) {
		fmsbox_scratchpad_xfer_alt(box, &box->pos_rep.clb_des);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R2) {
		fmsbox_scratchpad_xfer_time(box, &box->pos_rep.pos_time);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R3) {
		fmsbox_scratchpad_xfer_time(box, &box->pos_rep.time_at_dest);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R4) {
		fmsbox_scratchpad_xfer_offset(box, &box->pos_rep.off);
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_pos_rep(box))
			verify_pos_rep(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fmsbox_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
