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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fans_impl.h"
#include "fans_req.h"
#include "fans_pos_rep.h"
#include "fans_req_alt.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

#define	VVI_CLB_DES_THRESH	300	/* fpm */
#define	SAME_ALT_THRESH		100	/* ft */

static void
verify_pos_rep(fans_t *box)
{
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
	cpdlc_pos_rep_t rep = CPDLC_NULL_POS_REP;
	float vvi;
	int seg;

	CPDLC_ASSERT(box != NULL);

	rep.rpt_wpt_alt = CPDLC_NULL_ALT;

	if (box->pos_rep.cur_pos.set) {
		rep.cur_pos = box->pos_rep.cur_pos;
	} else {
		CPDLC_ASSERT(box->pos_rep.cur_pos_auto.set);
		rep.cur_pos = box->pos_rep.cur_pos_auto;
	}
	if (!CPDLC_IS_NULL_ALT(box->pos_rep.alt)) {
		rep.cur_alt = box->pos_rep.alt;
	} else {
		CPDLC_ASSERT(box->pos_rep.alt_auto.alt != 0);
		rep.cur_alt = box->pos_rep.alt_auto;
	}
	rep.time_cur_pos = fms_time2cpdlc_time(box->pos_rep.pos_time.set ?
	    box->pos_rep.pos_time : box->pos_rep.pos_time_auto);
	fans_get_cur_spd(box, &rep.spd);
#if 0
	if (box->pos_rep.clb_des.alt.alt != 0)
		alt = box->pos_rep.clb_des;
	else
		alt = box->pos_rep.clb_des_auto;
	if (alt.alt.alt != 0) {
		float vvi = round(fans_get_cur_vvi(box) / 100) * 100;
		const char *op;

		if (vvi <= VVI_CLB_DES_THRESH && alt.alt.alt > cur_alt.alt.alt)
			op = "CRZ CLB";
		else if (alt.alt.alt > cur_alt.alt.alt)
			op = "CLB";
		else
			op = "DES";
		fans_print_alt(&alt, alt_buf, sizeof (alt_buf), true);
		APPEND_SNPRINTF(buf, l, " %s %s", op, alt_buf);
	}

	APPEND_SNPRINTF(buf, l, " %s", spd_buf);

	if (fabs(box->pos_rep.off.nm) >= 0.1) {
		APPEND_SNPRINTF(buf, l, " OFFSET %s%.1f",
		    box->pos_rep.off.dir == CPDLC_DIR_LEFT ? "L" : "R",
		    box->pos_rep.off.nm);
	} else if (fabs(box->pos_rep.off_auto.nm) >= 0.1) {
		APPEND_SNPRINTF(buf, l, " OFFSET %s%.1f",
		    box->pos_rep.off_auto.dir == CPDLC_DIR_LEFT ? "L" : "R",
		    box->pos_rep.off_auto.nm);
	}
#endif
	vvi = fans_get_cur_vvi(box);
	if (!isnan(vvi)) {
		rep.vvi_set = true;
		rep.vvi = round(vvi / 100) * 100;
	}
	rep.rpt_wpt_pos = (box->pos_rep.rpt_wpt.set ?
	    box->pos_rep.rpt_wpt : box->pos_rep.rpt_wpt_auto);
	rep.rpt_wpt_time = fms_time2cpdlc_time(box->pos_rep.wpt_time.set ?
	    box->pos_rep.wpt_time : box->pos_rep.wpt_time_auto);
	rep.rpt_wpt_alt = (!CPDLC_IS_NULL_ALT(box->pos_rep.wpt_alt) ?
	    box->pos_rep.wpt_alt : box->pos_rep.wpt_alt_auto);
#if 0
		if (alt.alt.alt != 0) {
			fans_print_alt(&alt, alt_buf, sizeof (alt_buf), true);
			APPEND_SNPRINTF(buf, l, " %s", alt_buf);
		}
		if (box->pos_rep.wpt_spd.spd.spd != 0)
			spd = box->pos_rep.wpt_spd;
		else
			spd = box->pos_rep.wpt_spd_auto;
		if (spd.spd.spd != 0) {
			fans_print_spd(&spd, spd_buf, sizeof (spd_buf), false,
			    true);
			APPEND_SNPRINTF(buf, l, " %s", spd_buf);
		}
	}
#endif
	rep.fix_next = (box->pos_rep.nxt_fix.set ? box->pos_rep.nxt_fix :
	    box->pos_rep.nxt_fix_auto);
	rep.time_fix_next = fms_time2cpdlc_time(box->pos_rep.nxt_fix_time.set ?
	    box->pos_rep.nxt_fix_time : box->pos_rep.nxt_fix_time_auto);
	rep.fix_next_p1 = (box->pos_rep.nxt_fix1.set ? box->pos_rep.nxt_fix1 :
	    box->pos_rep.nxt_fix1_auto);
	rep.time_dest = fms_time2cpdlc_time(box->pos_rep.time_at_dest.set ?
	    box->pos_rep.time_at_dest : box->pos_rep.time_at_dest_auto);
	rep.wind = fms_wind2cpdlc_wind(box->pos_rep.winds_aloft.set ?
	    box->pos_rep.winds_aloft : box->pos_rep.winds_aloft_auto);
	rep.temp = (box->pos_rep.temp.set ? box->pos_rep.temp.temp :
	    box->pos_rep.temp_auto.temp);

	seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM48_POS_REPORT_posreport, 0);
	cpdlc_msg_seg_set_arg(msg, seg, 0, &rep, NULL);
	fans_req_add_common(box, msg, NULL);

	fans_verify_msg(box, msg, "POS REP", FMS_PAGE_POS_REP, true);
}

static void
set_rpt_wpt(fans_t *box, const cpdlc_pos_t *pos)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(pos != NULL);
	box->pos_rep.rpt_wpt = *pos;
}

static void
set_nxt_fix(fans_t *box, const cpdlc_pos_t *pos)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(pos != NULL);
	box->pos_rep.nxt_fix = *pos;
}

static void
set_nxt_fix1(fans_t *box, const cpdlc_pos_t *pos)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(pos != NULL);
	box->pos_rep.nxt_fix1 = *pos;
}

static void
set_cur_pos(fans_t *box, const cpdlc_pos_t *pos)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(pos != NULL);
	box->pos_rep.cur_pos = *pos;
}

static bool
can_verify_pos_rep(fans_t *box)
{
	cpdlc_spd_t spd;

	CPDLC_ASSERT(box != NULL);
	return ((box->pos_rep.cur_pos.set || box->pos_rep.cur_pos_auto.set) &&
	    (!CPDLC_IS_NULL_ALT(box->pos_rep.alt) ||
	    !CPDLC_IS_NULL_ALT(box->pos_rep.alt_auto)) &&
	    (box->pos_rep.pos_time.set || box->pos_rep.pos_time_auto.set) &&
	    fans_get_cur_spd(box, &spd));
}

static void
draw_page1(fans_t *box)
{
	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "RPT WPT");
	fans_put_pos(box, LSK1_ROW, 0, false, &box->pos_rep.rpt_wpt,
	    &box->pos_rep.rpt_wpt_auto, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R1, "WPT TIME");
	fans_put_time(box, LSK1_ROW, 0, true, &box->pos_rep.wpt_time,
	    &box->pos_rep.wpt_time_auto, false, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "WPT ALT");
	fans_put_alt(box, LSK2_ROW, 0, false, &box->pos_rep.wpt_alt,
	    &box->pos_rep.wpt_alt_auto, true, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_R2, "SPEED");
	fans_put_spd(box, LSK2_ROW, 0, true, &box->pos_rep.wpt_spd,
	    &box->pos_rep.wpt_spd_auto, true, true, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "NXT FIX");
	fans_put_pos(box, LSK3_ROW, 0, false, &box->pos_rep.nxt_fix,
	    &box->pos_rep.nxt_fix_auto, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_L4, "NXT FIX+1");
	fans_put_pos(box, LSK4_ROW, 0, false, &box->pos_rep.nxt_fix1,
	    &box->pos_rep.nxt_fix1_auto, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R3, "NXT FIX TIME");
	fans_put_time(box, LSK3_ROW, 0, true, &box->pos_rep.nxt_fix_time,
	    &box->pos_rep.nxt_fix_time_auto, false, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R4, "TEMP");
	fans_put_temp(box, LSK4_ROW, 0, true, &box->pos_rep.temp,
	    &box->pos_rep.temp_auto, false);
}

static void
draw_page2(fans_t *box)
{
	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "WINDS ALOFT");
	fans_put_wind(box, LSK1_ROW, 0, false, &box->pos_rep.winds_aloft,
	    &box->pos_rep.winds_aloft_auto, false);
	fans_put_str(box, LSK1_ROW, 7, false, FMS_COLOR_GREEN,
	    FMS_FONT_SMALL, "T/KT");

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "CURR POS");
	fans_put_pos(box, LSK2_ROW, 0, false, &box->pos_rep.cur_pos,
	    &box->pos_rep.cur_pos_auto, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "ALT");
	fans_put_alt(box, LSK3_ROW, 0, false, &box->pos_rep.alt,
	    &box->pos_rep.alt_auto, true, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_L4, "CLB/DES");
	fans_put_alt(box, LSK4_ROW, 0, false, &box->pos_rep.clb_des,
	    &box->pos_rep.clb_des_auto, false, true);

	fans_put_lsk_title(box, FMS_KEY_LSK_R2, "POS TIME");
	fans_put_time(box, LSK2_ROW, 0, true, &box->pos_rep.pos_time,
	    &box->pos_rep.pos_time_auto, true, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R3, "TIME AT DEST");
	fans_put_time(box, LSK3_ROW, 0, true, &box->pos_rep.time_at_dest,
	    &box->pos_rep.time_at_dest_auto, false, false);

	fans_put_lsk_title(box, FMS_KEY_LSK_R4, "OFFSET");
	fans_put_off(box, LSK4_ROW, 0, true, &box->pos_rep.off,
	    &box->pos_rep.off_auto, false);
}

void
fans_pos_rep_init_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	/* The POS REP page can send freetext as well */
	memset(&box->req_common, 0, sizeof (box->req_common));
	fans_pos_rep_reset(box);
}

static bool
box_is_clb_or_des(const fans_t *box, int dir)
{
	float vvi;

	CPDLC_ASSERT(box != NULL);
	vvi = round(fans_get_cur_vvi(box) / 100) * 100;
	return ((!isnan(vvi) && vvi >= dir * VVI_CLB_DES_THRESH) ||
	    !fans_get_alt_hold(box));
}

static void
update_auto_data(fans_t *box)
{
	fms_wpt_info_t wptinfo;
	time_t now = time(NULL);
	struct tm t = *gmtime(&now);
	int cur_alt, sel_alt;
	bool sel_alt_fl;
	float dest_dist_NM, off_NM;
	unsigned dest_time_sec;

	CPDLC_ASSERT(box != NULL);

	(void)fans_get_prev_wpt(box, &wptinfo);
	fans_wptinfo2pos(&wptinfo, &box->pos_rep.rpt_wpt_auto);
	fans_wptinfo2time(&wptinfo, &box->pos_rep.wpt_time_auto);
	fans_wptinfo2alt(&wptinfo, &box->pos_rep.wpt_alt_auto);
	fans_wptinfo2spd(&wptinfo, &box->pos_rep.wpt_spd_auto);
	if (CPDLC_IS_NULL_SPD(box->pos_rep.wpt_spd_auto))
		fans_get_cur_spd(box, &box->pos_rep.wpt_spd_auto);

	(void)fans_get_next_wpt(box, &wptinfo);
	fans_wptinfo2pos(&wptinfo, &box->pos_rep.nxt_fix_auto);
	fans_wptinfo2time(&wptinfo, &box->pos_rep.nxt_fix_time_auto);

	(void)fans_get_next_next_wpt(box, &wptinfo);
	fans_wptinfo2pos(&wptinfo, &box->pos_rep.nxt_fix1_auto);

	if (fans_get_dest_info(box, &wptinfo, &dest_dist_NM, &dest_time_sec)) {
		time_t then = now + dest_time_sec;
		struct tm *tm = gmtime(&then);

		if (tm != NULL) {
			box->pos_rep.time_at_dest_auto.set = true;
			box->pos_rep.time_at_dest_auto.hrs = tm->tm_hour;
			box->pos_rep.time_at_dest_auto.mins = tm->tm_min;
		} else {
			box->pos_rep.time_at_dest_auto.set = false;
		}
	} else {
		box->pos_rep.time_at_dest_auto.set = false;
	}
	fans_get_sat(box, &box->pos_rep.temp_auto);
	fans_get_wind(box, &box->pos_rep.winds_aloft_auto);

	if (fans_get_cur_pos(box, &box->pos_rep.cur_pos_auto.lat_lon.lat,
	    &box->pos_rep.cur_pos_auto.lat_lon.lon)) {
		box->pos_rep.cur_pos_auto.set = true;
		box->pos_rep.cur_pos_auto.type = CPDLC_POS_LAT_LON;
	} else {
		box->pos_rep.cur_pos_auto = CPDLC_NULL_POS;
	}
	if (fans_get_cur_alt(box, &cur_alt, &box->pos_rep.alt_auto.fl))
		box->pos_rep.alt_auto.alt = cur_alt;
	else
		box->pos_rep.alt_auto = CPDLC_NULL_ALT;
	box->pos_rep.pos_time_auto.set = true;
	box->pos_rep.pos_time_auto.hrs = t.tm_hour;
	box->pos_rep.pos_time_auto.mins = t.tm_min;

	fans_get_sel_alt(box, &sel_alt, &sel_alt_fl);
	if (fans_is_valid_alt(sel_alt) &&
	    fans_is_valid_alt(cur_alt) && ((box_is_clb_or_des(box, 1) &&
	    sel_alt > cur_alt + SAME_ALT_THRESH) ||
	    (box_is_clb_or_des(box, -1) &&
	    sel_alt < cur_alt - SAME_ALT_THRESH))) {
		box->pos_rep.clb_des_auto.fl = sel_alt_fl;
		box->pos_rep.clb_des_auto.alt = sel_alt;
	} else {
		box->pos_rep.clb_des_auto = CPDLC_NULL_ALT;
	}

	off_NM = fans_get_offset(box);
	if (!isnan(off_NM) && round(off_NM * 10) / 10 != 0) {
		box->pos_rep.off_auto.dir = (off_NM > 0 ? CPDLC_DIR_RIGHT :
		    CPDLC_DIR_LEFT);
		box->pos_rep.off_auto.nm = fabs(round(off_NM * 10) / 10);
	} else {
		memset(&box->pos_rep.off_auto, 0,
		    sizeof (box->pos_rep.off_auto));
	}
}

void
fans_pos_rep_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	update_auto_data(box);

	fans_set_num_subpages(box, 3);

	fans_put_page_title(box, "FANS  POS REPORT");
	fans_put_page_ind(box);

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
	bool read_back;

	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		if (fans_scratchpad_xfer_pos(box, &box->pos_rep.rpt_wpt,
		    FMS_PAGE_POS_REP, set_rpt_wpt, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L2) {
		if (fans_scratchpad_xfer_alt(box, &box->pos_rep.wpt_alt,
		    &box->pos_rep.wpt_alt_auto, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L3) {
		if (fans_scratchpad_xfer_pos(box, &box->pos_rep.nxt_fix,
		    FMS_PAGE_POS_REP, set_nxt_fix, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L4) {
		if (fans_scratchpad_xfer_pos(box, &box->pos_rep.nxt_fix1,
		    FMS_PAGE_POS_REP, set_nxt_fix1, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R1) {
		if (fans_scratchpad_xfer_time(box, &box->pos_rep.wpt_time,
		    &box->pos_rep.wpt_time_auto, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R2) {
		if (fans_scratchpad_xfer_spd(box, &box->pos_rep.wpt_spd,
		    &box->pos_rep.wpt_spd_auto, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R3) {
		if (fans_scratchpad_xfer_time(box, &box->pos_rep.nxt_fix_time,
		    &box->pos_rep.nxt_fix_time_auto, &read_back) &&
		    !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R4) {
		if (fans_scratchpad_xfer_temp(box, &box->pos_rep.temp,
		    &box->pos_rep.temp_auto, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L1) {
		if (fans_scratchpad_xfer_wind(box, &box->pos_rep.winds_aloft,
		    &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L2) {
		if (fans_scratchpad_xfer_pos(box, &box->pos_rep.cur_pos,
		    FMS_PAGE_POS_REP, set_cur_pos, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L3) {
		if (fans_scratchpad_xfer_alt(box, &box->pos_rep.alt,
		    &box->pos_rep.alt_auto, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_L4) {
		if (fans_scratchpad_xfer_alt(box, &box->pos_rep.clb_des,
		    &box->pos_rep.clb_des_auto, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R2) {
		if (fans_scratchpad_xfer_time(box, &box->pos_rep.pos_time,
		    &box->pos_rep.pos_time_auto, &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R3) {
		if (fans_scratchpad_xfer_time(box, &box->pos_rep.time_at_dest,
		    &box->pos_rep.time_at_dest_auto, &read_back) &&
		    !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 1 && key == FMS_KEY_LSK_R4) {
		if (fans_scratchpad_xfer_offset(box, &box->pos_rep.off, NULL,
		    &read_back) && !read_back) {
			fans_scratchpad_clear(box);
		}
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

void
fans_pos_rep_reset(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	memset(&box->pos_rep, 0, sizeof (box->pos_rep));

	box->pos_rep.rpt_wpt = CPDLC_NULL_POS;
	box->pos_rep.wpt_alt = CPDLC_NULL_ALT;
	box->pos_rep.wpt_spd = CPDLC_NULL_SPD;
	box->pos_rep.nxt_fix = CPDLC_NULL_POS;
	box->pos_rep.nxt_fix1 = CPDLC_NULL_POS;
	box->pos_rep.cur_pos = CPDLC_NULL_POS;
	box->pos_rep.alt = CPDLC_NULL_ALT;
	box->pos_rep.clb_des = CPDLC_NULL_ALT;
}
