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

#include "fans_impl.h"
#include "fans_req.h"
#include "fans_req_alt.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

/*
static void
verify_pos_rep(fans_t *box)
{
	int l = 0;
	char buf[1024];

	ASSERT(box != NULL);

	APPEND_SNPRINTF(buf, l, "POSITION REPORT");
}
*/

static void
set_rpt_wpt(fans_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->pos_rep.rpt_wpt, pos, sizeof (*pos));
}

static void
set_nxt_fix(fans_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->pos_rep.nxt_fix, pos, sizeof (*pos));
}

static void
set_nxt_fix1(fans_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->pos_rep.nxt_fix1, pos, sizeof (*pos));
}

static void
set_cur_pos(fans_t *box, const fms_pos_t *pos)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	memcpy(&box->pos_rep.cur_pos, pos, sizeof (*pos));
}

static bool
can_verify_pos_rep(fans_t *box)
{
	ASSERT(box != NULL);
	return (true);
}

static void
verify_pos_rep(fans_t *box)
{
	UNUSED(box);
}

static void
draw_page1(fans_t *box)
{
	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "RPT WPT");
	fans_put_pos(box, LSK1_ROW, 0, false, &box->pos_rep.rpt_wpt, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "WPT ALT");
	fans_put_alt(box, LSK2_ROW, 0, false, &box->pos_rep.wpt_alt,
	    false, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "NXT FIX");
	fans_put_pos(box, LSK3_ROW, 0, false, &box->pos_rep.nxt_fix, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_L4, "NXT FIX+1");
	fans_put_pos(box, LSK4_ROW, 0, false, &box->pos_rep.nxt_fix1, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R1, "WPT TIME");
	fans_put_time(box, LSK1_ROW, 0, true, &box->pos_rep.wpt_time,
	    false, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R2, "SPEED");
	fans_put_spd(box, LSK2_ROW, 0, true, &box->pos_rep.spd, false, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_R3, "NXT FIX TIME");
	fans_put_time(box, LSK3_ROW, 0, true, &box->pos_rep.nxt_fix_time,
	    false, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R4, "TEMP");
	fans_put_temp(box, LSK4_ROW, 0, true, &box->pos_rep.temp, false);
}

static void
draw_page2(fans_t *box)
{
	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "WINDS ALOFT");
	fans_put_wind(box, LSK1_ROW, 0, false, &box->pos_rep.winds_aloft,
	    false);
	fans_put_str(box, LSK1_ROW, 7, false, FMS_COLOR_GREEN,
	    FMS_FONT_SMALL, "T/KT");

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "CURR POS");
	fans_put_pos(box, LSK2_ROW, 0, false, &box->pos_rep.cur_pos, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "ALT");
	fans_put_alt(box, LSK3_ROW, 0, false, &box->pos_rep.alt,
	    true, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_L4, "CLB/DES");
	fans_put_alt(box, LSK4_ROW, 0, false, &box->pos_rep.clb_des,
	    false, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_R2, "POS TIME");
	fans_put_time(box, LSK2_ROW, 0, true, &box->pos_rep.pos_time,
	    true, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R3, "TIME AT DEST");
	fans_put_time(box, LSK3_ROW, 0, true, &box->pos_rep.time_at_dest,
	    false, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R4, "OFFSET");
	fans_put_off(box, LSK4_ROW, 0, true, &box->pos_rep.off, false);
}

void
fans_pos_rep_draw_cb(fans_t *box)
{
	ASSERT(box != NULL);

	fans_set_num_subpages(box, 3);

	fans_put_page_title(box, "FANS  POS REPORT");
	fans_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_page1(box);
	else if (box->subpage == 1)
		draw_page2(box);
	else
		fans_req_draw_freetext(box);

	if (can_verify_pos_rep(box)) {
		fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}
	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_pos_rep_key_cb(fans_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		fans_scratchpad_xfer_pos(box, &box->pos_rep.rpt_wpt,
		    FMS_PAGE_POS_REP, set_rpt_wpt);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L2) {
		fans_scratchpad_xfer_alt(box, &box->pos_rep.wpt_alt);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L3) {
		fans_scratchpad_xfer_pos(box, &box->pos_rep.nxt_fix,
		    FMS_PAGE_POS_REP, set_nxt_fix);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		fans_scratchpad_xfer_pos(box, &box->pos_rep.nxt_fix1,
		    FMS_PAGE_POS_REP, set_nxt_fix1);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R1) {
		fans_scratchpad_xfer_time(box, &box->pos_rep.wpt_time);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R2) {
		fans_scratchpad_xfer_spd(box, &box->pos_rep.spd);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R3) {
		fans_scratchpad_xfer_time(box, &box->pos_rep.nxt_fix_time);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		fans_scratchpad_xfer_temp(box, &box->pos_rep.temp);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L1) {
		fans_scratchpad_xfer_wind(box, &box->pos_rep.winds_aloft);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L2) {
		fans_scratchpad_xfer_pos(box, &box->pos_rep.cur_pos,
		    FMS_PAGE_POS_REP, set_cur_pos);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L3) {
		fans_scratchpad_xfer_alt(box, &box->pos_rep.alt);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L4) {
		fans_scratchpad_xfer_alt(box, &box->pos_rep.clb_des);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R2) {
		fans_scratchpad_xfer_time(box, &box->pos_rep.pos_time);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R3) {
		fans_scratchpad_xfer_time(box, &box->pos_rep.time_at_dest);
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R4) {
		fans_scratchpad_xfer_offset(box, &box->pos_rep.off);
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_pos_rep(box))
			verify_pos_rep(box);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 2)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
