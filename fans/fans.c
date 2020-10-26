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

#include "fans.h"
#include "fans_emer.h"
#include "fans_freetext.h"
#include "fans_impl.h"
#include "fans_logon_status.h"
#include "fans_main_menu.h"
#include "fans_msg.h"
#include "fans_parsing.h"
#include "fans_pos_pick.h"
#include "fans_pos_rep.h"
#include "fans_req.h"
#include "fans_req_alt.h"
#include "fans_req_clx.h"
#include "fans_req_off.h"
#include "fans_req_rte.h"
#include "fans_req_spd.h"
#include "fans_req_vmc.h"
#include "fans_req_voice.h"
#include "fans_req_wcw.h"
#include "fans_scratchpad.h"
#include "fans_rej.h"
#include "fans_vrfy.h"

static fms_page_t fms_pages[FMS_NUM_PAGES] = {
	{	/* FMS_PAGE_MAIN_MENU */
		.draw_cb = fans_main_menu_draw_cb,
		.key_cb = fans_main_menu_key_cb
	},
	{	/* FMS_PAGE_LOGON_STATUS */
		.init_cb = fans_logon_status_init_cb,
		.draw_cb = fans_logon_status_draw_cb,
		.key_cb = fans_logon_status_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_MSG_LOG */
		.draw_cb = fans_msg_log_draw_cb,
		.key_cb = fans_msg_log_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_MSG_THR */
		.draw_cb = fans_msg_thr_draw_cb,
		.key_cb = fans_msg_thr_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_FREETEXT */
		.init_cb = fans_freetext_init_cb,
		.draw_cb = fans_freetext_draw_cb,
		.key_cb = fans_freetext_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_REQUESTS */
		.draw_cb = fans_requests_draw_cb,
		.key_cb = fans_requests_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_REQ_ALT */
		.init_cb = fans_req_alt_init_cb,
		.draw_cb = fans_req_alt_draw_cb,
		.key_cb = fans_req_alt_key_cb
	},
	{	/* FMS_PAGE_REQ_OFF */
		.init_cb = fans_req_off_init_cb,
		.draw_cb = fans_req_off_draw_cb,
		.key_cb = fans_req_off_key_cb
	},
	{	/* FMS_PAGE_REQ_SPD */
		.init_cb = fans_req_spd_init_cb,
		.draw_cb = fans_req_spd_draw_cb,
		.key_cb = fans_req_spd_key_cb
	},
	{	/* FMS_PAGE_REQ_RTE */
		.init_cb = fans_req_rte_init_cb,
		.draw_cb = fans_req_rte_draw_cb,
		.key_cb = fans_req_rte_key_cb
	},
	{	/* FMS_PAGE_REQ_CLX */
		.init_cb = fans_req_clx_init_cb,
		.draw_cb = fans_req_clx_draw_cb,
		.key_cb = fans_req_clx_key_cb
	},
	{	/* FMS_PAGE_REQ_VMC */
		.draw_cb = fans_req_vmc_draw_cb,
		.key_cb = fans_req_vmc_key_cb
	},
	{	/* FMS_PAGE_REQ_WCW */
		.init_cb = fans_req_wcw_init_cb,
		.draw_cb = fans_req_wcw_draw_cb,
		.key_cb = fans_req_wcw_key_cb
	},
	{	/* FMS_PAGE_REQ_VOICE */
		.init_cb = fans_req_voice_init_cb,
		.draw_cb = fans_req_voice_draw_cb,
		.key_cb = fans_req_voice_key_cb
	},
	{	/* FMS_PAGE_REJ */
		.draw_cb = fans_rej_draw_cb,
		.key_cb = fans_rej_key_cb
	},
	{	/* FMS_PAGE_VRFY */
		.draw_cb = fans_vrfy_draw_cb,
		.key_cb = fans_vrfy_key_cb
	},
	{	/* FMS_PAGE_EMER */
		.init_cb = fans_emer_init_cb,
		.draw_cb = fans_emer_draw_cb,
		.key_cb = fans_emer_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_POS_PICK */
		.draw_cb = fans_pos_pick_draw_cb,
		.key_cb = fans_pos_pick_key_cb
	},
	{	/* FMS_PAGE_POS_REP */
		.init_cb = fans_pos_rep_init_cb,
		.draw_cb = fans_pos_rep_draw_cb,
		.key_cb = fans_pos_rep_key_cb,
		.has_return = true
	}
};

static void draw_atc_msg_lsk(fans_t *box);
static void handle_atc_msg_lsk(fans_t *box);
static void put_cur_time(fans_t *box);

void
fans_set_thr_id(fans_t *box, cpdlc_msg_thr_id_t thr_id)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(thr_id != CPDLC_NO_MSG_THR_ID);
	box->thr_id = thr_id;
	cpdlc_msglist_thr_mark_seen(box->msglist, thr_id);
}

void
fans_set_page(fans_t *box, unsigned page_nr, bool init)
{
	fms_page_t *page;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT3U(page_nr, <, FMS_NUM_PAGES);
	page = &fms_pages[page_nr];
	CPDLC_ASSERT(page->draw_cb != NULL);
	CPDLC_ASSERT(page->key_cb != NULL);

	box->page = page;
	box->subpage = 0;
	box->num_subpages = 0;

	if (init && page->init_cb != NULL)
		page->init_cb(box);
}

void
fans_set_num_subpages(fans_t *box, unsigned num)
{
	CPDLC_ASSERT(box != NULL);
	box->num_subpages = num;
	box->subpage %= num;
}

void
fans_set_error(fans_t *box, const char *error)
{
	CPDLC_ASSERT(box != NULL);

	if (error != NULL)
		cpdlc_strlcpy(box->error_msg, error, sizeof (box->error_msg));
	else
		memset(box->error_msg, 0, sizeof (box->error_msg));
}

static int
put_str_v(fans_t *box, unsigned row, unsigned col, bool align_right,
    fms_color_t color, fms_font_t size, const char *fmt, va_list ap)
{
	char *text;
	int l;
	va_list ap2;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(fmt != NULL);
	CPDLC_ASSERT3U(row, <, FMS_ROWS);
	CPDLC_ASSERT3U(col, <, FMS_COLS);

	va_copy(ap2, ap);
	l = vsnprintf(NULL, 0, fmt, ap);
	text = safe_malloc(l + 1);
	vsnprintf(text, l + 1, fmt, ap2);

	if (align_right)
		col = MAX(FMS_COLS - (int)col - l, 0);

	for (int i = 0; i < l; i++) {
		if (col + i >= FMS_COLS)
			break;
		box->scr[row][col + i].c = text[i];
		box->scr[row][col + i].color = color;
		box->scr[row][col + i].size = size;
	}

	free(text);
	va_end(ap2);

	return (l);
}

int
fans_put_str(fans_t *box, unsigned row, unsigned col, bool align_right,
    fms_color_t color, fms_font_t size, const char *fmt, ...)
{
	int l;
	va_list ap;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT3U(color, <=, FMS_COLOR_MAGENTA);
	CPDLC_ASSERT3U(size, <=, FMS_FONT_LARGE);
	CPDLC_ASSERT(fmt != NULL);

	va_start(ap, fmt);
	l = put_str_v(box, row, col, align_right, color, size, fmt, ap);
	va_end(ap);

	return (l);
}

void
fans_put_page_title(fans_t *box, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	put_str_v(box, 0, 0, false, FMS_COLOR_WHITE, FMS_FONT_LARGE, fmt, ap);
	va_end(ap);
}

static void
lsk2row(int lsk_key_id, int *row, bool *align_right)
{
	CPDLC_ASSERT(row != NULL);
	CPDLC_ASSERT(align_right);
	if (lsk_key_id >= FMS_KEY_LSK_L1 && lsk_key_id <= FMS_KEY_LSK_L6) {
		*row = 2 * (lsk_key_id - FMS_KEY_LSK_L1) + 2;
		*align_right = false;
	} else if (lsk_key_id >= FMS_KEY_LSK_R1 &&
	    lsk_key_id <= FMS_KEY_LSK_R6) {
		*row = 2 * (lsk_key_id - FMS_KEY_LSK_R1) + 2;
		*align_right = true;
	} else {
		CPDLC_VERIFY_MSG(0, "Invalid lsk_key_id = %x", lsk_key_id);
	}
}

void
fans_put_lsk_action(fans_t *box, int lsk_key_id, fms_color_t color,
    const char *fmt, ...)
{
	int row;
	bool align_right;
	va_list ap;

	lsk2row(lsk_key_id, &row, &align_right);
	va_start(ap, fmt);
	put_str_v(box, row, 0, align_right, color, FMS_FONT_LARGE, fmt, ap);
	va_end(ap);
}

void
fans_put_lsk_title(fans_t *box, int lsk_key_id, const char *fmt, ...)
{
	int row;
	bool align_right;
	va_list ap;

	lsk2row(lsk_key_id, &row, &align_right);
	/* Put one row above */
	row--;
	va_start(ap, fmt);
	put_str_v(box, row, 0, align_right, FMS_COLOR_CYAN, FMS_FONT_SMALL,
	    fmt, ap);
	va_end(ap);
}

void
fans_put_altn_selector(fans_t *box, int row, bool align_right,
    int option, const char *first, ...)
{
	va_list ap;
	int idx = 0, offset = 0;

	va_start(ap, first);
	for (const char *str = first, *next = NULL; str != NULL;
	    str = next, idx++) {
		next = va_arg(ap, const char *);
		if (idx == option) {
			fans_put_str(box, row, offset, align_right,
			    FMS_COLOR_GREEN, FMS_FONT_LARGE, "%s", str);
		} else {
			fans_put_str(box, row, offset, align_right,
			    FMS_COLOR_WHITE, FMS_FONT_SMALL, "%s", str);
		}
		offset += strlen(str);
		if (next != NULL) {
			fans_put_str(box, row, offset, align_right,
			    FMS_COLOR_WHITE, FMS_FONT_LARGE, "/");
			offset++;
		}
	}
	va_end(ap);
}

static void
put_str_with_units(fans_t *box, int row, int col, bool align_right,
    fms_color_t color, fms_font_t font, const char *units, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (align_right) {
		put_str_v(box, row, col + strlen(units) + 1, true, color,
		    font, fmt, ap);
		fans_put_str(box, row, col, true, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", units);
	} else {
		int l = put_str_v(box, row, col, false, color, font, fmt, ap);
		fans_put_str(box, row, col + l + 1, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", units);
	}
	va_end(ap);
}

#define	DATA_PICK(__type, __name, __test, __userdata, __autodata) \
	__type __name; \
	fms_color_t color; \
	fms_font_t font; \
	if (__test) { \
		__name = (__userdata); \
		color = FMS_COLOR_WHITE; \
		font = FMS_FONT_LARGE; \
	} else { \
		__name = (__autodata); \
		color = FMS_COLOR_GREEN; \
		font = FMS_FONT_SMALL; \
	}

void
fans_put_alt(fans_t *box, int row, int col, bool align_right,
    const cpdlc_arg_t *useralt, const cpdlc_arg_t *autoalt,
    bool req, bool units)
{
	CPDLC_ASSERT(box != NULL);
	DATA_PICK(const cpdlc_arg_t *, alt, useralt->alt.alt != 0,
	    useralt, autoalt);

	if (alt != NULL && alt->alt.alt != 0) {
		char buf[8];

		fans_print_alt(alt, buf, sizeof (buf));
		if (units && !alt->alt.fl) {
			put_str_with_units(box, row, col, align_right,
			    color, font, "FT", "%s", buf);
		} else {
			fans_put_str(box, row, col, align_right,
			    color, font, "%s", buf);
		}
	} else {
		fans_put_str(box, row, col, align_right, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, req ? "_____" : "-----");
	}
}

void
fans_put_spd(fans_t *box, int row, int col, bool align_right,
    const cpdlc_arg_t *userspd, const cpdlc_arg_t *autospd,
    bool req, bool units)
{
	char buf[8];

	CPDLC_ASSERT(box != NULL);
	DATA_PICK(const cpdlc_arg_t *, spd, userspd->spd.spd != 0,
	    userspd, autospd);

	if (spd != NULL && spd->spd.spd != 0) {
		fans_print_spd(spd, buf, sizeof (buf));
		if (units && !spd->spd.mach) {
			put_str_with_units(box, row, col, align_right,
			    color, font, "KT", "%s", buf);
		} else {
			fans_put_str(box, row, col, align_right,
			    color, font, "%s", buf);
		}
	} else {
		fans_put_str(box, row, col, align_right, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, req ? "___" : "---");
	}
}

void
fans_put_hdg(fans_t *box, int row, int col, bool align_right,
    const fms_hdg_t *hdg, bool req)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(hdg != NULL);

	if (hdg->set) {
		fans_put_str(box, row, col + 2, align_right, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%03d", hdg->hdg);
		fans_put_str(box, row, col, align_right, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "`%c", hdg->tru ? 'T' : 'M');
	} else {
		fans_put_str(box, row, col, align_right, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, req ? "___" : "---");
	}
}

void
fans_put_time(fans_t *box, int row, int col, bool align_right,
    const fms_time_t *usertime, const fms_time_t *autotime, bool req,
    bool colon)
{
	CPDLC_ASSERT(box != NULL);
	DATA_PICK(const fms_time_t *, t, usertime->set, usertime, autotime);

	if (t != NULL && t->set) {
		if (colon) {
			fans_put_str(box, row, col, align_right, color, font,
			    "%02d:%02d", t->hrs, t->mins);
		} else {
			fans_put_str(box, row, col, align_right, color, font,
			    "%02d%02dZ", t->hrs, t->mins);
		}
	} else {
		fans_put_str(box, row, col, align_right, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, "%s%s%s", req ? "__" : "--",
		    colon ? ":" : "", req ? "__" : "--");
	}
}

void
fans_put_temp(fans_t *box, int row, int col, bool align_right,
    const fms_temp_t *usertemp, const fms_temp_t *autotemp, bool req)
{
	CPDLC_ASSERT(box != NULL);
	DATA_PICK(const fms_temp_t *, temp, usertemp->set, usertemp, autotemp);
	if (temp != NULL && temp->set) {
		fans_put_str(box, row, col + 2, align_right, color,
		    font, "%+d", temp->temp);
		fans_put_str(box, row, col, align_right, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "`C");
	} else {
		fans_put_str(box, row, col, align_right, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, req ? "___" : "---");
	}
}

void
fans_put_pos(fans_t *box, int row, int col, bool align_right,
    const fms_pos_t *userpos, const fms_pos_t *autopos, bool req)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(userpos != NULL);

	if (userpos->set) {
		char buf[16];
		fans_print_pos(userpos, buf, sizeof (buf), POS_PRINT_COMPACT);
		fans_put_str(box, row, col, align_right, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", buf);
	} else if (autopos != NULL && autopos->set) {
		char buf[16];
		fans_print_pos(autopos, buf, sizeof (buf), POS_PRINT_COMPACT);
		fans_put_str(box, row, col, align_right, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "<POS");
		fans_put_str(box, row, col + 5, align_right, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", buf);
	} else {
		fans_put_str(box, row, col, align_right, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "<POS");
		fans_put_str(box, row, col + 5, align_right, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, req ? "_________" : "---------");
	}
}

void
fans_put_off(fans_t *box, int row, int col, bool align_right,
    const fms_off_t *useroff, const fms_off_t *autooff, bool req)
{
	CPDLC_ASSERT(box != NULL);
	DATA_PICK(const fms_off_t *, off, useroff->nm != 0, useroff, autooff);

	if (off != NULL && off->nm != 0) {
		CPDLC_ASSERT(off->dir == CPDLC_DIR_LEFT ||
		    off->dir == CPDLC_DIR_RIGHT);
		fans_put_str(box, row, col, align_right, color, font, "%c%.0f",
		    off->dir == CPDLC_DIR_LEFT ? 'L' : 'R', off->nm);
	} else {
		fans_put_str(box, row, col, align_right,
		    FMS_COLOR_CYAN, FMS_FONT_LARGE, req ? "____" : "----");
	}
}

void
fans_put_wind(fans_t *box, int row, int col, bool align_right,
    const fms_wind_t *userwind, const fms_wind_t *autowind, bool req)
{
	CPDLC_ASSERT(box != NULL);
	DATA_PICK(const fms_wind_t *, wind, userwind->set, userwind, autowind);

	if (wind != NULL && wind->set) {
		fans_put_str(box, row, col, align_right, color, font,
		    "%03d %d", wind->deg, wind->spd);
		fans_put_str(box, row, col + 3, align_right, FMS_COLOR_CYAN,
		    font, "/");
	} else {
		fans_put_str(box, row, col, align_right, FMS_COLOR_CYAN,
		    FMS_FONT_LARGE, req ? "___/___" : "---/---");
	}
}

static void
clear_screen(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	for (unsigned row = 0; row < FMS_ROWS; row++) {
		for (unsigned col = 0; col < FMS_COLS; col++) {
			box->scr[row][col].c = ' ';
			box->scr[row][col].color = FMS_COLOR_WHITE;
			box->scr[row][col].size = FMS_FONT_SMALL;
		}
	}
}

static void
update_error_msg(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	fans_put_str(box, ERROR_MSG_ROW, 0, false, FMS_COLOR_AMBER,
	    FMS_FONT_LARGE, "%s", box->error_msg);
}

fans_t *
fans_alloc(const fans_funcs_t *funcs, void *userinfo)
{
	fans_t *box = safe_calloc(1, sizeof (*box));

	if (funcs != NULL)
		memcpy(&box->funcs, funcs, sizeof (*funcs));
	box->userinfo = userinfo;
	box->cl = cpdlc_client_alloc(false);
	CPDLC_ASSERT(box->cl != NULL);
	cpdlc_strlcpy(box->hostname, "localhost", sizeof (box->hostname));
	box->msglist = cpdlc_msglist_alloc(box->cl);
	fans_set_page(box, FMS_PAGE_MAIN_MENU, true);
	box->thr_id = CPDLC_NO_MSG_THR_ID;
	box->volume = 1;

	fans_update(box);

	return (box);
}

void
fans_free(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(box->msglist != NULL);
	CPDLC_ASSERT(box->cl != NULL);

	if (box->verify.msg != NULL)
		cpdlc_msg_free(box->verify.msg);
	cpdlc_msglist_free(box->msglist);
	cpdlc_client_free(box->cl);
	free(box);
}

cpdlc_client_t *
fans_get_client(const fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->cl);
}

cpdlc_msglist_t *
fans_get_msglist(const fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->msglist);
}

fans_network_t
fans_get_network(const fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->net);
}

void
fans_set_network(fans_t *box, fans_network_t net)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(net <= FANS_NETWORK_PILOTEDGE);
	box->net = net;
}

const fms_char_t *
fans_get_screen_row(const fans_t *box, unsigned row)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT3U(row, <, FMS_ROWS);
	return (box->scr[row]);
}

static void
del_key(fans_t *box)
{
	unsigned l;

	CPDLC_ASSERT(box != NULL);

	if (box->error_msg[0] != '\0') {
		if (strlen(box->error_msg) > ERROR_MSG_MAX) {
			memmove(box->error_msg,
			    &box->error_msg[ERROR_MSG_MAX],
			    strlen(&box->error_msg[ERROR_MSG_MAX]) + 1);
		} else {
			box->error_msg[0] = '\0';
		}
		return;
	}
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
fans_push_key(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(box->page != NULL);
	CPDLC_ASSERT(box->page->key_cb != NULL);

	/* Clear any errors on a new LSK entry */
	if (key >= FMS_KEY_LSK_L1 && key <= FMS_KEY_LSK_R6 &&
	    strlen(box->scratchpad) != 0)
		fans_set_error(box, NULL);
	if (!box->page->key_cb(box, key)) {
		if (key == FMS_KEY_CLR_DEL) {
			del_key(box);
		} else if (key == FMS_KEY_CLR_DEL_LONG) {
			fans_scratchpad_clear(box);
		} else if (key == FMS_KEY_PLUS_MINUS) {
			fans_scratchpad_pm(box);
		} else if (key == FMS_KEY_IDX) {
			fans_set_page(box, FMS_PAGE_MAIN_MENU, true);
		} else if (key == FMS_KEY_NEXT && box->num_subpages > 0) {
			box->subpage = (box->subpage + 1) % box->num_subpages;
		} else if (key == FMS_KEY_PREV && box->num_subpages > 0) {
			if (box->subpage > 0)
				box->subpage--;
			else
				box->subpage = box->num_subpages - 1;
		} else if (key == FMS_KEY_LSK_R6) {
			handle_atc_msg_lsk(box);
		} else if (key == FMS_KEY_LSK_L6) {
			fans_set_page(box, FMS_PAGE_MAIN_MENU, true);
		}
	}
	fans_update(box);
}

void
fans_push_char(fans_t *box, char c)
{
	unsigned len;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(c != 0);

	if (strcmp(box->scratchpad, "DELETE") == 0)
		memset(box->scratchpad, 0, sizeof (box->scratchpad));

	len = strlen(box->scratchpad);
	if (len + 1 < sizeof (box->scratchpad))
		box->scratchpad[len] = toupper(c);

	fans_update(box);
}

static void
update_cda(fans_t *box)
{
	cpdlc_logon_status_t st;

	CPDLC_ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);
	if (st == CPDLC_LOGON_COMPLETE) {
		char cda[8];
		cpdlc_client_get_cda(box->cl, cda, sizeof (cda));
		cpdlc_strlcpy(box->to, cda, sizeof (box->to));
	}
}

void
fans_update(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(box->page != NULL);

	cpdlc_msglist_update(box->msglist);
	clear_screen(box);
	CPDLC_ASSERT(box->page->draw_cb != NULL);
	box->page->draw_cb(box);
	draw_atc_msg_lsk(box);
	if (box->page->has_return) {
		fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE,
		    "<RETURN");
	}
	fans_put_atc_status(box);
	put_cur_time(box);
	fans_update_scratchpad(box);
	update_error_msg(box);
	update_cda(box);
}

void
fans_put_page_ind(fans_t *box, fms_color_t color)
{
	CPDLC_ASSERT(box->num_subpages != 0);
	fans_put_str(box, 0, 0, true, color, FMS_FONT_LARGE, "%d/%d",
	    box->subpage + 1, box->num_subpages);
}

void
fans_put_atc_status(fans_t *box)
{
	cpdlc_logon_status_t st;

	CPDLC_ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);

	if (st != CPDLC_LOGON_COMPLETE) {
		fans_put_str(box, LSK_HEADER_ROW(LSK6_ROW), 6, false,
		    FMS_COLOR_WHITE, FMS_FONT_SMALL, "NO ATC COMM");
	}
}

static void
put_cur_time(fans_t *box)
{
	int t;

	CPDLC_ASSERT(box != NULL);

	if (box->funcs.get_time != NULL) {
		unsigned hours, mins;
		box->funcs.get_time(box->userinfo, &hours, &mins);
		t = hours * 100 + mins;
	} else {
		time_t now = time(NULL);
		const struct tm *tm = gmtime(&now);
		t = tm->tm_hour * 100 + tm->tm_min;
	}
	fans_put_str(box, LSK6_ROW, 8, false, FMS_COLOR_GREEN,
	    FMS_FONT_SMALL, "%04dZ", t);
}

cpdlc_msg_thr_id_t *
fans_get_thr_ids(fans_t *box, unsigned *num_thr_ids, bool ignore_closed)
{
	cpdlc_msg_thr_id_t *thr_ids;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(num_thr_ids != NULL);

	*num_thr_ids = 0;
	cpdlc_msglist_get_thr_ids(box->msglist, ignore_closed, NULL,
	    num_thr_ids);
	thr_ids = safe_calloc(*num_thr_ids, sizeof (*thr_ids));
	cpdlc_msglist_get_thr_ids(box->msglist, ignore_closed, thr_ids,
	    num_thr_ids);

	return (thr_ids);
}

const char *
fans_thr_status2str(cpdlc_msg_thr_status_t st, bool dirty)
{
	if (dirty)
		return ("NEW");

	switch (st) {
	case CPDLC_MSG_THR_OPEN:
		return ("OPEN");
	case CPDLC_MSG_THR_CLOSED:
		return ("CLOSED");
	case CPDLC_MSG_THR_ACCEPTED:
		return ("ACPT");
	case CPDLC_MSG_THR_REJECTED:
		return ("REJ");
	case CPDLC_MSG_THR_TIMEDOUT:
		return ("TIMEDOUT");
	case CPDLC_MSG_THR_STANDBY:
		return ("STBY");
	case CPDLC_MSG_THR_FAILED:
		return ("FAILED");
	case CPDLC_MSG_THR_PENDING:
		return ("PENDING");
	case CPDLC_MSG_THR_DISREGARD:
		return ("DISREGARD");
	case CPDLC_MSG_THR_ERROR:
		return ("ERROR");
	case CPDLC_MSG_THR_CONN_ENDED:
		return ("CONN ENDED");
	default:
		CPDLC_VERIFY_MSG(0, "Invalid thread status %x", st);
	}
}

static cpdlc_msg_thr_id_t
get_new_thr_id(fans_t *box)
{
	unsigned num_thr_ids;
	cpdlc_msg_thr_id_t *thr_ids;
	cpdlc_msg_thr_id_t thr_id = CPDLC_NO_MSG_THR_ID;

	CPDLC_ASSERT(box != NULL);

	thr_ids = fans_get_thr_ids(box, &num_thr_ids, true);
	for (unsigned i = 0; i < num_thr_ids; i++) {
		bool dirty;

		cpdlc_msglist_get_thr_status(box->msglist, thr_ids[i], &dirty);
		if (dirty) {
			thr_id = thr_ids[i];
			break;
		}
	}
	free(thr_ids);

	return (thr_id);
}

static void
draw_atc_msg_lsk(fans_t *box)
{
	cpdlc_msg_thr_id_t thr_id;

	CPDLC_ASSERT(box != NULL);
	thr_id = get_new_thr_id(box);
	if (thr_id != CPDLC_NO_MSG_THR_ID) {
		fans_put_lsk_action(box, FMS_KEY_LSK_R6, FMS_COLOR_CYAN,
		    "ATC MSG*");
	}
}

static void
handle_atc_msg_lsk(fans_t *box)
{
	cpdlc_msg_thr_id_t thr_id;

	CPDLC_ASSERT(box != NULL);
	thr_id = get_new_thr_id(box);
	if (thr_id != CPDLC_NO_MSG_THR_ID) {
		fans_set_thr_id(box, thr_id);
		fans_set_page(box, FMS_PAGE_MSG_THR, true);
	}
}

void
fans_put_step_at(fans_t *box, const fms_step_at_t *step_at)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(step_at != NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_R1, "STEP AT");
	switch (step_at->type) {
	case STEP_AT_NONE:
		fans_put_str(box, LSK1_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "NONEv");
		break;
	case STEP_AT_TIME:
		fans_put_str(box, LSK1_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "TIMEv");
		fans_put_lsk_title(box, FMS_KEY_LSK_R2, "TIME");
		fans_put_time(box, LSK2_ROW, 0, true, &step_at->tim,
		    NULL, true, false);
		break;
	case STEP_AT_POS:
		fans_put_str(box, LSK1_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "POSv");
		fans_put_lsk_title(box, FMS_KEY_LSK_R2, "POS");
		if (strlen(step_at->pos) != 0) {
			fans_put_str(box, LSK2_ROW, 0, true, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "%s", step_at->pos);
		} else {
			fans_put_str(box, LSK2_ROW, 0, true, FMS_COLOR_CYAN,
			    FMS_FONT_LARGE, "_____");
		}
		break;
	default:
		CPDLC_VERIFY(0);
	}
}

void
fans_key_step_at(fans_t *box, fms_key_t key, fms_step_at_t *step_at)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(step_at != NULL);

	if (key == FMS_KEY_LSK_R1) {
		step_at->type = (step_at->type + 1) % NUM_STEP_AT_TYPES;
	} else if (step_at->type != STEP_AT_NONE) {
		CPDLC_ASSERT3U(key, ==, FMS_KEY_LSK_R2);

		if (step_at->type == STEP_AT_TIME) {
			fans_scratchpad_xfer_time(box, &step_at->tim, NULL);
		} else {
			fans_scratchpad_xfer(box, step_at->pos,
			    sizeof (step_at->pos), true);
		}
	}
}

bool
fans_step_at_can_send(const fms_step_at_t *step_at)
{
	CPDLC_ASSERT(step_at != NULL);
	return (step_at->type == STEP_AT_NONE ||
	    (step_at->type == STEP_AT_TIME && step_at->tim.set) ||
	    (step_at->type == STEP_AT_POS && strlen(step_at->pos) != 0));
}

void
fans_get_cur_spd(const fans_t *box, cpdlc_arg_t *spd)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(spd != NULL);
	memset(spd, 0, sizeof (*spd));
	if (box->funcs.get_cur_spd != NULL && box->funcs.get_cur_spd(
	    box->userinfo, &spd->spd.mach, &spd->spd.spd)) {
		spd->spd.spd = MIN(spd->spd.spd, 999);
	}
}

int
fans_get_cur_alt(const fans_t *box)
{
	int alt = 0;
	CPDLC_ASSERT(box != NULL);
	if (box->funcs.get_cur_alt != NULL)
		alt = box->funcs.get_cur_alt(box->userinfo);
	return (MIN(MAX(alt, 0), MAX_ALT));
}

int
fans_get_sel_alt(const fans_t *box)
{
	int alt = 0;
	CPDLC_ASSERT(box != NULL);
	if (box->funcs.get_sel_alt != NULL)
		alt = box->funcs.get_sel_alt(box->userinfo);
	return (MIN(MAX(alt, 0), MAX_ALT));
}

bool
get_wpt_common(const fans_t *box, fans_get_wpt_info_t func,
    fms_wpt_info_t *info)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(info != NULL);
	memset(info, 0, sizeof (*info));
	if (func != NULL) {
		bool res = func(box->userinfo, info);
		if (res) {
			if (strlen(info->wpt_name) == 0)
				return (false);
			info->hrs = MIN(info->hrs, 23);
			info->mins = MIN(info->mins, 60);
			info->alt_ft = MAX(MIN(info->alt_ft, 60000), -2000);
			info->spd = MIN(info->spd, 999);
		}
		return (res);
	}
	return (false);
}

bool
fans_get_prev_wpt(const fans_t *box, fms_wpt_info_t *info)
{
	return (get_wpt_common(box, box->funcs.get_prev_wpt, info));
}

bool
fans_get_next_wpt(const fans_t *box, fms_wpt_info_t *info)
{
	return (get_wpt_common(box, box->funcs.get_next_wpt, info));
}

bool
fans_get_next_next_wpt(const fans_t *box, fms_wpt_info_t *info)
{
	return (get_wpt_common(box, box->funcs.get_next_next_wpt, info));
}

bool
fans_get_dest_wpt(const fans_t *box, fms_wpt_info_t *info)
{
	return (get_wpt_common(box, box->funcs.get_dest_wpt, info));
}

int
fans_get_offset(const fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	if (box->funcs.get_offset != NULL)
		return (box->funcs.get_offset(box->userinfo));
	return (0);
}

bool
fans_get_fuel(const fans_t *box, unsigned *hrs, unsigned *mins)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(hrs != NULL);
	CPDLC_ASSERT(mins != NULL);
	if (box->funcs.get_fuel != NULL)
		return (box->funcs.get_fuel(box->userinfo, hrs, mins));
	return (false);
}

void
fans_get_sat(const fans_t *box, fms_temp_t *temp)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(temp != NULL);
	if (box->funcs.get_sat != NULL)
		temp->set = box->funcs.get_sat(box->userinfo, &temp->temp);
	else
		temp->set = false;
}

void
fans_get_wind(const fans_t *box, fms_wind_t *wind)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(wind != NULL);
	memset(wind, 0, sizeof (*wind));
	if (box->funcs.get_wind != NULL &&
	    box->funcs.get_wind(box->userinfo, &wind->deg, &wind->spd)) {
		wind->set = true;
		wind->deg %= 360;
		wind->spd = MIN(wind->spd, MAX_WIND);
	}
}

void
fans_wptinfo2pos(const fms_wpt_info_t *info, fms_pos_t *pos)
{
	CPDLC_ASSERT(info != NULL);
	CPDLC_ASSERT(pos != NULL);

	memset(pos, 0, sizeof (*pos));
	pos->set = true;
	pos->type = FMS_POS_FIX;
	cpdlc_strlcpy(pos->name, info->wpt_name, sizeof (pos->name));
}

void
fans_wptinfo2time(const fms_wpt_info_t *info, fms_time_t *tim)
{
	CPDLC_ASSERT(info != NULL);
	CPDLC_ASSERT(tim != NULL);

	memset(tim, 0, sizeof (*tim));
	if (info->time_set) {
		tim->set = true;
		tim->hrs = info->hrs;
		tim->mins = info->mins;
	}
}

void
fans_wptinfo2alt(const fms_wpt_info_t *info, cpdlc_arg_t *alt)
{
	CPDLC_ASSERT(info != NULL);
	CPDLC_ASSERT(alt != NULL);

	memset(alt, 0, sizeof (*alt));
	if (info->alt_set) {
		alt->alt.fl = info->alt_fl;
		alt->alt.alt = info->alt_ft;
	}
}

void
fans_wptinfo2spd(const fms_wpt_info_t *info, cpdlc_arg_t *spd)
{
	CPDLC_ASSERT(info != NULL);
	CPDLC_ASSERT(spd != NULL);

	memset(spd, 0, sizeof (*spd));
	if (info->spd_set) {
		spd->spd.mach = info->spd_mach;
		spd->spd.spd = info->spd;
	}
}

void
fans_set_host(fans_t *box, const char *hostname)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(hostname != NULL);
	cpdlc_strlcpy(box->hostname, hostname, sizeof (box->hostname));
}

const char *
fans_get_host(const fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->hostname);
}

void
fans_set_port(fans_t *box, int port)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(port >= 0);
	CPDLC_ASSERT(port <= UINT16_MAX);
	box->port = port;
}

int
fans_get_port(const fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->port);
}

void
fans_set_shows_volume(fans_t *box, bool flag)
{
	CPDLC_ASSERT(box != NULL);
	box->show_volume = flag;
}

bool
fans_get_shows_volume(const fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->show_volume);
}

void
fans_set_volume(fans_t *box, double volume)
{
	CPDLC_ASSERT(box != NULL);
	box->volume = MIN(MAX(volume, 0.0), 1.0);
}

double
fans_get_volume(const fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->volume);
}
