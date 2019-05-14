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
#include "fmsbox.h"

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
#define	REQ_FREETEXT_LINES	4

typedef struct {
	void	(*draw_cb)(fmsbox_t *box);
	bool	(*key_cb)(fmsbox_t *box, fms_key_t key);
	bool	has_return;
} fms_page_t;

typedef enum {
	STEP_AT_NONE,
	STEP_AT_TIME,
	STEP_AT_POS,
	NUM_STEP_AT_TYPES
} step_at_type_t;

typedef struct {
	step_at_type_t	type;
	char		pos[8];
	bool		time_set;
	int		hrs;
	int		mins;
} fms_step_at_t;

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

	union {
		struct {
			cpdlc_arg_t	alt[2];
			fms_step_at_t	step_at;
			bool		plt_discret;
			bool		maint_sep_vmc;
		} alt_req;
		struct {
			cpdlc_dir_t	dir;
			double		nm;
			fms_step_at_t	step_at;
		} off_req;
		struct {
			cpdlc_arg_t	spd[2];
		} spd_req;
	};
	struct {
		bool	due_wx;
		bool	due_ac;
		bool	due_tfc;
		char	freetext[REQ_FREETEXT_LINES][FMSBOX_COLS + 1];
	} req_common;

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
	FMS_PAGE_REQ_OFF,
	FMS_PAGE_REQ_SPD,
	FMS_PAGE_REQ_RTE,
	FMS_PAGE_REQ_CLX,
	FMS_PAGE_REQ_VMC,
	FMS_PAGE_REQ_WCW,
	FMS_PAGE_REQ_VOICE,
	FMS_PAGE_VRFY,
	FMS_NUM_PAGES
};

void fmsbox_set_thr_id(fmsbox_t *box, cpdlc_msg_thr_id_t thr_id);
void fmsbox_set_page(fmsbox_t *box, unsigned page_nr);
void fmsbox_set_num_subpages(fmsbox_t *box, unsigned num);
void fmsbox_set_error(fmsbox_t *box, const char *error);

void fmsbox_put_page_ind(fmsbox_t *box, fms_color_t color);
void fmsbox_put_atc_status(fmsbox_t *box);

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
void fmsbox_put_spd(fmsbox_t *box, int row, int col, const cpdlc_arg_t *alt);

const char *fmsbox_thr_status2str(cpdlc_msg_thr_status_t st);
void fmsbox_msg2lines(const cpdlc_msg_t *msg, char ***lines_p,
    unsigned *n_lines_p);
void fmsbox_thr2lines(cpdlc_msglist_t *msglist, cpdlc_msg_thr_id_t thr_id,
    char ***lines_p, unsigned *n_lines_p);
void fmsbox_free_lines(char **lines, unsigned n_lines);

cpdlc_msg_thr_id_t *fmsbox_get_thr_ids(fmsbox_t *box, unsigned *num_thr_ids,
    bool ignore_closed);

void fmsbox_put_step_at(fmsbox_t *box, const fms_step_at_t *step_at);
void fmsbox_key_step_at(fmsbox_t *box, fms_key_t key, fms_step_at_t *step_at);
bool fmsbox_step_at_can_send(const fms_step_at_t *step_at);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FMSBOX_IMPL_H_ */
