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
#define	ERROR_MSG_MAX	19
#define	ERROR_MSG_ROW	14
#define	LSK1_ROW	2
#define	LSK2_ROW	4
#define	LSK3_ROW	6
#define	LSK4_ROW	8
#define	LSK5_ROW	10
#define	LSK6_ROW	12
#define	LSKi_ROW(_idx)	((_idx) * 2 + 2)
#define	LSK_HEADER_ROW(x)	((x) - 1)
#define	MAX_FREETEXT_LINES	8

typedef struct {
	void	(*draw_cb)(fmsbox_t *box);
	bool	(*key_cb)(fmsbox_t *box, fms_key_t key);
	bool	has_return;
} fms_page_t;

struct fmsbox_s {
	fmsbox_char_t	scr[FMSBOX_ROWS][FMSBOX_COLS];
	fms_page_t	*page;
	unsigned	subpage;
	unsigned	num_subpages;
	char		scratchpad[SCRATCHPAD_MAX + 1];
	char		error_msg[ERROR_MSG_MAX + 1];

	char			flt_id[12];
	char			to[8];

	cpdlc_msg_thr_id_t	thr_id;
	bool			msg_log_open;
	char			freetext[MAX_FREETEXT_LINES][FMSBOX_COLS + 1];

	struct {
		cpdlc_arg_t	alt[2];
		bool		due_wx;
		bool		due_ac;
		char		step_at[8];
		bool		step_at_time;
		int		step_hrs;
		int		step_mins;
		bool		plt_discret;
		bool		maint_sep_vmc;
	} alt_req;

	struct {
		cpdlc_msg_t	*msg;
		char		title[8];
		int		ret_page;
	} verify;

	cpdlc_client_t	*cl;
	cpdlc_msglist_t	*msglist;
};

enum {
	FMS_PAGE_MAIN_MENU,
	FMS_PAGE_LOGON_STATUS,
	FMS_PAGE_MSG_LOG,
	FMS_PAGE_MSG_THR,
	FMS_PAGE_FREETEXT,
	FMS_PAGE_REQUESTS,
	FMS_PAGE_REQ_ALT,
#if 0
	FMS_PAGE_REQ_OFF,
	FMS_PAGE_REQ_SPD,
	FMS_PAGE_REQ_RTE,
#endif
	FMS_PAGE_VRFY,
	FMS_NUM_PAGES
};

static void draw_atc_msg_lsk(fmsbox_t *box);
static void handle_atc_msg_lsk(fmsbox_t *box);
static void put_cur_time(fmsbox_t *box);

static void main_menu_draw_cb(fmsbox_t *box);
static bool main_menu_key_cb(fmsbox_t *box, fms_key_t key);
static void logon_status_draw_cb(fmsbox_t *box);
static bool logon_status_key_cb(fmsbox_t *box, fms_key_t key);
static void msg_log_draw_cb(fmsbox_t *box);
static bool msg_log_key_cb(fmsbox_t *box, fms_key_t key);
static void msg_thr_draw_cb(fmsbox_t *box);
static bool msg_thr_key_cb(fmsbox_t *box, fms_key_t key);
static void freetext_draw_cb(fmsbox_t *box);
static bool freetext_key_cb(fmsbox_t *box, fms_key_t key);
static void requests_draw_cb(fmsbox_t *box);
static bool requests_key_cb(fmsbox_t *box, fms_key_t key);
static void req_alt_draw_cb(fmsbox_t *box);
static bool req_alt_key_cb(fmsbox_t *box, fms_key_t key);
#if 0
static void req_off_draw_cb(fmsbox_t *box);
static bool req_off_key_cb(fmsbox_t *box, fms_key_t key);
static void req_spd_draw_cb(fmsbox_t *box);
static bool req_spd_key_cb(fmsbox_t *box, fms_key_t key);
static void req_rte_draw_cb(fmsbox_t *box);
static bool req_rte_key_cb(fmsbox_t *box, fms_key_t key);
#endif
static void vrfy_draw_cb(fmsbox_t *box);
static bool vrfy_key_cb(fmsbox_t *box, fms_key_t key);

static fms_page_t fms_pages[FMS_NUM_PAGES] = {
	{	/* FMS_PAGE_MAIN_MENU */
		.draw_cb = main_menu_draw_cb,
		.key_cb = main_menu_key_cb
	},
	{	/* FMS_PAGE_LOGON_STATUS */
		.draw_cb = logon_status_draw_cb,
		.key_cb = logon_status_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_MSG_LOG */
		.draw_cb = msg_log_draw_cb,
		.key_cb = msg_log_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_MSG_THR */
		.draw_cb = msg_thr_draw_cb,
		.key_cb = msg_thr_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_MSG_THR */
		.draw_cb = freetext_draw_cb,
		.key_cb = freetext_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_REQUESTS */
		.draw_cb = requests_draw_cb,
		.key_cb = requests_key_cb,
		.has_return = true
	},
	{	/* FMS_PAGE_REQ_ALT */
		.draw_cb = req_alt_draw_cb,
		.key_cb = req_alt_key_cb
	},
#if 0
	{	/* FMS_PAGE_REQ_OFF */
		.draw_cb = req_off_draw_cb,
		.key_cb = req_off_key_cb
	},
	{	/* FMS_PAGE_REQ_SPD */
		.draw_cb = req_spd_draw_cb,
		.key_cb = req_spd_key_cb
	},
	{	/* FMS_PAGE_REQ_RTE */
		.draw_cb = req_rte_draw_cb,
		.key_cb = req_rte_key_cb
	}
#endif
	{	/* FMS_PAGE_VRFY */
		.draw_cb = vrfy_draw_cb,
		.key_cb = vrfy_key_cb
	}
};

static void put_str(fmsbox_t *box, unsigned row, unsigned col,
    bool align_right, fms_color_t color, fms_font_t size,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(7);

static void
set_thr_id(fmsbox_t *box, cpdlc_msg_thr_id_t thr_id)
{
	ASSERT(box != NULL);
	ASSERT(thr_id != CPDLC_NO_MSG_THR_ID);

	box->thr_id = thr_id;
	cpdlc_msglist_thr_mark_seen(box->msglist, thr_id);
}

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
	return (box->flt_id[0] != '\0' && box->to[0] != '\0' &&
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
lsk2row(int lsk_key_id, int *row, bool *align_right)
{
	ASSERT(row != NULL);
	ASSERT(align_right);
	if (lsk_key_id >= FMS_KEY_LSK_L1 && lsk_key_id <= FMS_KEY_LSK_L6) {
		*row = 2 * (lsk_key_id - FMS_KEY_LSK_L1) + 2;
		*align_right = false;
	} else if (lsk_key_id >= FMS_KEY_LSK_R1 &&
	    lsk_key_id <= FMS_KEY_LSK_R6) {
		*row = 2 * (lsk_key_id - FMS_KEY_LSK_R1) + 2;
		*align_right = true;
	} else {
		VERIFY_MSG(0, "Invalid lsk_key_id = %x", lsk_key_id);
	}
}

static void
put_lsk_action(fmsbox_t *box, int lsk_key_id, fms_color_t color,
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

static void
put_lsk_title(fmsbox_t *box, int lsk_key_id, const char *fmt, ...)
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
update_error_msg(fmsbox_t *box)
{
	ASSERT(box != NULL);
	put_str(box, ERROR_MSG_ROW, 0, false, FMS_COLOR_AMBER,
	    FMS_FONT_LARGE, "%s", box->error_msg);
}

static void
scratchpad_xfer(fmsbox_t *box, char *dest, size_t cap, bool allow_mod)
{
	if (!allow_mod) {
		if (box->scratchpad[0] == '\0') {
			cpdlc_strlcpy(box->scratchpad, dest,
			    sizeof (box->scratchpad));
		}
		return;
	}
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

#define	READ_FUNC_BUF_SZ	(FMSBOX_COLS + 1)
typedef const char *(*parse_func_t)(const char *str, unsigned field_nr,
    void *data);
typedef const char *(*insert_func_t)(fmsbox_t *box, unsigned field_nr,
    void *data, void *userinfo);
typedef const char *(*delete_func_t)(fmsbox_t *box, void *userinfo);
typedef void (*read_func_t)(fmsbox_t *box, void *userinfo,
    char str[READ_FUNC_BUF_SZ]);

static void
scratchpad_parse_multipart(fmsbox_t *box, void *userinfo, size_t buf_sz,
    parse_func_t parse_func, insert_func_t insert_func,
    delete_func_t delete_func, read_func_t read_func)
{
	const char *error = NULL;

	ASSERT(box != NULL);
	ASSERT(buf_sz != 0);
	ASSERT(parse_func != NULL);
	ASSERT(insert_func != NULL);
	ASSERT(delete_func != NULL);

	if (strlen(box->scratchpad) == 0) {
		if (read_func != NULL) {
			char str[READ_FUNC_BUF_SZ] = { 0 };

			read_func(box, userinfo, str);
			if (strlen(str) == 0) {
				error = "NO DATA";
			} else {
				cpdlc_strlcpy(box->scratchpad, str,
				    sizeof (box->scratchpad));
			}
		}
	} else if (strcmp(box->scratchpad, "DELETE") == 0) {
		error = delete_func(box, userinfo);
		memset(box->scratchpad, 0, sizeof (box->scratchpad));
	} else {
		const char *start = box->scratchpad;
		const char *end = start + strlen(start);
		void *data_buf = safe_malloc(buf_sz);

		for (unsigned field_nr = 0; start < end; field_nr++) {
			char substr[sizeof (box->scratchpad) + 1];
			const char *sep = strchr(start, '/');

			if (sep == NULL)
				sep = end;
			cpdlc_strlcpy(substr, start, (sep - start) + 1);
			start = sep + 1;

			if (strlen(substr) == 0)
				continue;
			memset(data_buf, 0, buf_sz);
			error = parse_func(substr, field_nr, data_buf);
			if (error != NULL)
				break;
			error = insert_func(box, field_nr, data_buf, userinfo);
			if (error != NULL)
				break;
		}
		memset(box->scratchpad, 0, sizeof (box->scratchpad));
		free(data_buf);
	}

	if (error != NULL)
		cpdlc_strlcpy(box->error_msg, error, sizeof (box->error_msg));
}

static bool
parse_time(const char *buf, int *hrs_p, int *mins_p)
{
	int num, hrs, mins;

	ASSERT(buf != NULL);

	if (strlen(buf) != 4 || !isdigit(buf[0]) || !isdigit(buf[1]) ||
	    !isdigit(buf[2]) || !isdigit(buf[3]) ||
	    sscanf(buf, "%d", &num) != 1) {
		return (false);
	}
	hrs = num / 100;
	mins = num % 100;
	if (hrs_p != NULL)
		*hrs_p = hrs;
	if (mins_p != NULL)
		*mins_p = mins;
	return (hrs >= 0 && hrs <= 23 && mins >= 0 && mins <= 59);
}

static const char *
parse_alt(const char *str, unsigned field_nr, void *data)
{
	cpdlc_arg_t *arg;

	ASSERT(str != NULL);
	UNUSED(field_nr);
	ASSERT(data != NULL);
	arg = data;

	if (strlen(str) < 3)
		goto errout;
	if (str[0] == 'F' && str[1] == 'L') {
		if (sscanf(&str[2], "%d", &arg->alt.alt) != 1)
			goto errout;
		arg->alt.fl = true;
		arg->alt.alt *= 100;
	} else if (str[0] == 'F') {
		if (sscanf(&str[1], "%d", &arg->alt.alt) != 1)
			goto errout;
		arg->alt.fl = true;
		arg->alt.alt *= 100;
	} else {
		if (sscanf(str, "%d", &arg->alt.alt) != 1)
			goto errout;
		/* Round to nearest 100 ft */
		arg->alt.alt = round(arg->alt.alt / 100.0) * 100;
	}
	if (arg->alt.alt <= 0 || arg->alt.alt > 60000)
		goto errout;

	return (NULL);
errout:
	return ("BAD ALTITUDE");
}

static const char *
insert_alt_block(fmsbox_t *box, unsigned field_nr, void *data, void *userinfo)
{
	cpdlc_arg_t *inarg, *outarg;
	size_t offset;

	ASSERT(box != NULL);
	ASSERT(data != NULL);
	inarg = data;
	offset = (uintptr_t)userinfo;
	ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;

	if (field_nr >= 2)
		goto errout;
	if (field_nr == 0) {
		memcpy(outarg, inarg, sizeof (*inarg));
		memset(&outarg[1], 0, sizeof (*inarg));
	} else {
		if (/* lower alt must actually be set */
		    outarg[0].alt.alt == 0 ||
		    /* lower alt must really be lower */
		    outarg[0].alt.alt >= inarg->alt.alt ||
		    /* if lower alt is FL, higher must as well */
		    (outarg[0].alt.fl && !inarg->alt.fl)) {
			goto errout;
		}
		memcpy(&outarg[1], inarg, sizeof (*inarg));
	}

	return (NULL);
errout:
	return ("BAD INSERT");
}

static const char *
delete_alt_block(fmsbox_t *box, void *userinfo)
{
	cpdlc_arg_t *outarg;
	size_t offset;

	ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;
	memset(outarg, 0, 2 * sizeof (cpdlc_arg_t));

	return (NULL);
}

static int
print_alt(const cpdlc_arg_t *arg, char *str, size_t cap)
{
	ASSERT(arg != NULL);
	if (arg->alt.fl)
		return (snprintf(str, cap, "FL%d", arg->alt.alt / 100));
	return (snprintf(str, cap, "%d", arg->alt.alt));
}

static void
read_alt_block(fmsbox_t *box, void *userinfo, char str[READ_FUNC_BUF_SZ])
{
	cpdlc_arg_t *outarg;
	size_t offset;

	ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;

	if (outarg->alt.alt != 0) {
		int l = print_alt(outarg, str, READ_FUNC_BUF_SZ);
		if (outarg[1].alt.alt != 0) {
			strncat(str, "/", READ_FUNC_BUF_SZ - l - 1);
			l++;
			print_alt(&outarg[1], &str[l], READ_FUNC_BUF_SZ - l);
		}
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
	set_page(box, &fms_pages[FMS_PAGE_MAIN_MENU]);
	box->thr_id = CPDLC_NO_MSG_THR_ID;

	fmsbox_update(box);

	return (box);
}

void
fmsbox_free(fmsbox_t *box)
{
	ASSERT(box != NULL);
	ASSERT(box->msglist != NULL);
	ASSERT(box->cl != NULL);

	if (box->verify.msg != NULL)
		cpdlc_msg_free(box->verify.msg);
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

	memset(box->error_msg, 0, sizeof (box->error_msg));
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
		} else if (key == FMS_KEY_LSK_R6) {
			handle_atc_msg_lsk(box);
		} else if (key == FMS_KEY_LSK_L6) {
			set_page(box, &fms_pages[FMS_PAGE_MAIN_MENU]);
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

	cpdlc_msglist_update(box->msglist);
	clear_screen(box);
	ASSERT(box->page->draw_cb != NULL);
	box->page->draw_cb(box);
	draw_atc_msg_lsk(box);
	if (box->page->has_return)
		put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
	put_cur_time(box);
	update_scratchpad(box);
	update_error_msg(box);
}

static void
put_page_ind(fmsbox_t *box, fms_color_t color)
{
	ASSERT(box->num_subpages != 0);
	put_str(box, 0, 0, true, color, FMS_FONT_LARGE, "%d/%d",
	    box->subpage + 1, box->num_subpages);
}

static void
put_atc_status(fmsbox_t *box)
{
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);

	if (st != CPDLC_LOGON_COMPLETE) {
		put_str(box, LSK_HEADER_ROW(LSK6_ROW), 6, false,
		    FMS_COLOR_WHITE, FMS_FONT_SMALL, "NO ATC COMM");
	}
}

static void
put_cur_time(fmsbox_t *box)
{
	time_t now;
	const struct tm *tm;

	ASSERT(box != NULL);

	now = time(NULL);
	tm = localtime(&now);
	put_str(box, LSK6_ROW, 8, false, FMS_COLOR_CYAN, FMS_FONT_SMALL,
	    "%02d%02dZ", tm->tm_hour, tm->tm_min);
}

static void
send_logon(fmsbox_t *box)
{
	char buf[64];

	ASSERT(box != NULL);

	snprintf(buf, sizeof (buf), "%s", box->flt_id);
	cpdlc_client_logon(box->cl, buf, box->flt_id, box->to);
}

static void
send_logoff(fmsbox_t *box)
{
	cpdlc_client_logoff(box->cl);
}

static cpdlc_msg_thr_id_t *
get_thr_ids(fmsbox_t *box, unsigned *num_thr_ids, bool ignore_closed)
{
	cpdlc_msg_thr_id_t *thr_ids;

	ASSERT(box != NULL);
	ASSERT(num_thr_ids != NULL);

	*num_thr_ids = 0;
	cpdlc_msglist_get_thr_ids(box->msglist, ignore_closed, NULL,
	    num_thr_ids);
	thr_ids = safe_calloc(*num_thr_ids, sizeof (*thr_ids));
	cpdlc_msglist_get_thr_ids(box->msglist, ignore_closed, thr_ids,
	    num_thr_ids);

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
	case CPDLC_MSG_THR_STANDBY:
		return ("STANDBY");
	case CPDLC_MSG_THR_FAILED:
		return ("FAILED");
	case CPDLC_MSG_THR_PENDING:
		return ("PENDING");
	case CPDLC_MSG_THR_DISREGARD:
		return ("DISREGARD");
	case CPDLC_MSG_THR_ERROR:
		return ("ERROR");
	default:
		VERIFY_MSG(0, "Invalid thread status %x", st);
	}
}

static cpdlc_msg_thr_id_t
get_new_thr_id(fmsbox_t *box)
{
	unsigned num_thr_ids;
	cpdlc_msg_thr_id_t *thr_ids;
	cpdlc_msg_thr_id_t thr_id = CPDLC_NO_MSG_THR_ID;

	ASSERT(box != NULL);

	thr_ids = get_thr_ids(box, &num_thr_ids, true);
	for (unsigned i = 0; i < num_thr_ids; i++) {
		cpdlc_msg_thr_status_t st =
		    cpdlc_msglist_get_thr_status(box->msglist, thr_ids[i]);

		if (st == CPDLC_MSG_THR_NEW) {
			thr_id = thr_ids[i];
			break;
		}
	}
	free(thr_ids);

	return (thr_id);
}

static void
draw_atc_msg_lsk(fmsbox_t *box)
{
	cpdlc_msg_thr_id_t thr_id;
	cpdlc_logon_status_t logon;

	ASSERT(box != NULL);
	logon = cpdlc_client_get_logon_status(box->cl, NULL);
	thr_id = get_new_thr_id(box);
	if (logon == CPDLC_LOGON_COMPLETE && thr_id != CPDLC_NO_MSG_THR_ID)
		put_lsk_action(box, FMS_KEY_LSK_R6, FMS_COLOR_CYAN, "ATC MSG*");
}

static void
handle_atc_msg_lsk(fmsbox_t *box)
{
	cpdlc_msg_thr_id_t thr_id;
	cpdlc_logon_status_t logon;

	ASSERT(box != NULL);
	logon = cpdlc_client_get_logon_status(box->cl, NULL);
	thr_id = get_new_thr_id(box);
	if (logon == CPDLC_LOGON_COMPLETE && thr_id != CPDLC_NO_MSG_THR_ID) {
		set_thr_id(box, thr_id);
		set_page(box, &fms_pages[FMS_PAGE_MSG_THR]);
	}
}

static void
main_menu_draw_cb(fmsbox_t *box)
{
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);

	put_page_title(box, "FANS  MAIN MENU");
	put_lsk_action(box, FMS_KEY_LSK_L1, FMS_COLOR_WHITE, "<LOGON/STATUS");
	if (st == CPDLC_LOGON_COMPLETE) {
		put_lsk_action(box, FMS_KEY_LSK_R1, FMS_COLOR_WHITE,
		    "MSG LOG>");
		put_lsk_action(box, FMS_KEY_LSK_L2, FMS_COLOR_WHITE,
		    "<REQUESTS");
		put_lsk_action(box, FMS_KEY_LSK_R2, FMS_COLOR_WHITE,
		    "EMERGENCY>");
		put_lsk_action(box, FMS_KEY_LSK_L3, FMS_COLOR_WHITE,
		    "<POS REP");
		put_lsk_action(box, FMS_KEY_LSK_R3, FMS_COLOR_WHITE,
		    "REPORTS DUE>");
		put_lsk_action(box, FMS_KEY_LSK_L4, FMS_COLOR_WHITE,
		    "<FREE TEXT");
	}

	put_atc_status(box);
}

static bool
main_menu_key_cb(fmsbox_t *box, fms_key_t key)
{
	cpdlc_logon_status_t st;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, NULL);

	if (key == FMS_KEY_LSK_L1)
		set_page(box, &fms_pages[FMS_PAGE_LOGON_STATUS]);
	else if (key == FMS_KEY_LSK_L2)
		set_page(box, &fms_pages[FMS_PAGE_REQUESTS]);
	else if (key == FMS_KEY_LSK_R1 && st == CPDLC_LOGON_COMPLETE)
		set_page(box, &fms_pages[FMS_PAGE_MSG_LOG]);
	else if (key == FMS_KEY_LSK_L4 && st == CPDLC_LOGON_COMPLETE)
		set_page(box, &fms_pages[FMS_PAGE_FREETEXT]);
	else
		return (false);

	return (true);
}

static void
logon_status_draw_cb(fmsbox_t *box)
{
	cpdlc_logon_status_t st;
	bool logon_failed;

	ASSERT(box != NULL);
	st = cpdlc_client_get_logon_status(box->cl, &logon_failed);

	put_page_title(box, "FANS  LOGON/STATUS");
	if (logon_failed) {
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "LOGON FAILED*");
	} else if (can_send_logon(box, st)) {
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "SEND LOGON*");
	} else if (st == CPDLC_LOGON_CONNECTING_LINK ||
	    st == CPDLC_LOGON_HANDSHAKING_LINK || st == CPDLC_LOGON_IN_PROG) {
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_WHITE,
		    "IN PROGRESS");
	} else if (st == CPDLC_LOGON_COMPLETE) {
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "LOG OFF*");
	}

	put_str(box, 1, 1, false, FMS_COLOR_WHITE, FMS_FONT_SMALL, "CDA");
	if (st == CPDLC_LOGON_COMPLETE) {
		put_str(box, 1, 5, false, FMS_COLOR_GREEN, FMS_FONT_SMALL,
		    "%s", box->to);
	} else {
		put_str(box, 1, 5, false, FMS_COLOR_GREEN, FMS_FONT_SMALL,
		    "--------");
	}
	put_str(box, 2, 1, false, FMS_COLOR_WHITE, FMS_FONT_SMALL, "NDA");
	put_str(box, 2, 5, false, FMS_COLOR_GREEN, FMS_FONT_SMALL, "--------");
	put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE, FMS_FONT_LARGE,
	    "------------------------");
	put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, false, FMS_COLOR_WHITE,
	    FMS_FONT_SMALL, "FLT ID");
	if (box->flt_id[0] != '\0') {
		put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", box->flt_id);
	} else {
		put_str(box, LSK4_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "--------");
	}
	put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, true, FMS_COLOR_WHITE,
	    FMS_FONT_SMALL, "LOGON TO");
	if (box->to[0] != '\0') {
		put_str(box, LSK4_ROW, 0, true, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", box->to);
	} else {
		put_str(box, LSK4_ROW, 0, true, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "----");
	}

	if (st == CPDLC_LOGON_COMPLETE) {
		put_str(box, LSK_HEADER_ROW(LSK5_ROW), 5, true,
		    FMS_COLOR_GREEN, FMS_FONT_SMALL, "LOGGED ON TO %s",
		    box->to);
	} else {
		put_str(box, LSK_HEADER_ROW(LSK5_ROW), 5, true,
		    FMS_COLOR_GREEN, FMS_FONT_SMALL, "LOGON REQUIRED");
	}

	put_atc_status(box);
}

static bool
logon_status_key_cb(fmsbox_t *box, fms_key_t key)
{
	cpdlc_logon_status_t st = cpdlc_client_get_logon_status(box->cl, NULL);

	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L4) {
		scratchpad_xfer(box, box->flt_id, sizeof (box->flt_id),
		    st <= CPDLC_LOGON_LINK_AVAIL);
	} else if (key == FMS_KEY_LSK_R4) {
		scratchpad_xfer(box, box->to, sizeof (box->to),
		    st <= CPDLC_LOGON_LINK_AVAIL);
	} else if (key == FMS_KEY_LSK_R5) {
		if (can_send_logon(box, st)) {
			send_logon(box);
		} else if (st == CPDLC_LOGON_COMPLETE) {
			send_logoff(box);
		}
	} else {
		return (false);
	}

	return (true);
}

static void
draw_thr_hdr(fmsbox_t *box, unsigned row, cpdlc_msg_thr_id_t thr_id)
{
	const cpdlc_msg_t *msg;
	cpdlc_msg_thr_status_t st;
	unsigned hours, mins;
	bool sent;
	const char *atsu;

	cpdlc_msglist_get_thr_msg(box->msglist, thr_id, 0, &msg, NULL,
	    &hours, &mins, &sent);
	st = cpdlc_msglist_get_thr_status(box->msglist, thr_id);

	if (sent)
		atsu = cpdlc_msg_get_to(msg);
	else
		atsu = cpdlc_msg_get_from(msg);

	/* Message uplink/downlink, time and sender */
	put_str(box, row, 0, false, FMS_COLOR_GREEN, FMS_FONT_SMALL,
	    "%s %02d:%02d%s%s", sent ? "v" : "^", hours, mins, sent ? "" : "-",
	    sent ? "" : atsu);
	/* Thread status */
	put_str(box, row, 0, true, FMS_COLOR_GREEN, FMS_FONT_SMALL,
	    "%s", thr_status2str(st));
}

static void
msg_log_draw_thr(fmsbox_t *box, cpdlc_msg_thr_id_t thr_id, unsigned row)
{
	const cpdlc_msg_t *msg;
	char buf[64];

	ASSERT(box != NULL);
	ASSERT3U(row, <, 5);

	draw_thr_hdr(box, 2 * row + 1, thr_id);

	cpdlc_msglist_get_thr_msg(box->msglist, thr_id, 0, &msg, NULL, NULL,
	    NULL, NULL);
	cpdlc_msg_readable(msg, buf, sizeof (buf));
	put_str(box, 2 * row + 2, 0, false, FMS_COLOR_WHITE, FMS_FONT_LARGE,
	    "<%s", buf);
}

static void
msg_log_draw_cb(fmsbox_t *box)
{
	enum { MSG_LOG_LINES = 4 };
	cpdlc_msg_thr_id_t *thr_ids;
	unsigned num_thr_ids;

	ASSERT(box != NULL);

	thr_ids = get_thr_ids(box, &num_thr_ids, box->msg_log_open);
	if (num_thr_ids == 0) {
		set_num_subpages(box, 1);
	} else {
		set_num_subpages(box,
		    ceil(num_thr_ids / (double)MSG_LOG_LINES));
	}

	put_page_title(box, "CPDLC MESSAGE LOG");
	put_page_ind(box, FMS_COLOR_WHITE);

	for (unsigned i = 0; i < MSG_LOG_LINES; i++) {
		unsigned thr_i = i + box->subpage * MSG_LOG_LINES;
		if (thr_i >= num_thr_ids)
			break;
		msg_log_draw_thr(box, thr_ids[thr_i], i);
	}

	put_str(box, LSK_HEADER_ROW(LSK5_ROW), 0, true, FMS_COLOR_CYAN,
	    FMS_FONT_SMALL, "FILTER");
	if (box->msg_log_open) {
		put_str(box, LSK5_ROW, 5, true, FMS_COLOR_WHITE,
		    FMS_FONT_SMALL, "ALL");
	} else {
		put_str(box, LSK5_ROW, 5, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "ALL");
	}
	put_str(box, LSK5_ROW, 4, true, FMS_COLOR_WHITE, FMS_FONT_LARGE, "/");
	if (box->msg_log_open) {
		put_str(box, LSK5_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "OPEN");
	} else {
		put_str(box, LSK5_ROW, 0, true, FMS_COLOR_WHITE,
		    FMS_FONT_SMALL, "OPEN");
	}

	free(thr_ids);
}

static bool
msg_log_key_cb(fmsbox_t *box, fms_key_t key)
{
	enum { MSG_LOG_LINES = 4 };

	ASSERT(box != NULL);

	if (key >= FMS_KEY_LSK_L1 && key <= FMS_KEY_LSK_L4) {
		unsigned num_thr_ids;
		cpdlc_msg_thr_id_t *thr_ids = get_thr_ids(box, &num_thr_ids,
		    box->msg_log_open);
		unsigned thr_nr = (key - FMS_KEY_LSK_L1) +
		    (box->subpage * MSG_LOG_LINES);

		if (thr_nr < num_thr_ids) {
			set_thr_id(box, thr_ids[thr_nr]);
			set_page(box, &fms_pages[FMS_PAGE_MSG_THR]);
		}
		free(thr_ids);
	} else if (key == FMS_KEY_LSK_R5) {
		box->msg_log_open = !box->msg_log_open;
	} else {
		return (false);
	}

	return (true);
}

static char **
msgs2lines(fmsbox_t *box, unsigned *n_lines_p)
{
#define	ADD_LINE(__start, __len) \
	do { \
		lines = safe_realloc(lines, (n_lines + 1) * sizeof (*lines)); \
		lines[n_lines] = safe_malloc((__len) + 1); \
		cpdlc_strlcpy(lines[n_lines], (__start), (__len) + 1); \
		n_lines++; \
	} while (0)
	char **lines = NULL;
	unsigned n_lines = 0;

	for (unsigned i = 0, n = cpdlc_msglist_get_thr_msg_count(box->msglist,
	    box->thr_id); i < n; i++) {
		const cpdlc_msg_t *msg;
		char buf[1024];
		const char *start, *cur, *end, *last_sp;

		cpdlc_msglist_get_thr_msg(box->msglist, box->thr_id, i,
		    &msg, NULL, NULL, NULL, NULL);
		cpdlc_msg_readable(msg, buf, sizeof (buf));
		last_sp = strchr(buf, ' ');
		for (start = buf, cur = buf, end = buf + strlen(buf);; cur++) {
			if (last_sp == NULL)
				last_sp = end;
			if (cur == end) {
				ADD_LINE(start, cur - start);
				break;
			}
			if (cur - start >= FMSBOX_COLS) {
				ADD_LINE(start, last_sp - start);
				if (last_sp == end)
					break;
				start = last_sp + 1;
				last_sp = strchr(start, ' ');
			} else if (isspace(cur[0])) {
				last_sp = cur;
			}
		}
		if (i + 1 < n) {
			lines = safe_realloc(lines,
			    (n_lines + 1) * sizeof (*lines));
			lines[n_lines] = strdup("------------------------");
			n_lines++;
		}
	}

	*n_lines_p = n_lines;

	return (lines);
#undef	ADD_LINE
}

static void
free_lines(char **lines, unsigned n_lines)
{
	for (unsigned i = 0; i < n_lines; i++)
		free(lines[i]);
	free(lines);
}

static bool
msg_can_resp(fmsbox_t *box, cpdlc_resp_type_t resp)
{
	const cpdlc_msg_t *msg;

	ASSERT(box != NULL);
	ASSERT(box->thr_id != CPDLC_NO_MSG_THR_ID);

	cpdlc_msglist_get_thr_msg(box->msglist, box->thr_id, 0, &msg,
	    NULL, NULL, NULL, NULL);
	return (msg->segs[0].info->resp == resp);
}

static bool
msg_can_roger(fmsbox_t *box)
{
	return (!cpdlc_msglist_thr_is_done(box->msglist, box->thr_id) &&
	    msg_can_resp(box, CPDLC_RESP_R));
}

static bool
msg_can_wilco(fmsbox_t *box)
{
	return (!cpdlc_msglist_thr_is_done(box->msglist, box->thr_id) &&
	    msg_can_resp(box, CPDLC_RESP_WU));
}

static bool
msg_can_affirm(fmsbox_t *box)
{
	return (!cpdlc_msglist_thr_is_done(box->msglist, box->thr_id) &&
	    msg_can_resp(box, CPDLC_RESP_AN));
}

static bool
msg_can_standby(fmsbox_t *box)
{
	cpdlc_msg_thr_status_t st =
	    cpdlc_msglist_get_thr_status(box->msglist, box->thr_id);
	return (!cpdlc_msglist_thr_is_done(box->msglist, box->thr_id) &&
	    st != CPDLC_MSG_THR_STANDBY &&
	    (msg_can_resp(box, CPDLC_RESP_AN) ||
	    msg_can_resp(box, CPDLC_RESP_WU) ||
	    msg_can_resp(box, CPDLC_RESP_R)));
}

static void
send_quick_resp(fmsbox_t *box, int msg_type)
{
	cpdlc_msg_t *msg = cpdlc_msg_alloc();

	ASSERT(box != NULL);

	cpdlc_msg_add_seg(msg, true, msg_type, 0);
	cpdlc_msglist_send(box->msglist, msg, box->thr_id);
}

static void
send_roger(fmsbox_t *box)
{
	send_quick_resp(box, CPDLC_DM3_ROGER);
}

static void
send_wilco(fmsbox_t *box)
{
	send_quick_resp(box, CPDLC_DM0_WILCO);
}

static void
send_affirm(fmsbox_t *box)
{
	send_quick_resp(box, CPDLC_DM4_AFFIRM);
}

static void
send_standby(fmsbox_t *box)
{
	send_quick_resp(box, CPDLC_DM2_STANDBY);
}

static void
msg_thr_draw_cb(fmsbox_t *box)
{
	char **lines;
	unsigned n_lines;
	const cpdlc_msg_t *msg;

	ASSERT(box != NULL);
	ASSERT(box->thr_id != CPDLC_NO_MSG_THR_ID);

	cpdlc_msglist_get_thr_msg(box->msglist, box->thr_id, 0, &msg,
	    NULL, NULL, NULL, NULL);
	lines = msgs2lines(box, &n_lines);
	set_num_subpages(box, ceil(n_lines / 5.0));

	put_page_title(box, "CPDLC MESSAGE");
	put_page_ind(box, FMS_COLOR_WHITE);
	draw_thr_hdr(box, 1, box->thr_id);

	for (unsigned i = 0; i < 5; i++) {
		unsigned line = i + box->subpage * 5;
		if (line >= n_lines)
			break;
		put_str(box, 2 + i, 0, false, FMS_COLOR_WHITE, FMS_FONT_LARGE,
		    "%s", lines[line]);
	}

	put_str(box, LSK_HEADER_ROW(LSK4_ROW), 0, false,
	    FMS_COLOR_WHITE, FMS_FONT_SMALL, "--------RESPONSE--------");

	if (msg_can_standby(box))
		put_lsk_action(box, FMS_KEY_LSK_L4, FMS_COLOR_CYAN, "*STANDBY");
	if (msg_can_roger(box)) {
		put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_CYAN, "*ROGER");
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN, "UNABLE>");
	} else if (msg_can_wilco(box)) {
		put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_CYAN, "*WILCO");
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN, "UNABLE>");
	} else if (msg_can_affirm(box)) {
		put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_CYAN, "*AFFIRM");
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN,
		    "NEGATIVE>");
	}

	free_lines(lines, n_lines);
}

static bool
msg_thr_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L6) {
		set_page(box, &fms_pages[FMS_PAGE_MSG_LOG]);
	} else if (key == FMS_KEY_LSK_L4) {
		if (msg_can_standby(box))
			send_standby(box);
	} else if (key == FMS_KEY_LSK_L5) {
		if (msg_can_roger(box))
			send_roger(box);
		else if (msg_can_wilco(box))
			send_wilco(box);
		else if (msg_can_affirm(box))
			send_affirm(box);
	} else {
		return (false);
	}

	return (true);
}

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

static void
freetext_draw_cb(fmsbox_t *box)
{
	enum { MAX_LINES = 4 };

	ASSERT(box != NULL);

	set_num_subpages(box, 2);

	put_page_title(box, "FANS  FREE TEXT");
	put_page_ind(box, FMS_COLOR_WHITE);

	for (int row = 0; row < MAX_LINES; row++) {
		int line = row + box->subpage * MAX_LINES;
		if (box->freetext[line][0] != '\0') {
			put_str(box, LSKi_ROW(row), 0, false, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "%s", box->freetext[line]);
		} else {
			put_str(box, LSKi_ROW(row), 0, false, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "%s", "------------------------");
		}
	}

	if (freetext_msg_ready(box)) {
		put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_CYAN, "*ERASE");
		put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN, "SEND*");
	}
}

static bool
freetext_key_cb(fmsbox_t *box, fms_key_t key)
{
	enum { MAX_LINES = 4 };

	ASSERT(box != NULL);

	if (key >= FMS_KEY_LSK_L1 && key <= FMS_KEY_LSK_L4) {
		int line = key - FMS_KEY_LSK_L1 + box->subpage * MAX_LINES;
		scratchpad_xfer(box, box->freetext[line],
		    sizeof (box->freetext[line]), true);
	} else if (key == FMS_KEY_LSK_L5) {
		memset(box->freetext, 0, sizeof (box->freetext));
	} else if (key == FMS_KEY_LSK_R5) {
		if (freetext_msg_ready(box)) {
			send_freetext_msg(box);
			set_page(box, &fms_pages[FMS_PAGE_MAIN_MENU]);
		}
	} else {
		return (false);
	}
	return (true);
}

static void
requests_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	put_page_title(box, "FANS  REQUESTS");

	put_lsk_action(box, FMS_KEY_LSK_L1, FMS_COLOR_WHITE, "<ALTITUDE");
	put_lsk_action(box, FMS_KEY_LSK_L2, FMS_COLOR_WHITE, "<OFFSET");
	put_lsk_action(box, FMS_KEY_LSK_L3, FMS_COLOR_WHITE, "<SPEED");
	put_lsk_action(box, FMS_KEY_LSK_L4, FMS_COLOR_WHITE, "<ROUTE");
	put_lsk_action(box, FMS_KEY_LSK_R1, FMS_COLOR_WHITE, "CLEARANCE>");
	put_lsk_action(box, FMS_KEY_LSK_R2, FMS_COLOR_WHITE, "VMC DESCENT>");
	put_lsk_action(box, FMS_KEY_LSK_R3, FMS_COLOR_WHITE, "WHEN CAN WE>");
	put_lsk_action(box, FMS_KEY_LSK_R4, FMS_COLOR_WHITE, "VOICE REQ>");
}

static bool
requests_key_cb(fmsbox_t *box, fms_key_t key)
{
	enum { MAX_LINES = 4 };

	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L1) {
		set_page(box, &fms_pages[FMS_PAGE_REQ_ALT]);
		memset(&box->alt_req, 0, sizeof (box->alt_req));
#if 0
	} else if (key == FMS_KEY_LSK_L2) {
		set_page(box, &fms_pages[FMS_PAGE_REQ_OFF]);
	} else if (key == FMS_KEY_LSK_L3) {
		set_page(box, &fms_pages[FMS_PAGE_REQ_SPD]);
	} else if (key == FMS_KEY_LSK_L4) {
		set_page(box, &fms_pages[FMS_PAGE_REQ_RTE]);
#endif
	} else {
		return (false);
	}

	return (true);
}

static void
put_altn_selector(fmsbox_t *box, int row, bool align_right, int option,
    const char *first, ...)
{
	va_list ap;
	int idx = 0, offset = 0;

	va_start(ap, first);
	for (const char *str = first, *next = NULL; str != NULL;
	    str = next, idx++) {
		next = va_arg(ap, const char *);
		if (idx == option) {
			put_str(box, row, offset, align_right, FMS_COLOR_GREEN,
			    FMS_FONT_LARGE, "%s", str);
		} else {
			put_str(box, row, offset, align_right, FMS_COLOR_WHITE,
			    FMS_FONT_SMALL, "%s", str);
		}
		offset += strlen(str);
		if (next != NULL) {
			put_str(box, row, offset, align_right, FMS_COLOR_WHITE,
			    FMS_FONT_LARGE, "/");
			offset++;
		}
	}
	va_end(ap);
}

static void
verify_send(fmsbox_t *box, const char *title, int ret_page)
{
	ASSERT(box != NULL);
	ASSERT(title != NULL);

	cpdlc_strlcpy(box->verify.title, title, sizeof (box->verify.title));
	box->verify.ret_page = ret_page;
	set_page(box, &fms_pages[FMS_PAGE_VRFY]);
}

static void
put_alt(fmsbox_t *box, int row, int col, const cpdlc_arg_t *alt)
{
	char buf[8];

	ASSERT(box != NULL);

	print_alt(alt, buf, sizeof (buf));
	put_str(box, row, col, false, FMS_COLOR_WHITE, FMS_FONT_LARGE,
	    "%s", buf);
}

static bool
can_verify_alt_req(fmsbox_t *box)
{
	ASSERT(box);
	return (box->alt_req.alt[0].alt.alt != 0);
}

static void
construct_alt_req(fmsbox_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg;

	if (box->verify.msg != NULL)
		cpdlc_msg_free(box->verify.msg);
	msg = box->verify.msg = cpdlc_msg_alloc();
	if (strlen(box->alt_req.step_at) != 0) {
		if (box->alt_req.step_at_time) {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM13_AT_time_REQ_CLB_TO_alt, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    &box->alt_req.step_hrs, &box->alt_req.step_mins);
		} else {
			seg = cpdlc_msg_add_seg(msg, true,
			    CPDLC_DM11_AT_pos_REQ_CLB_TO_alt, 0);
			cpdlc_msg_seg_set_arg(msg, seg, 0,
			    box->alt_req.step_at, NULL);
		}
		cpdlc_msg_seg_set_arg(msg, seg, 1,
		    &box->alt_req.alt[0].alt.fl, &box->alt_req.alt[0].alt.alt);
	} else if (box->alt_req.alt[1].alt.alt != 0) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM7_REQ_BLOCK_alt_TO_alt, 0);
		for (int i = 0; i < 2; i++) {
			cpdlc_msg_seg_set_arg(msg, seg, i,
			    &box->alt_req.alt[i].alt.fl,
			    &box->alt_req.alt[i].alt.alt);
		}
	} else {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM6_REQ_alt, 0);
		cpdlc_msg_seg_set_arg(msg, seg, 0, &box->alt_req.alt[0].alt.fl,
		    &box->alt_req.alt[0].alt.alt);
	}
	if (box->alt_req.plt_discret) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM75_AT_PILOTS_DISCRETION, 0);
	}
	if (box->alt_req.maint_sep_vmc) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM74_MAINT_OWN_SEPARATION_AND_VMC, 0);
	}
	if (box->alt_req.due_wx) {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM65_DUE_TO_WX, 0);
	} else if (box->alt_req.due_ac) {
		seg = cpdlc_msg_add_seg(msg, true,
		    CPDLC_DM66_DUE_TO_ACFT_PERF, 0);
	}
}

static void
req_alt_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	put_page_title(box, "FANS  ALTITUDE REQ");

	put_lsk_title(box, FMS_KEY_LSK_L1, "ALT/ALT BLOCK");
	if (box->alt_req.alt[0].alt.alt != 0) {
		put_alt(box, LSK1_ROW, 0, &box->alt_req.alt[0]);
	} else {
		put_str(box, LSK1_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "_____");
	}
	put_str(box, LSK1_ROW, 5, false, FMS_COLOR_CYAN, FMS_FONT_SMALL, "/");
	if (box->alt_req.alt[1].alt.alt != 0) {
		put_alt(box, LSK1_ROW, 6, &box->alt_req.alt[1]);
	} else {
		put_str(box, LSK1_ROW, 6, false, FMS_COLOR_CYAN,
		    FMS_FONT_SMALL, "-----");
	}

	put_lsk_title(box, FMS_KEY_LSK_L2, "DUE TO WX");
	put_altn_selector(box, LSK2_ROW, false, box->alt_req.due_wx,
	    "NO", "YES", NULL);

	put_lsk_title(box, FMS_KEY_LSK_L3, "DUE TO A/C");
	put_altn_selector(box, LSK3_ROW, false, box->alt_req.due_ac,
	    "NO", "YES", NULL);

	put_lsk_title(box, FMS_KEY_LSK_R1, "STEP AT");
	if (strlen(box->alt_req.step_at) != 0) {
		put_str(box, LSK1_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "%s", box->alt_req.step_at);
	} else {
		put_str(box, LSK1_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_LARGE, "NONE");
	}

	put_lsk_title(box, FMS_KEY_LSK_R4, "PLT DISCRET");
	put_altn_selector(box, LSK4_ROW, true, box->alt_req.plt_discret,
	    "NO", "YES", NULL);

	put_lsk_title(box, FMS_KEY_LSK_R5, "MAINT SEP/VMC");
	put_altn_selector(box, LSK5_ROW, true, box->alt_req.maint_sep_vmc,
	    "NO", "YES", NULL);

	if (can_verify_alt_req(box))
		put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE, "<VERIFY");

	put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

static bool
req_alt_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_L1) {
		scratchpad_parse_multipart(box,
		    (void *)offsetof(fmsbox_t, alt_req.alt),
		    sizeof (cpdlc_arg_t), parse_alt, insert_alt_block,
		    delete_alt_block, read_alt_block);
	} else if (key == FMS_KEY_LSK_L2) {
		box->alt_req.due_wx = !box->alt_req.due_wx;
		box->alt_req.due_ac = false;
	} else if (key == FMS_KEY_LSK_L3) {
		box->alt_req.due_wx = false;
		box->alt_req.due_ac = !box->alt_req.due_ac;
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_alt_req(box)) {
			construct_alt_req(box);
			verify_send(box, "ALT REQ", FMS_PAGE_REQ_ALT);
		}
	} else if (key == FMS_KEY_LSK_L6) {
		set_page(box, &fms_pages[FMS_PAGE_REQUESTS]);
	} else if (key == FMS_KEY_LSK_R1) {
		scratchpad_xfer(box, box->alt_req.step_at,
		    sizeof (box->alt_req.step_at), true);
		box->alt_req.step_at_time = parse_time(box->alt_req.step_at,
		    &box->alt_req.step_hrs, &box->alt_req.step_mins);
		if (strlen(box->alt_req.step_at) != 0) {
			memset(&box->alt_req.alt[1], 0,
			    sizeof (box->alt_req.alt[1]));
		}
	} else if (key == FMS_KEY_LSK_R4) {
		box->alt_req.plt_discret = !box->alt_req.plt_discret;
	} else if (key == FMS_KEY_LSK_R5) {
		box->alt_req.maint_sep_vmc = !box->alt_req.maint_sep_vmc;
	} else {
		return (false);
	}

	return (true);
}

static void
vrfy_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	put_lsk_action(box, FMS_KEY_LSK_R5, FMS_COLOR_CYAN, "SEND*");
	put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

static bool
vrfy_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (key == FMS_KEY_LSK_R5) {
		ASSERT(box->verify.msg != NULL);
		cpdlc_msglist_send(box->msglist, box->verify.msg,
		    CPDLC_NO_MSG_THR_ID);
		box->verify.msg = NULL;
		set_page(box, &fms_pages[FMS_PAGE_MAIN_MENU]);
	} else if (key == FMS_KEY_LSK_L6 && box->verify.ret_page >= 0 &&
	    box->verify.ret_page < FMS_NUM_PAGES) {
		set_page(box, &fms_pages[box->verify.ret_page]);
	} else {
		return (false);
	}

	return (true);
}
