/*
 * Copyright 2020 Saso Kiselkov
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

#include "../src/cpdlc_alloc.h"
#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_string.h"

#include "fans_reports.h"
#include "fans_parsing.h"
#include "fans_scratchpad.h"

#define	REPORTS_PER_PAGE	5
#define	SAME_ALT_THRESH		100	/* feet */
#define	VVI_CLB_DES_THRESH	500	/* feet/min */
#define	VVI_LVL_THRESH		300	/* feet/min */
#define	IS_LEVEL(vvi)		(fabs(vvi) <= VVI_LVL_THRESH)
#define	IS_CLIMBING(vvi)	((vvi) >= VVI_CLB_DES_THRESH)
#define	IS_DESCENDING(vvi)	((vvi) <= -VVI_CLB_DES_THRESH)
#define	OFF_ROUTE_THRESH	1.0	/* NM */
#define	BACK_ON_ROUTE_THRESH	0.5	/* NM */

static float
alt_get(const cpdlc_msg_seg_t *seg, unsigned arg_nr)
{
	CPDLC_ASSERT(seg != NULL);
	CPDLC_ASSERT(arg_nr < seg->info->num_args);
	if (seg->args[arg_nr].alt.met)
		return (seg->args[arg_nr].alt.alt * 3.2808398950131);
	else
		return (seg->args[arg_nr].alt.alt);
}

static void
report_add_common(const fans_report_t *report, cpdlc_msg_t *msg)
{
	char *buf;
	int l;

	CPDLC_ASSERT(report != NULL);
	CPDLC_ASSERT(msg != NULL);

	l = strlen(report->remarks[0]) + strlen(report->remarks[1]) +
	    strlen(report->remarks[2]);
	buf = safe_calloc(1, l + 1);
	for (int i = 0; i < 3; i++) {
		if (report->remarks[i][0] != '\0') {
			cpdlc_strlcpy(&buf[strlen(buf)], report->remarks[i],
			    l + 1);
		}
	}
	if (buf[0] != '\0') {
		int seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM67_FREETEXT_NORMAL_text, 0);
		cpdlc_msg_seg_set_arg(msg, seg_nr, 0, buf, NULL);
	}
	free(buf);
}

static void
send_report(const fans_t *box, const fans_report_t *report)
{
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);
	int seg_nr;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(report != NULL);

	switch (report->seg->info->msg_type) {
	case CPDLC_UM127_REPORT_BACK_ON_ROUTE:
		cpdlc_msg_add_seg(msg, true, CPDLC_DM41_BACK_ON_ROUTE, 0);
		break;
	case CPDLC_UM128_REPORT_LEAVING_alt:
		seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM28_LEAVING_alt, 0);
		msg->segs[seg_nr].args[0] = report->seg->args[0];
		break;
	case CPDLC_UM129_REPORT_LEVEL_alt:
		seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM37_LEVEL_alt, 0);
		msg->segs[seg_nr].args[0] = report->seg->args[0];
		break;
	case CPDLC_UM130_REPORT_PASSING_pos:
		seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM31_PASSING_pos, 0);
		msg->segs[seg_nr].args[0] = report->seg->args[0];
		break;
	case CPDLC_UM175_REPORT_REACHING_alt:
		seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM72_REACHING_alt, 0);
		msg->segs[seg_nr].args[0] = report->seg->args[0];
		break;
	case CPDLC_UM180_REPORT_REACHING_BLOCK_alt_TO_alt:
		seg_nr = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM76_REACHING_BLOCK_alt_TO_alt, 0);
		msg->segs[seg_nr].args[0] = report->seg->args[0];
		msg->segs[seg_nr].args[1] = report->seg->args[1];
		break;
	default:
		CPDLC_VERIFY(0);
	}
	report_add_common(report, msg);
	cpdlc_msglist_send(box->msglist, msg, CPDLC_NO_MSG_THR_ID);
}

static void
put_report_info(fans_t *box, const fans_report_t *report, int row, bool is_list)
{
	const cpdlc_arg_t *arg1, *arg2;
	char arg1_str[16], arg2_str[16];
	int data_off = (is_list ? 1 : 0);
	int data_color;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(report != NULL);
	CPDLC_ASSERT(report->seg != NULL);
	CPDLC_ASSERT(report->seg->info != NULL);
	CPDLC_ASSERT(!report->seg->info->is_dl);
	arg1 = &report->seg->args[0];
	arg2 = &report->seg->args[1];

	if (is_list) {
		fans_put_str(box, row, 0, false, FMS_COLOR_CYAN,
		    FMS_FONT_SMALL, "<");
	}
	data_color = (report->armed ? FMS_COLOR_GREEN : FMS_COLOR_WHITE);

	switch (report->seg->info->msg_type) {
	case CPDLC_UM127_REPORT_BACK_ON_ROUTE:
		fans_put_str(box, row, data_off, false, FMS_COLOR_CYAN,
		    FMS_FONT_SMALL, "BACK ON ROUTE");
		break;
	case CPDLC_UM128_REPORT_LEAVING_alt:
		fans_print_alt(arg1, arg1_str, sizeof (arg1_str), false);
		fans_put_str(box, LSK_HEADER_ROW(row), 0, false,
		    FMS_COLOR_CYAN, FMS_FONT_SMALL, "LEAVING ALT");
		fans_put_str(box, row, data_off, false, data_color,
		    FMS_FONT_SMALL, "%s", arg1_str);
		break;
	case CPDLC_UM129_REPORT_LEVEL_alt:
		fans_print_alt(arg1, arg1_str, sizeof (arg1_str), false);
		fans_put_str(box, LSK_HEADER_ROW(row), 0, false,
		    FMS_COLOR_CYAN, FMS_FONT_SMALL, "LEVEL ALTITUDE");
		fans_put_str(box, row, data_off, false, data_color,
		    FMS_FONT_SMALL, "%s", arg1_str);
		break;
	case CPDLC_UM130_REPORT_PASSING_pos:
		fans_put_str(box, LSK_HEADER_ROW(row), 0, false,
		    FMS_COLOR_CYAN, FMS_FONT_SMALL, "PASSING POSITION");
		fans_put_str(box, row, data_off, false, data_color,
		    FMS_FONT_SMALL, "%s", arg1->pos);
		break;
	case CPDLC_UM175_REPORT_REACHING_alt:
		fans_print_alt(arg1, arg1_str, sizeof (arg1_str), false);
		fans_put_str(box, LSK_HEADER_ROW(row), 0, false,
		    FMS_COLOR_CYAN, FMS_FONT_SMALL, "REACHING ALTITUDE");
		fans_put_str(box, row, data_off, false, data_color,
		    FMS_FONT_SMALL, "%s", arg1_str);
		break;
	case CPDLC_UM180_REPORT_REACHING_BLOCK_alt_TO_alt:
		fans_print_alt(arg1, arg1_str, sizeof (arg1_str), false);
		fans_print_alt(arg2, arg2_str, sizeof (arg2_str), false);
		fans_put_str(box, LSK_HEADER_ROW(row), 0, false,
		    FMS_COLOR_CYAN, FMS_FONT_SMALL, "BLOCK ALTITUDE");
		fans_put_str(box, row, data_off, false, data_color,
		    FMS_FONT_SMALL, "%s - %s", arg1_str, arg2_str);
		break;
	default:
		CPDLC_VERIFY(0);
	}
	fans_put_str(box, LSK_HEADER_ROW(row), 0, true, data_color,
	    FMS_FONT_SMALL, report->armed ? "ARMED" : "DISARMED");
	if (!is_list) {
		fans_put_str(box, row, 0, true, FMS_COLOR_CYAN, FMS_FONT_LARGE,
		    report->armed ? "DISARM*" : "ARM*");
	}
}

void
fans_reports_due_draw_cb(fans_t *box)
{
	const fans_report_t *report = NULL;
	unsigned n;

	CPDLC_ASSERT(box != NULL);

	n = ceil(list_count(&box->reports_due) / (double)REPORTS_PER_PAGE);
	fans_set_num_subpages(box, MAX(n, 1));

	fans_put_page_title(box, "FANS  REPORTS DUE");
	fans_put_page_ind(box, FMS_COLOR_WHITE);

	if (list_count(&box->reports_due) != 0) {
		report = list_get_i(&box->reports_due,
		    fans_get_subpage(box) * REPORTS_PER_PAGE);
	}
	for (int i = 0; report != NULL && i < REPORTS_PER_PAGE; i++) {
		put_report_info(box, report, LSKi_ROW(i), true);
		report = list_next(&box->reports_due, report);
	}
}

bool
fans_reports_due_key_cb(fans_t *box, fms_key_t key)
{
	if (key >= FMS_KEY_LSK_L1 && key <= FMS_KEY_LSK_L5) {
		unsigned nr = fans_get_subpage(box) * REPORTS_PER_PAGE +
		    (key - FMS_KEY_LSK_L1);

		if (nr >= list_count(&box->reports_due))
			return (false);
		box->report = list_get_i(&box->reports_due, nr);
		fans_set_page(box, FMS_PAGE_REPORT, true);
	} else {
		return (false);
	}

	return (true);
}

void
fans_report_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	if (box->report == NULL) {
		fans_set_page(box, FMS_PAGE_REPORTS_DUE, false);
		return;
	}
	fans_put_page_title(box, "FANS  REPORT");

	put_report_info(box, box->report, LSK1_ROW, false);
	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "  REMARKS");
	for (int i = 0; i < 3; i++) {
		if (box->report->remarks[i][0] != '\0') {
			fans_put_str(box, LSKi_ROW(i + 1), 0, false,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE, "%s",
			    box->report->remarks[i]);
		} else {
			fans_put_str(box, LSKi_ROW(i + 1), 0, false,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE,
			    "------------------------");
		}
	}
	if (!box->report->armed) {
		fans_put_str(box, LSK5_ROW, 0, true, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, "SEND*");
	}
}

bool
fans_report_key_cb(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);
	if (box->report == NULL)
		return (false);

	if (key == FMS_KEY_LSK_L6) {
		fans_set_page(box, FMS_PAGE_REPORTS_DUE, true);
	} else if (key >= FMS_KEY_LSK_L2 && key <= FMS_KEY_LSK_L4) {
		int line = key - FMS_KEY_LSK_L2;
		fans_scratchpad_xfer(box, box->report->remarks[line],
		    sizeof (box->report->remarks[line]), true);
	} else if (key == FMS_KEY_LSK_R1) {
		box->report->armed = !box->report->armed;
	} else if (key == FMS_KEY_LSK_R5 && !box->report->armed) {
		send_report(box, box->report);
		list_remove(&box->reports_due, box->report);
		free(box->report);
		box->report = NULL;
	} else {
		return (false);
	}

	return (true);
}

static void
report_generate_for_seg(fans_t *box, const cpdlc_msg_t *msg,
    const cpdlc_msg_seg_t *seg)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(seg->info != NULL);
	CPDLC_ASSERT(!seg->info->is_dl);

	if (seg->info->msg_type == CPDLC_UM127_REPORT_BACK_ON_ROUTE ||
	    seg->info->msg_type == CPDLC_UM128_REPORT_LEAVING_alt ||
	    seg->info->msg_type == CPDLC_UM129_REPORT_LEVEL_alt ||
	    seg->info->msg_type == CPDLC_UM130_REPORT_PASSING_pos ||
	    seg->info->msg_type == CPDLC_UM175_REPORT_REACHING_alt ||
	    seg->info->msg_type ==
	    CPDLC_UM180_REPORT_REACHING_BLOCK_alt_TO_alt) {
		fans_report_t *report = safe_calloc(1, sizeof (*report));

		report->msg = msg;
		report->seg = seg;
		report->armed = true;
		if (seg->info->msg_type == CPDLC_UM128_REPORT_LEAVING_alt ||
		    seg->info->msg_type == CPDLC_UM175_REPORT_REACHING_alt ||
		    seg->info->msg_type ==
		    CPDLC_UM180_REPORT_REACHING_BLOCK_alt_TO_alt) {
			float cur_alt_ft = fans_get_cur_alt(box);
			float lvl_alt_ft = alt_get(seg, 0);
			float d_alt_ft = lvl_alt_ft - cur_alt_ft;

			if (d_alt_ft > SAME_ALT_THRESH)
				report->lvl_type = LVL_FROM_BLW;
			else if (d_alt_ft > -SAME_ALT_THRESH)
				report->lvl_type = LVL_AT;
			else
				report->lvl_type = LVL_FROM_ABV;
		}
		list_insert_tail(&box->reports_due, report);
	}
}

void
fans_reports_generate(fans_t *box, cpdlc_msg_thr_id_t thr_id)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(thr_id != CPDLC_NO_MSG_THR_ID);

	for (unsigned i = 0, n = cpdlc_msglist_get_thr_msg_count(box->msglist,
	    thr_id); i < n; i++) {
		const cpdlc_msg_t *msg;
		bool is_sent;

		cpdlc_msglist_get_thr_msg(box->msglist, thr_id, i,
		    &msg, NULL, NULL, NULL, &is_sent);
		if (is_sent)
			continue;

		for (unsigned j = 0, m = cpdlc_msg_get_num_segs(msg); j < m;
		    j++) {
			report_generate_for_seg(box, msg, &msg->segs[j]);
		}
	}
}

static bool
update_back_on_route(const fans_t *box, fans_report_t *report)
{
	float offset;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(report != NULL);

	offset = fans_get_offset(box);
	if (isnan(offset)) {
		report->armed = false;
	} else if (fabs(offset) >= OFF_ROUTE_THRESH) {
		report->off_route = true;
	} else if (fabs(offset) <= BACK_ON_ROUTE_THRESH && report->off_route) {
		return (true);
	}

	return (false);
}

static bool
update_leaving_alt(const fans_t *box, fans_report_t *report)
{
	float cur_alt_ft, rpt_alt_ft;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(report != NULL);

	cur_alt_ft = fans_get_cur_alt(box);
	rpt_alt_ft = alt_get(report->seg, 0);

	switch (report->lvl_type) {
	case LVL_FROM_BLW:
		return (cur_alt_ft > rpt_alt_ft);
	case LVL_AT:
		return (fabs(cur_alt_ft - rpt_alt_ft) > SAME_ALT_THRESH);
	case LVL_FROM_ABV:
		return (cur_alt_ft < rpt_alt_ft);
	}
	CPDLC_VERIFY(0);
}

static bool
update_level_alt(const fans_t *box, const fans_report_t *report)
{
	float cur_alt_ft, rep_alt_ft, vvi_fpm;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(report != NULL);

	rep_alt_ft = alt_get(report->seg, 0);
	cur_alt_ft = fans_get_cur_alt(box);
	vvi_fpm = fans_get_cur_vvi(box);

	return (fabs(cur_alt_ft - rep_alt_ft) <= SAME_ALT_THRESH &&
	    IS_LEVEL(vvi_fpm));
}

static bool
update_reaching_alt(const fans_t *box, const fans_report_t *report, bool block)
{
	float cur_alt_ft;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(report != NULL);

	cur_alt_ft = fans_get_cur_alt(box);
	if (block) {
		if (report->lvl_type == LVL_FROM_BLW) {
			return (cur_alt_ft >= alt_get(report->seg, 0) -
			    SAME_ALT_THRESH);
		} else {
			return (cur_alt_ft <= alt_get(report->seg, 1) +
			    SAME_ALT_THRESH);
		}
	} else {
		if (report->lvl_type == LVL_FROM_BLW) {
			return (cur_alt_ft >= alt_get(report->seg, 0) -
			    SAME_ALT_THRESH);
		} else {
			return (cur_alt_ft <= alt_get(report->seg, 0) +
			    SAME_ALT_THRESH);
		}
	}
}

static void
report_update(fans_t *box, fans_report_t *report)
{
	bool send = false;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(report != NULL);
	CPDLC_ASSERT(report->seg != NULL);

	if (!report->armed)
		return;

	switch (report->seg->info->msg_type) {
	case CPDLC_UM127_REPORT_BACK_ON_ROUTE:
		send = update_back_on_route(box, report);
		break;
	case CPDLC_UM128_REPORT_LEAVING_alt:
		send = update_leaving_alt(box, report);
		break;
	case CPDLC_UM129_REPORT_LEVEL_alt:
		send = update_level_alt(box, report);
		break;
	case CPDLC_UM130_REPORT_PASSING_pos:
		/* FIXME: we need some reasonable way to do this */
		report->armed = false;
		break;
	case CPDLC_UM175_REPORT_REACHING_alt:
		send = update_reaching_alt(box, report, false);
		break;
	case CPDLC_UM180_REPORT_REACHING_BLOCK_alt_TO_alt:
		send = update_reaching_alt(box, report, true);
		break;
	default:
		CPDLC_VERIFY(0);
	}
	if (send) {
		if (box->report == report)
			box->report = NULL;
		send_report(box, report);
		list_remove(&box->reports_due, report);
		free(report);
	}
}

void
fans_reports_update(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	if (cpdlc_client_get_logon_status(box->cl, NULL) !=
	    CPDLC_LOGON_COMPLETE) {
		fans_report_t *report;

		while ((report = list_remove_head(&box->reports_due)) != NULL)
			free(report);
		box->report = NULL;
		return;
	}
	for (fans_report_t *report = list_head(&box->reports_due),
	    *next_report = NULL; report != NULL; report = next_report) {
		/* report_update might remove this report, so prep for that */
		next_report = list_next(&box->reports_due, report);
		report_update(box, report);
	}
}
