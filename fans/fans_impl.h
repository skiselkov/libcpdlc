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

#ifndef	_LIBCPDLC_FANS_IMPL_H_
#define	_LIBCPDLC_FANS_IMPL_H_

#include <stdbool.h>

#include "../src/cpdlc_msglist.h"
#include "fans.h"

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
#define	REJ_FREETEXT_LINES	3
#define	CRZ_CLB_THRESHOLD	32000	/* feet */

#define	APPEND_SNPRINTF(__buf, __len, ...) \
	do { \
		(__len) += snprintf(&(__buf)[(__len)], \
		    sizeof (__buf) >= (__len) ? sizeof (__buf) - (__len) : 0, \
		    __VA_ARGS__); \
	} while (0)

typedef struct {
	void	(*init_cb)(fans_t *box);
	void	(*draw_cb)(fans_t *box);
	bool	(*key_cb)(fans_t *box, fms_key_t key);
	bool	has_return;
} fms_page_t;

typedef struct {
	bool	set;
	int	hrs;
	int	mins;
} fms_time_t;

typedef enum {
	STEP_AT_NONE,
	STEP_AT_TIME,
	STEP_AT_POS,
	NUM_STEP_AT_TYPES
} step_at_type_t;

typedef struct {
	step_at_type_t	type;
	char		pos[8];
	fms_time_t	tim;
} fms_step_at_t;

typedef enum {
	FMS_POS_NAVAID,		/* 1-4 letter navaid ID */
	FMS_POS_ARPT,		/* 4 letter ICAO */
	FMS_POS_FIX,		/* 1-5 letter fix */
	FMS_POS_LAT_LON,	/* latitude-longitude */
	FMS_POS_PBD		/* place-bearing-distance */
} fms_pos_type_t;

typedef struct {
	fms_pos_type_t	type;
	bool		set;
	char		name[8];
	unsigned	brg;
	unsigned	dist;
	double		lat;
	double		lon;
} fms_pos_t;

typedef struct {
	bool		set;
	unsigned	deg;
	unsigned	spd;
} fms_wind_t;

typedef struct {
	bool		set;
	unsigned	hdg;
	bool		tru;
} fms_hdg_t;

typedef struct {
	bool		set;
	int		temp;
} fms_temp_t;

typedef struct {
	cpdlc_dir_t	dir;
	double		nm;
} fms_off_t;

typedef enum {
	CLX_REQ_NONE,
	CLX_REQ_ARR,
	CLX_REQ_APP,
	CLX_REQ_DEP
} clx_req_t;

typedef enum {
	ALT_CHG_NONE,
	ALT_CHG_HIGHER,
	ALT_CHG_LOWER
} alt_chg_t;

typedef enum {
	REJ_DUE_NONE,
	REJ_DUE_WX,
	REJ_DUE_AC,
	REJ_DUE_UNLOADABLE
} rej_due_t;

typedef enum {
	EMER_REASON_NONE,
	EMER_REASON_WX,
	EMER_REASON_MED,
	EMER_REASON_CABIN_PRESS,
	EMER_REASON_ENG_LOSS,
	EMER_REASON_LOW_FUEL
} emer_reason_t;

typedef void (*pos_pick_done_cb_t)(fans_t *box, const fms_pos_t *pos);

struct fans_s {
	fms_char_t		scr[FMS_ROWS][FMS_COLS];
	fans_funcs_t		funcs;
	void			*userinfo;
	fms_page_t		*page;
	unsigned		subpage;
	unsigned		num_subpages;
	char			scratchpad[SCRATCHPAD_MAX + 1];
	char			error_msg[ERROR_MSG_MAX + 1];

	char			flt_id[12];
	char			to[8];

	cpdlc_msg_thr_id_t	thr_id;
	bool			msg_log_open;

	union {
		char		freetext[MAX_FREETEXT_LINES][FMS_COLS + 1];
		struct {
			cpdlc_arg_t	alt[2];
			bool		crz_clb;
			fms_step_at_t	step_at;
			bool		plt_discret;
			bool		maint_sep_vmc;
		} alt_req;
		struct {
			fms_off_t	off;
			fms_step_at_t	step_at;
		} off_req;
		struct {
			cpdlc_arg_t	spd[2];
		} spd_req;
		struct {
			fms_pos_t	dct;
			fms_pos_t	wx_dev;
			fms_hdg_t	hdg;
			fms_hdg_t	trk;
		} rte_req;
		struct {
			clx_req_t	type;
			char		proc[8];
			char		trans[8];
			bool		clx;
		} clx_req;
		struct {
			cpdlc_arg_t	alt;
			cpdlc_arg_t	spd[2];
			bool		crz_clb;
			bool		back_on_rte;
			alt_chg_t	alt_chg;
		} wcw_req;
		struct {
			bool		freq_set;
			double		freq;
		} voice_req;
		struct {
			fms_pos_t	rpt_wpt;
			fms_time_t	wpt_time;
			cpdlc_arg_t	wpt_alt;
			cpdlc_arg_t	spd;
			fms_pos_t	nxt_fix;
			fms_time_t	nxt_fix_time;
			fms_pos_t	nxt_fix1;
			fms_temp_t	temp;
			fms_wind_t	winds_aloft;
			fms_pos_t	cur_pos;
			fms_time_t	pos_time;
			cpdlc_arg_t	alt;
			fms_time_t	time_at_dest;
			cpdlc_arg_t	clb_des;
			fms_off_t	off;
		} pos_rep;
	};
	struct {
		bool	due_wx;
		bool	due_ac;
		bool	due_tfc;
		bool	distress;
		char	freetext[REQ_FREETEXT_LINES][FMS_COLS + 1];
	} req_common;
	struct {
		cpdlc_msg_t	*msg;
		char		title[8];
		unsigned	ret_page;
		bool		is_req;
	} verify;
	struct {
		bool		unable_or_neg;
		unsigned	ret_page;
		rej_due_t	due;
		char		freetext[REJ_FREETEXT_LINES][FMS_COLS + 1];
	} rej;
	struct {
		fms_pos_t		pos;
		bool			was_set;
		unsigned		ret_page;
		pos_pick_done_cb_t	done_cb;
	} pos_pick;
	struct {
		bool			pan;
		fms_time_t		fuel;
		bool			souls_set;
		unsigned		souls;
		cpdlc_arg_t		des;
		fms_off_t		off;
		fms_pos_t		divert;
		emer_reason_t		reason;
	} emer;

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
	FMS_PAGE_REJ,
	FMS_PAGE_VRFY,
	FMS_PAGE_EMER,
	FMS_PAGE_POS_PICK,
	FMS_PAGE_POS_REP,
	FMS_NUM_PAGES
};

void fans_set_thr_id(fans_t *box, cpdlc_msg_thr_id_t thr_id);
void fans_set_page(fans_t *box, unsigned page_nr, bool init);
void fans_set_num_subpages(fans_t *box, unsigned num);
void fans_set_error(fans_t *box, const char *error);

void fans_put_page_ind(fans_t *box, fms_color_t color);
void fans_put_atc_status(fans_t *box);

int fans_put_str(fans_t *box, unsigned row, unsigned col,
    bool align_right, fms_color_t color, fms_font_t size,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(7);
void fans_put_page_title(fans_t *box,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(2);
void fans_put_lsk_action(fans_t *box, int lsk_key_id, fms_color_t color,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(4);
void fans_put_lsk_title(fans_t *box, int lsk_key_id,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(3);

void fans_put_altn_selector(fans_t *box, int row, bool align_right,
    int option, const char *first, ...);

void fans_put_alt(fans_t *box, int row, int col, bool align_right,
    const cpdlc_arg_t *alt, bool req, bool units);
void fans_put_spd(fans_t *box, int row, int col, bool align_right,
    const cpdlc_arg_t *spd, bool req, bool units);
void fans_put_hdg(fans_t *box, int row, int col, bool align_right,
    const fms_hdg_t *hdg, bool req);
void fans_put_time(fans_t *box, int row, int col, bool align_right,
    const fms_time_t *t, bool req, bool colon);
void fans_put_temp(fans_t *box, int row, int col, bool align_right,
    const fms_temp_t *temp, bool req);
void fans_put_pos(fans_t *box, int row, int col, bool align_right,
    const fms_pos_t *pos, bool req);
void fans_put_off(fans_t *box, int row, int col, bool align_right,
    const fms_off_t *off, bool req);
void fans_put_wind(fans_t *box, int row, int col, bool align_right,
    const fms_wind_t *wind, bool req);

const char *fans_thr_status2str(cpdlc_msg_thr_status_t st, bool dirty);
void fans_msg2lines(const cpdlc_msg_t *msg, char ***lines_p,
    unsigned *n_lines_p);
void fans_thr2lines(cpdlc_msglist_t *msglist, cpdlc_msg_thr_id_t thr_id,
    char ***lines_p, unsigned *n_lines_p);
void fans_free_lines(char **lines, unsigned n_lines);

cpdlc_msg_thr_id_t *fans_get_thr_ids(fans_t *box, unsigned *num_thr_ids,
    bool ignore_closed);

void fans_put_step_at(fans_t *box, const fms_step_at_t *step_at);
void fans_key_step_at(fans_t *box, fms_key_t key, fms_step_at_t *step_at);
bool fans_step_at_can_send(const fms_step_at_t *step_at);

int fans_get_time(const fans_t *box);
int fans_get_cur_alt(const fans_t *box);
int fans_get_sel_alt(const fans_t *box);
bool fans_get_prev_wpt(const fans_t *box, fms_wpt_info_t *info);
bool fans_get_next_wpt(const fans_t *box, fms_wpt_info_t *info);
bool fans_get_next_next_wpt(const fans_t *box, fms_wpt_info_t *info);
bool fans_get_dest_wpt(const fans_t *box, fms_wpt_info_t *info);
int fans_get_offset(const fans_t *box);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FANS_IMPL_H_ */
