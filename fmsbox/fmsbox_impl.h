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

#ifndef	_LIBCPDLC_FMSBOX_IMPL_H_
#define	_LIBCPDLC_FMSBOX_IMPL_H_

#include <stdbool.h>

#include "../src/cpdlc_msglist.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define	SCRATCHPAD_MAX		22
#define	SCRATCHPAD_ROW		13
#define	ERROR_MSG_MAX		19
#define	ERROR_MSG_ROW		14
#define	LSK1_ROW		2
#define	LSK2_ROW		4
#define	LSK3_ROW		6
#define	LSK4_ROW		8
#define	LSK5_ROW		10
#define	LSK6_ROW		12
#define	LSKi_ROW(_idx)		((_idx) * 2 + 2)
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

void fmsbox_set_thr_id(fmsbox_t *box, cpdlc_msg_thr_id_t thr_id);
void fmsbox_set_page(fmsbox_t *box, unsigned page_nr);
void fmsbox_set_num_subpages(fmsbox_t *box, unsigned num);

void fmsbox_put_str(fmsbox_t *box, unsigned row, unsigned col,
    bool align_right, fms_color_t color, fms_font_t size,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(7);
void fmsbox_put_page_title(fmsbox_t *box,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(2);
void fmsbox_put_lsk_action(fmsbox_t *box, int lsk_key_id, fms_color_t color,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(4);
void fmsbox_put_lsk_title(fmsbox_t *box, int lsk_key_id,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(3);

void fmsbox_put_altn_selector(fmsbox_t *box, int row, bool align_right,
    int option, const char *first, ...);
void fmsbox_put_alt(fmsbox_t *box, int row, int col, const cpdlc_arg_t *alt);

void fmsbox_main_menu_draw_cb(fmsbox_t *box);
bool fmsbox_main_menu_key_cb(fmsbox_t *box, fms_key_t key);
void fmsbox_logon_status_draw_cb(fmsbox_t *box);
bool fmsbox_logon_status_key_cb(fmsbox_t *box, fms_key_t key);
void fmsbox_msg_log_draw_cb(fmsbox_t *box);
bool fmsbox_msg_log_key_cb(fmsbox_t *box, fms_key_t key);
void fmsbox_msg_thr_draw_cb(fmsbox_t *box);
bool fmsbox_msg_thr_key_cb(fmsbox_t *box, fms_key_t key);
void fmsbox_freetext_draw_cb(fmsbox_t *box);
bool fmsbox_freetext_key_cb(fmsbox_t *box, fms_key_t key);
void fmsbox_requests_draw_cb(fmsbox_t *box);
bool fmsbox_requests_key_cb(fmsbox_t *box, fms_key_t key);
void fmsbox_req_alt_draw_cb(fmsbox_t *box);
bool fmsbox_req_alt_key_cb(fmsbox_t *box, fms_key_t key);
#if 0
void fmsbox_req_off_draw_cb(fmsbox_t *box);
bool fmsbox_req_off_key_cb(fmsbox_t *box, fms_key_t key);
void fmsbox_req_spd_draw_cb(fmsbox_t *box);
bool fmsbox_req_spd_key_cb(fmsbox_t *box, fms_key_t key);
void fmsbox_req_rte_draw_cb(fmsbox_t *box);
bool fmsbox_req_rte_key_cb(fmsbox_t *box, fms_key_t key);
#endif
void fmsbox_vrfy_draw_cb(fmsbox_t *box);
bool fmsbox_vrfy_key_cb(fmsbox_t *box, fms_key_t key);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FMSBOX_IMPL_H_ */
