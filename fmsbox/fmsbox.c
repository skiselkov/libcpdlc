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
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "../src/cpdlc_alloc.h"
#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_client.h"
#include "../src/cpdlc_core.h"
#include "../src/cpdlc_msglist.h"
#include "../src/cpdlc_string.h"

#include "fmsbox.h"

#define	SCRATCHPAD_MAX	22
#define	SCRATCHPAD_ROW	13
#define	LSK1_ROW	2
#define	LSK2_ROW	4
#define	LSK3_ROW	6
#define	LSK4_ROW	8
#define	LSK5_ROW	10
#define	LSK6_ROW	12

typedef struct {
	void	(*draw_cb)(fmsbox_t *box);
	bool	(*key_cb)(fmsbox_t *box, fms_key_t key);
} fms_page_t;

struct fmsbox_s {
	fmsbox_char_t	scr[FMSBOX_ROWS][FMSBOX_COLS];
	fms_page_t	*page;
	unsigned	subpage;
	unsigned	num_subpages;
	char		scratchpad[SCRATCHPAD_MAX + 1];

	char		flt_id[9];
	char		orig[5];
	char		dest[5];

	cpdlc_client_t	*cl;
	cpdlc_msglist_t	*msglist;
};

enum {
	FMS_PAGE_ATN_MENU,
	FMS_PAGE_CPDLC_LOGON,
	FMS_PAGE_ATN_STATUS,
	FMS_PAGE_ATC_LOG,
	FMS_NUM_PAGES
};

static void atn_menu_draw_cb(fmsbox_t *box);
static bool atn_menu_key_cb(fmsbox_t *box, fms_key_t key);
static void cpdlc_logon_draw_cb(fmsbox_t *box);
static bool cpdlc_logon_key_cb(fmsbox_t *box, fms_key_t key);
static void atn_status_draw_cb(fmsbox_t *box);
static bool atn_status_key_cb(fmsbox_t *box, fms_key_t key);
static void atc_log_draw_cb(fmsbox_t *box);
static bool atc_log_key_cb(fmsbox_t *box, fms_key_t key);

static fms_page_t fms_pages[FMS_NUM_PAGES] = {
	{	/* FMS_PAGE_ATN_MENU */
		.draw_cb = atn_menu_draw_cb,
		.key_cb = atn_menu_key_cb
	},
	{	/* FMS_PAGE_CPDLC_LOGON */
		.draw_cb = cpdlc_logon_draw_cb,
		.key_cb = cpdlc_logon_key_cb
	},
	{	/* FMS_PAGE_ATN_STATUS */
		.draw_cb = atn_status_draw_cb,
		.key_cb = atn_status_key_cb
	},
	{	/* FMS_PAGE_ATC_LOG */
		.draw_cb = atc_log_draw_cb,
		.key_cb = atc_log_key_cb
	}
};

static void put_str(fmsbox_t *box, unsigned row, unsigned col,
    bool align_right, fms_color_t color, fms_font_t size,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(7);

static void
set_page(fmsbox_t *box, fms_page_t *page)
{
	ASSERT(box != NULL);
	ASSERT(page != NULL);
	ASSERT(page->draw_cb != NULL);
	ASSERT(page->key_cb != NULL);
	box->page = page;
	box->subpage = 0;
	box->num_subpages = 0;
}

void
set_num_subpages(fmsbox_t *box, unsigned num)
{
	ASSERT(box != NULL);
	box->num_subpages = num;
	box->subpage %= num;
}

static bool
can_send_logon(const fmsbox_t *box, cpdlc_logon_status_t st)
{
	ASSERT(box != NULL);
	return (box->flt_id[0] != '\0' && box->orig[0] != '\0' &&
	    box->dest[0] != '\0' &&
	    (st == CPDLC_LOGON_NONE || st == CPDLC_LOGON_LINK_AVAIL));
}

static void
put_str_v(fmsbox_t *box, unsigned row, unsigned col, bool align_right,
    fms_color_t color, fms_font_t size, const char *fmt, va_list ap)
{
	char *text;
	int l;
	va_list ap2;

	ASSERT(box != NULL);
	ASSERT(fmt != NULL);
	ASSERT3U(row, <, FMSBOX_ROWS);
	ASSERT3U(col, <, FMSBOX_COLS);

	va_copy(ap2, ap);
	l = vsnprintf(NULL, 0, fmt, ap);
	text = safe_malloc(l + 1);
	vsnprintf(text, l + 1, fmt, ap2);

	if (align_right)
		col = MAX(FMSBOX_COLS - (int)col - l, 0);

	for (int i = 0, n = strlen(text); i < n; i++) {
		if (col + i >= FMSBOX_COLS)
			break;
		box->scr[row][col + i].c = text[i];
		box->scr[row][col + i].color = color;
		box->scr[row][col + i].size = size;
	}

	free(text);
	va_end(ap2);
}

static void
put_str(fmsbox_t *box, unsigned row, unsigned col, bool align_right,
    fms_color_t color, fms_font_t size, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	put_str_v(box, row, col, align_right, color, size, fmt, ap);
	va_end(ap);
}

static void
put_page_title(fmsbox_t *box, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	put_str_v(box, 0, 0, false, FMS_COLOR_WHITE, FMS_FONT_LARGE, fmt, ap);
	va_end(ap);
}

static void
put_lsk_action(fmsbox_t *box, int lsk_key_id, fms_color_t color,
    const char *fmt, ...)
{
	int row;
	bool align_right;
	va_list ap;

	if (lsk_key_id >= FMS_KEY_LSK_L1 && lsk_key_id <= FMS_KEY_LSK_L6) {
		row = 2 * (lsk_key_id - FMS_KEY_LSK_L1) + 2;
		align_right = false;
	} else if (lsk_key_id >= FMS_KEY_LSK_R1 &&
	    lsk_key_id <= FMS_KEY_LSK_R6) {
		row = 2 * (lsk_key_id - FMS_KEY_LSK_R1) + 2;
		align_right = true;
	} else {
		VERIFY_MSG(0, "Invalid lsk_key_id = %x", lsk_key_id);
	}

	va_start(ap, fmt);
	put_str_v(box, row, 0, align_right, color, FMS_FONT_LARGE, fmt, ap);
	va_end(ap);
}

static void
clear_screen(fmsbox_t *box)
{
	ASSERT(box != NULL);

	for (unsigned row = 0; row < FMSBOX_ROWS; row++) {
		for (unsigned col = 0; col < FMSBOX_COLS; col++) {
			box->scr[row][col].c = ' ';
			box->scr[row][col].color = FMS_COLOR_WHITE;
			box->scr[row][col].size = FMS_FONT_SMALL;
		}
	}
}

static void
update_scratchpad(fmsbox_t *box)
{
	put_str(box, SCRATCHPAD_ROW, 0, false, FMS_COLOR_CYAN,
	    FMS_FONT_LARGE, "[");
	put_str(box, SCRATCHPAD_ROW, 1, false, FMS_COLOR_WHITE,
	    FMS_FONT_LARGE, "%s", box->scratchpad);
	put_str(box, SCRATCHPAD_ROW, 0, true, FMS_COLOR_CYAN,
	    FMS_FONT_LARGE, "]");
}

static void
scratchpad_xfer(fmsbox_t *box, char *dest, size_t cap)
{
	if (strcmp(box->scratchpad, "DELETE") == 0) {
		memset(dest, 0, cap);
		memset(box->scratchpad, 0, sizeof (box->scratchpad));
	} else if (box->scratchpad[0] == '\0') {
		cpdlc_strlcpy(box->scratchpad, dest,
		    sizeof (box->scratchpad));
	} else {
		cpdlc_strlcpy(dest, box->scratchpad, cap);
		memset(box->scratchpad, 0, sizeof (box->scratchpad));
	}
}

fmsbox_t *
fmsbox_alloc(const char *hostname, unsigned port, const char *ca_file)
{
	fmsbox_t *box = safe_calloc(1, sizeof (*box));

	ASSERT(hostname != NULL);
	ASSERT3U(port, <=, UINT16_MAX);
	ASSERT(ca_file != NULL);

	box->cl = cpdlc_client_alloc(hostname, port, ca_file, false);
	box->msglist = cpdlc_msglist_alloc(box->cl);
	set_page(box, &fms_pages[FMS_PAGE_ATN_MENU]);

	fmsbox_update(box);

	return (box);
}

void
fmsbox_free(fmsbox_t *box)
{
	ASSERT(box != NULL);
	ASSERT(box->msglist != NULL);
	ASSERT(box->cl != NULL);

	cpdlc_msglist_free(box->msglist);
	cpdlc_client_free(box->cl);
	free(box);
}

const fmsbox_char_t *
fmsbox_get_screen_row(const fmsbox_t *box, unsigned row)
{
	ASSERT(box != NULL);
	ASSERT3U(row, <, FMSBOX_ROWS);
	return (box->scr[row]);
}

static void
del_key(fmsbox_t *box)
{
	unsigned l;

	ASSERT(box != NULL);

	l = strlen(box->scratchpad);
	if (l == 0) {
		cpdlc_strlcpy(box->scratchpad, "DELETE",
		    sizeof (box->scratchpad));
	} else if (strcmp(box->scratchpad, "DELETE") == 0) {
		memset(box->scratchpad, 0, sizeof (box->scratchpad));
	} else {
		box->scratchpad[l - 1] = '\0';
	}
}

void
fmsbox_push_key(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);
	ASSERT(box->page != NULL);
	ASSERT(box->page->key_cb != NULL);

	if (!box->page->key_cb(box, key)) {
		if (key == FMS_KEY_CLR_DEL) {
			del_key(box);
		} else if (key == FMS_KEY_NEXT && box->num_subpages > 0) {
			box->subpage = (box->subpage + 1) % box->num_subpages;
		} else if (key == FMS_KEY_PREV && box->num_subpages > 0) {
			if (box->subpage > 0)
				box->subpage--;
			else
				box->subpage = box->num_subpages - 1;
		}
	}
	fmsbox_update(box);
}

void
fmsbox_push_char(fmsbox_t *box, char c)
{
	unsigned len;

	ASSERT(box != NULL);
	ASSERT(c != 0);

	if (strcmp(box->scratchpad, "DELETE") == 0)
		memset(box->scratchpad, 0, sizeof (box->scratchpad));

	len = strlen(box->scratchpad);
	if (len + 1 < sizeof (box->scratchpad))
		box->scratchpad[len] = toupper(c);

	fmsbox_update(box);
}

void
fmsbox_update(fmsbox_t *box)
{
	ASSERT(box != NULL);
	ASSERT(box->page != NULL);

	clear_screen(box);
	ASSERT(box->page->draw_cb != NULL);
	box->page->draw_cb(box);
	update_scratchpad(box);
}

static void
put_page_ind(fmsbox_t *box, fms_color_t color)
{
	ASSERT(box->num_subpages != 0);
	put_str(box, 0, 0, true, color, FMS_FONT_LARGE, "%d/%d",
	    box->subpage + 1, box->num_subpages);
}

static void
put_cur_time(fmsbox_t *box)
{
	time_t now;
	const struct tm *tm;

	ASSERT(box != NULL);

	now = time(NULL);
	tm = localtime(&now);
	put_str(box, LSK6_ROW, 8, false, FMS_COLOR_WHITE, FMS_FONT_SMALL,
	    "%02d:%02d", tm->tm_hour, tm->tm_min);
}

static void
send_logon(fmsbox_t *box)
{
	char buf[64];

	ASSERT(box != NULL);

	snprintf(buf, sizeof (buf), "ORIG=%s,DEST=%s", box->orig, box->dest);
	cpdlc_client_logon(box->cl, buf, box->flt_id, "ATSU");
}

static void
send_logoff(fmsbox_t *box)
{
	cpdlc_client_logoff(box->cl);
}

static void
atn_menu_draw_cb(fmsbox_t *box)
{
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl);

	put_page_title(box, "ATN-ATN MENU");
	put_lsk_action(box, FMS_KEY_LSK_L1, FMS_COLOR_WHITE, "<LOGON/STATUS");
	if (st == CPDLC_LOGON_COMPLETE) {
		put_lsk_action(box, FMS_KEY_LSK_R1, FMS_COLOR_CYAN, "LOGOFF*");
		put_lsk_action(box, FMS_KEY_LSK_L2, FMS_COLOR_WHITE,
		    "<ATC LOG");
		put_lsk_action(box, FMS_KEY_LSK_L3, FMS_COLOR_WHITE,
		    "<REPORTS/REQUESTS");
	}
	put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
	    "<ATN LINK STATUS");

	put_cur_time(box);
}

static bool
atn_menu_key_cb(fmsbox_t *box, fms_key_t key)
{
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl);

	if (key == FMS_KEY_LSK_L1)
		set_page(box, &fms_pages[FMS_PAGE_CPDLC_LOGON]);
	else if (key == FMS_KEY_LSK_R1 && st == CPDLC_LOGON_COMPLETE)
		send_logoff(box);
	else if (key == FMS_KEY_LSK_L2)
		set_page(box, &fms_pages[FMS_PAGE_ATC_LOG]);
	else if (key == FMS_KEY_LSK_L5)
		set_page(box, &fms_pages[FMS_PAGE_ATN_STATUS]);
	else
		return (false);

	return (true);
}

static void
cpdlc_logon_draw_cb(fmsbox_t *box)
{
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl);

	put_page_title(box, "CPDLC-LOGON/STATUS");
	if (can_send_logon(box, st)) {
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "SEND LOGON*");
	} else if (st == CPDLC_LOGON_CONNECTING_LINK ||
	    st == CPDLC_LOGON_HANDSHAKING_LINK || st == CPDLC_LOGON_IN_PROG) {
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "IN PROGRESS");
	} else if (st == CPDLC_LOGON_COMPLETE) {
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "LOGGED ON");
	}
	put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");

	put_str(box, 1, 5, false, FMS_COLOR_WHITE, FMS_FONT_SMALL, "CDA");
	put_str(box, 2, 3, false, FMS_COLOR_GREEN, FMS_FONT_SMALL, "--------");
	put_str(box, 1, 5, true, FMS_COLOR_WHITE, FMS_FONT_SMALL, "NDA");
	put_str(box, 2, 2, true, FMS_COLOR_GREEN, FMS_FONT_SMALL, "--------");
	put_str(box, 3, 0, false, FMS_COLOR_WHITE, FMS_FONT_SMALL, "FLT ID");
	if (box->flt_id[0] != '\0') {
		put_str(box, 4, 0, false, FMS_COLOR_CYAN, FMS_FONT_SMALL,
		    "%s", box->flt_id);
	} else {
		put_str(box, 4, 0, false, FMS_COLOR_CYAN, FMS_FONT_SMALL,
		    "--------");
	}
	put_str(box, 5, 0, false, FMS_COLOR_WHITE, FMS_FONT_SMALL, "ORIG STA");
	if (box->orig[0] != '\0') {
		put_str(box, 6, 0, false, FMS_COLOR_CYAN, FMS_FONT_SMALL,
		    "%s", box->orig);
	} else {
		put_str(box, 6, 0, false, FMS_COLOR_CYAN, FMS_FONT_SMALL,
		    "----");
	}
	put_str(box, 5, 0, true, FMS_COLOR_WHITE, FMS_FONT_SMALL, "DEST STA");
	if (box->dest[0] != '\0') {
		put_str(box, 6, 0, true, FMS_COLOR_CYAN, FMS_FONT_SMALL,
		    "%s", box->dest);
	} else {
		put_str(box, 6, 0, true, FMS_COLOR_CYAN, FMS_FONT_SMALL,
		    "----");
	}

	put_cur_time(box);
}

static bool
cpdlc_logon_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L6) {
		set_page(box, &fms_pages[FMS_PAGE_ATN_MENU]);
	} else if (key == FMS_KEY_LSK_L2) {
		scratchpad_xfer(box, box->flt_id, sizeof (box->flt_id));
	} else if (key == FMS_KEY_LSK_L3) {
		scratchpad_xfer(box, box->orig, sizeof (box->orig));
	} else if (key == FMS_KEY_LSK_R3) {
		scratchpad_xfer(box, box->dest, sizeof (box->dest));
	} else if (key == FMS_KEY_LSK_R5) {
		if (can_send_logon(box, cpdlc_client_get_logon_status(box->cl)))
			send_logon(box);
	} else {
		return (false);
	}

	return (true);
}

static void
atn_status_draw(fmsbox_t *box, const char *line1, const char *line2)
{
	ASSERT(box != NULL);

	if (line1 != NULL) {
		put_str(box, 2, 1, false, FMS_COLOR_GREEN, FMS_FONT_SMALL,
		    "%s", line1);
	}
	if (line2 != NULL) {
		put_str(box, 3, 1, false, FMS_COLOR_GREEN, FMS_FONT_SMALL,
		    "%s", line2);
	}
}

static void
atn_status_draw_cb(fmsbox_t *box)
{
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl);

	put_page_title(box, "CPDLC-STATUS");

	switch (st) {
	case CPDLC_LOGON_NONE:
		atn_status_draw(box, "ATN IS AVAILABLE.", "YOU MAY LOGON.");
		break;
	case CPDLC_LOGON_CONNECTING_LINK:
	case CPDLC_LOGON_HANDSHAKING_LINK:
		atn_status_draw(box, "ATN IS CONNECTING.", "PLEASE WAIT.");
		break;
	case CPDLC_LOGON_LINK_AVAIL:
		atn_status_draw(box, "ATN IS AVAILABLE.",
		    "CPDLC IS NOT CONNECTED.");
		break;
	case CPDLC_LOGON_IN_PROG:
		atn_status_draw(box, "LOGON IN PROGRESS.", "PLEASE WAIT.");
		break;
	case CPDLC_LOGON_COMPLETE:
		atn_status_draw(box, "CPDLC CONNECTION",
		    "ESTABLIHED WITH ATSU.");
		break;
	}

	put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_WHITE, "LOGON/STATUS>");
	put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");

	put_cur_time(box);
}

static bool
atn_status_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);
	if (key == FMS_KEY_LSK_R5) {
		set_page(box, &fms_pages[FMS_PAGE_CPDLC_LOGON]);
	} else if (key == FMS_KEY_LSK_L6) {
		set_page(box, &fms_pages[FMS_PAGE_ATN_MENU]);
	} else {
		return (false);
	}
	return (true);
}

static cpdlc_msg_thr_id_t *
get_thr_ids(fmsbox_t *box, unsigned *num_thr_ids)
{
	cpdlc_msg_thr_id_t *thr_ids;

	ASSERT(box != NULL);
	ASSERT(num_thr_ids != NULL);

	*num_thr_ids = 0;
	cpdlc_msglist_get_thr_ids(box->msglist, NULL, num_thr_ids);
	thr_ids = safe_calloc(*num_thr_ids, sizeof (*thr_ids));
	cpdlc_msglist_get_thr_ids(box->msglist, thr_ids, num_thr_ids);

	return (thr_ids);
}

static const char *
thr_status2str(cpdlc_msg_thr_status_t st)
{
	switch (st) {
	case CPDLC_MSG_THR_NEW:
		return ("NEW");
	case CPDLC_MSG_THR_OPEN:
		return ("OPEN");
	case CPDLC_MSG_THR_CLOSED:
		return ("CLOSED");
	case CPDLC_MSG_THR_ACCEPTED:
		return ("ACCEPTED");
	case CPDLC_MSG_THR_REJECTED:
		return ("REJECTED");
	case CPDLC_MSG_THR_TIMEDOUT:
		return ("TIMEDOUT");
	case CPDLC_MSG_THR_FAILED:
		return ("FAILED");
	case CPDLC_MSG_THR_PENDING:
		return ("PENDING");
	case CPDLC_MSG_THR_ERROR:
		return ("ERROR");
	default:
		VERIFY_MSG(0, "Invalid thread status %x", st);
	}
}

static void
atc_log_draw_thr(fmsbox_t *box, cpdlc_msg_thr_id_t thr_id, unsigned thr_i)
{
	const cpdlc_msg_t *msg;
	cpdlc_msg_thr_status_t st;
	char buf[64];
	unsigned hours, mins;
	bool sent;

	ASSERT(box != NULL);
	ASSERT3U(thr_i, <, 5);

	cpdlc_msglist_get_thr_msg(box->msglist, thr_id, 0, &msg, NULL,
	    &hours, &mins, &sent);
	st = cpdlc_msglist_get_thr_status(box->msglist, thr_id);

	/* Message uplink/downlink, time and sender */
	put_str(box, 2 * thr_i + 1, 0, false, FMS_COLOR_GREEN, FMS_FONT_SMALL,
	    "%s %02d:%02d%s%s", sent ? "v" : "^", hours, mins, sent ? "" : "-",
	    sent ? "" : cpdlc_msg_get_from(msg));
	/* Initiating message MIN value */
	put_str(box, 2 * thr_i + 1, 9, true, FMS_COLOR_GREEN, FMS_FONT_SMALL,
	    "%d", cpdlc_msg_get_min(msg) % 64);
	/* Thread status */
	put_str(box, 2 * thr_i + 1, 0, true, FMS_COLOR_GREEN, FMS_FONT_SMALL,
	    "%s", thr_status2str(st));
	/* Message printout */
	cpdlc_msg_readable(msg, buf, sizeof (buf));
	put_str(box, 2 * thr_i + 2, 0, false, FMS_COLOR_WHITE, FMS_FONT_LARGE,
	    "<%s", buf);
}

static void
atc_log_draw_cb(fmsbox_t *box)
{
	cpdlc_msg_thr_id_t *thr_ids;
	unsigned num_thr_ids;

	ASSERT(box != NULL);

	thr_ids = get_thr_ids(box, &num_thr_ids);
	if (num_thr_ids == 0)
		set_num_subpages(box, 1);
	else
		set_num_subpages(box, ceil(num_thr_ids / 5.0));

	put_page_title(box, "CPDLC-ATC LOG");
	put_page_ind(box, FMS_COLOR_WHITE);

	for (unsigned thr_i = box->subpage * 5; thr_i < num_thr_ids; thr_i++)
		atc_log_draw_thr(box, thr_ids[num_thr_ids - thr_i - 1], thr_i);

	put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
	put_cur_time(box);

	free(thr_ids);
}

static bool
atc_log_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L6)
		set_page(box, &fms_pages[FMS_PAGE_ATN_MENU]);
	else
		return (false);

	return (true);
}
