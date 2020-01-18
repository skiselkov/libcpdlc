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

#include <math.h>

#include "../src/cpdlc_assert.h"

#include "fans_impl.h"
#include "fans_rej.h"

static void
draw_thr_hdr(fans_t *box, unsigned row, cpdlc_msg_thr_id_t thr_id)
{
	const cpdlc_msg_t *msg;
	cpdlc_msg_thr_status_t st;
	unsigned hours, mins;
	bool sent, dirty;
	const char *atsu = NULL;

	/* Grab the last received message to figure out who it came from */
	for (int n = cpdlc_msglist_get_thr_msg_count(box->msglist, thr_id),
	    i = n - 1;
	    i >= 0; i--) {
		cpdlc_msglist_get_thr_msg(box->msglist, thr_id, i, &msg, NULL,
		    NULL, NULL, &sent);
		if (!sent) {
			atsu = cpdlc_msg_get_from(msg);
			break;
		}
	}

	cpdlc_msglist_get_thr_msg(box->msglist, thr_id, 0, &msg, NULL,
	    &hours, &mins, &sent);
	st = cpdlc_msglist_get_thr_status(box->msglist, thr_id, &dirty);

	/* Message uplink/downlink, time and sender */
	fans_put_str(box, row, 0, false, FMS_COLOR_GREEN, FMS_FONT_SMALL,
	    "%s %02d%02dZ%s%s", sent ? "v" : "^", hours, mins,
	    atsu != NULL ? "-" : "", atsu != NULL ? atsu : "");
	/* Thread status */
	fans_put_str(box, row, 0, true, FMS_COLOR_GREEN, FMS_FONT_SMALL,
	    "%s", fans_thr_status2str(st, dirty));
}

static void
msg_log_draw_thr(fans_t *box, cpdlc_msg_thr_id_t thr_id, unsigned row)
{
	const cpdlc_msg_t *msg;
	char buf[64];

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT3U(row, <, 5);

	draw_thr_hdr(box, 2 * row + 1, thr_id);

	cpdlc_msglist_get_thr_msg(box->msglist, thr_id, 0, &msg, NULL, NULL,
	    NULL, NULL);
	cpdlc_msg_readable(msg, buf, sizeof (buf));
	fans_put_str(box, 2 * row + 2, 0, false, FMS_COLOR_WHITE,
	    FMS_FONT_LARGE, "<%s", buf);
}

void
fans_msg_log_draw_cb(fans_t *box)
{
	enum { MSG_LOG_LINES = 4 };
	cpdlc_msg_thr_id_t *thr_ids;
	unsigned num_thr_ids;

	CPDLC_ASSERT(box != NULL);

	thr_ids = fans_get_thr_ids(box, &num_thr_ids, box->msg_log_open);
	if (num_thr_ids == 0) {
		fans_set_num_subpages(box, 1);
	} else {
		fans_set_num_subpages(box,
		    ceil(num_thr_ids / (double)MSG_LOG_LINES));
	}

	fans_put_page_title(box, "CPDLC MESSAGE LOG");
	fans_put_page_ind(box, FMS_COLOR_WHITE);
	for (unsigned i = 0; i < MSG_LOG_LINES; i++) {
		unsigned thr_i = i + box->subpage * MSG_LOG_LINES;
		if (thr_i >= num_thr_ids)
			break;
		msg_log_draw_thr(box, thr_ids[thr_i], i);
	}

	fans_put_str(box, LSK_HEADER_ROW(LSK5_ROW), 0, true, FMS_COLOR_CYAN,
	    FMS_FONT_SMALL, "FILTER");
	fans_put_altn_selector(box, LSK5_ROW, true, !box->msg_log_open,
	    "OPEN", "ALL", NULL);

	free(thr_ids);
}

bool
fans_msg_log_key_cb(fans_t *box, fms_key_t key)
{
	enum { MSG_LOG_LINES = 4 };

	CPDLC_ASSERT(box != NULL);

	if (key >= FMS_KEY_LSK_L1 && key <= FMS_KEY_LSK_L4) {
		unsigned num_thr_ids;
		cpdlc_msg_thr_id_t *thr_ids = fans_get_thr_ids(box,
		    &num_thr_ids, box->msg_log_open);
		unsigned thr_nr = (key - FMS_KEY_LSK_L1) +
		    (box->subpage * MSG_LOG_LINES);

		if (thr_nr < num_thr_ids) {
			fans_set_thr_id(box, thr_ids[thr_nr]);
			fans_set_page(box, FMS_PAGE_MSG_THR, true);
		}
		free(thr_ids);
	} else if (key == FMS_KEY_LSK_R5) {
		box->msg_log_open = !box->msg_log_open;
	} else {
		return (false);
	}

	return (true);
}

static const cpdlc_msg_t *
msg_get_last_uplink(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(box->thr_id != CPDLC_NO_MSG_THR_ID);

	for (unsigned i = 0,
	    n = cpdlc_msglist_get_thr_msg_count(box->msglist, box->thr_id);
	    i < n; i++) {
		const cpdlc_msg_t *msg;
		bool sent;

		cpdlc_msglist_get_thr_msg(box->msglist, box->thr_id, n - i - 1,
		    &msg, NULL, NULL, NULL, &sent);
		if (!sent)
			return (msg);
	}
	return (NULL);
}

static bool
msg_can_resp(fans_t *box, cpdlc_resp_type_t resp)
{
	const cpdlc_msg_t *msg;

	if (cpdlc_client_get_logon_status(box->cl, NULL) !=
	    CPDLC_LOGON_COMPLETE)
		return (false);

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(box->thr_id != CPDLC_NO_MSG_THR_ID);
	msg = msg_get_last_uplink(box);
	return (msg != NULL && msg->segs[0].info->resp == resp);
}

static bool
msg_can_roger(fans_t *box)
{
	return (!cpdlc_msglist_thr_is_done(box->msglist, box->thr_id) &&
	    msg_can_resp(box, CPDLC_RESP_R));
}

static bool
msg_can_wilco(fans_t *box)
{
	return (!cpdlc_msglist_thr_is_done(box->msglist, box->thr_id) &&
	    msg_can_resp(box, CPDLC_RESP_WU));
}

static bool
msg_can_affirm(fans_t *box)
{
	return (!cpdlc_msglist_thr_is_done(box->msglist, box->thr_id) &&
	    msg_can_resp(box, CPDLC_RESP_AN));
}

static bool
msg_can_standby(fans_t *box)
{
	cpdlc_msg_thr_status_t st =
	    cpdlc_msglist_get_thr_status(box->msglist, box->thr_id, NULL);
	return (!cpdlc_msglist_thr_is_done(box->msglist, box->thr_id) &&
	    st != CPDLC_MSG_THR_STANDBY &&
	    (msg_can_resp(box, CPDLC_RESP_AN) ||
	    msg_can_resp(box, CPDLC_RESP_WU) ||
	    msg_can_resp(box, CPDLC_RESP_R)));
}

static void
send_quick_resp(fans_t *box, int msg_type)
{
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);

	CPDLC_ASSERT(box != NULL);

	cpdlc_msg_add_seg(msg, true, msg_type, 0);
	cpdlc_msglist_send(box->msglist, msg, box->thr_id);
}

static void
send_roger(fans_t *box)
{
	send_quick_resp(box, CPDLC_DM3_ROGER);
}

static void
send_wilco(fans_t *box)
{
	send_quick_resp(box, CPDLC_DM0_WILCO);
}

static void
send_affirm(fans_t *box)
{
	send_quick_resp(box, CPDLC_DM4_AFFIRM);
}

static void
send_standby(fans_t *box)
{
	send_quick_resp(box, CPDLC_DM2_STANDBY);
}

static void
draw_response_section(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, false,
	    FMS_COLOR_CYAN, FMS_FONT_SMALL, "--------RESPONSE--------");

	switch (cpdlc_msglist_get_thr_status(box->msglist, box->thr_id, NULL)) {
	case CPDLC_MSG_THR_OPEN:
	case CPDLC_MSG_THR_STANDBY:
		if (msg_can_roger(box) || msg_can_wilco(box) ||
		    msg_can_affirm(box)) {
			fans_put_lsk_action(box, FMS_KEY_LSK_L4,
			    FMS_COLOR_CYAN, "*ACPT");
			fans_put_lsk_action(box, FMS_KEY_LSK_R5,
			    FMS_COLOR_WHITE, "REJ>");
		}
		if (msg_can_standby(box)) {
			fans_put_lsk_action(box, FMS_KEY_LSK_L5,
			    FMS_COLOR_CYAN, "*STBY");
		}
		break;
	case CPDLC_MSG_THR_ACCEPTED:
		fans_put_lsk_action(box, FMS_KEY_LSK_L4,
		    FMS_COLOR_WHITE, "ACPT");
		break;
	case CPDLC_MSG_THR_REJECTED:
		fans_put_lsk_action(box, FMS_KEY_LSK_L4,
		    FMS_COLOR_WHITE, "REJ");
		break;
	default:
		break;
	}
}

void
fans_msg_thr_draw_cb(fans_t *box)
{
	enum { MAX_LINES = 5 };
	char **lines = NULL;
	unsigned n_lines = 0;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(box->thr_id != CPDLC_NO_MSG_THR_ID);

	cpdlc_msglist_thr_mark_seen(box->msglist, box->thr_id);

	fans_thr2lines(box->msglist, box->thr_id, &lines, &n_lines);
	fans_set_num_subpages(box, ceil(n_lines / (double)MAX_LINES));

	fans_put_page_title(box, "CPDLC MESSAGE");
	fans_put_page_ind(box, FMS_COLOR_WHITE);
	draw_thr_hdr(box, 1, box->thr_id);
	for (unsigned i = 0; i < MAX_LINES; i++) {
		unsigned line = i + box->subpage * MAX_LINES;
		if (line >= n_lines)
			break;
		fans_put_str(box, 2 + i, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", lines[line]);
	}
	draw_response_section(box);

	fans_free_lines(lines, n_lines);
}

bool
fans_msg_thr_key_cb(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L6) {
		fans_set_page(box, FMS_PAGE_MSG_LOG, false);
	} else if (key == FMS_KEY_LSK_L4) {
		if (msg_can_roger(box))
			send_roger(box);
		else if (msg_can_wilco(box))
			send_wilco(box);
		else if (msg_can_affirm(box))
			send_affirm(box);
	} else if (key == FMS_KEY_LSK_L5) {
		if (msg_can_standby(box))
			send_standby(box);
	} else if (key == FMS_KEY_LSK_R5) {
		if (msg_can_roger(box) || msg_can_wilco(box))
			fans_rej(box, true, FMS_PAGE_MSG_THR);
		else if (msg_can_affirm(box))
			fans_rej(box, false, FMS_PAGE_MSG_THR);
	} else {
		return (false);
	}

	return (true);
}
