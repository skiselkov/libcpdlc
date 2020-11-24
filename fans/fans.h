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

#ifndef	_LIBCPDLC_FANS_H_
#define	_LIBCPDLC_FANS_H_

#include "../src/cpdlc_msglist.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define	FMS_COLS	24
#define	FMS_ROWS	15

typedef struct fans_s fans_t;

typedef enum {
	FMS_KEY_LSK_L1,
	FMS_KEY_LSK_L2,
	FMS_KEY_LSK_L3,
	FMS_KEY_LSK_L4,
	FMS_KEY_LSK_L5,
	FMS_KEY_LSK_L6,
	FMS_KEY_LSK_R1,
	FMS_KEY_LSK_R2,
	FMS_KEY_LSK_R3,
	FMS_KEY_LSK_R4,
	FMS_KEY_LSK_R5,
	FMS_KEY_LSK_R6,
	FMS_KEY_MSG,
	FMS_KEY_EXEC,
	FMS_KEY_DIR,
	FMS_KEY_FPLN,
	FMS_KEY_LEGS,
	FMS_KEY_DEP_ARR,
	FMS_KEY_PERF,
	FMS_KEY_MFD_MENU,
	FMS_KEY_MFD_ADV,
	FMS_KEY_MFD_DATA,
	FMS_KEY_PREV,
	FMS_KEY_NEXT,
	FMS_KEY_IDX,
	FMS_KEY_CLR_DEL,
	FMS_KEY_CLR_DEL_LONG,	/* Long press - clears the scratchpad */
	FMS_KEY_TUN,
	FMS_KEY_PLUS_MINUS,
	FMS_NUM_KEYS
} fms_key_t;

typedef enum {
	FMS_COLOR_WHITE,
	FMS_COLOR_WHITE_INV,
	FMS_COLOR_GREEN,
	FMS_COLOR_AMBER,
	FMS_COLOR_CYAN,
	FMS_COLOR_MAGENTA
} fms_color_t;

typedef enum {
	FMS_FONT_SMALL,
	FMS_FONT_LARGE
} fms_font_t;

typedef struct {
	char		c;
	fms_color_t	color;
	fms_font_t	size;
} fms_char_t;

typedef struct {
	char		wpt_name[16];
	float		lat, lon;	/* degrees, set to NAN if unknown */
	bool		time_set;	/* are the hrs and mins fields set? */
	unsigned	hrs, mins;
	bool		alt_set;	/* is the alt field set? */
	bool		alt_fl;
	int		alt_ft;
	bool		spd_set;	/* is the spd field set? */
	bool		spd_mach;
	unsigned	spd;
} fms_wpt_info_t;

typedef bool (*fans_get_flt_id_t)(void *userinfo, char flt_id[8]);
typedef bool (*fans_get_geo_pos_t)(void *userinfo, double *lat, double *lon);
typedef bool (*fans_get_spd_t)(void *userinfo, bool *mach, unsigned *spd_KIAS);
typedef float (*fans_get_alt_t)(void *userinfo);
typedef float (*fans_get_vvi_t)(void *userinfo);
typedef bool (*fans_get_wpt_info_t)(void *userinfo, fms_wpt_info_t *info);
typedef bool (*fans_get_dest_info_t)(void *userinfo, fms_wpt_info_t *info,
    float *dist_NM, unsigned *flt_time_sec);
typedef float (*fans_get_offset_t)(void *userinfo);
typedef bool (*fans_get_fuel_t)(void *userinfo, unsigned *hrs, unsigned *mins);
typedef bool (*fans_get_temp_t)(void *userinfo, int *temp_C);
typedef bool (*fans_get_wind_t)(void *userinfo, unsigned *deg_true,
    unsigned *knots);
typedef void (*fans_msgs_updated_cb_t)(void *userinfo,
    cpdlc_msg_thr_id_t *updated_threads, unsigned num_updated_threads);
typedef bool (*fans_get_souls_t)(void *userinfo, unsigned *souls);

typedef struct {
	fans_get_flt_id_t	get_flt_id;
	cpdlc_get_time_func_t	get_time;
	fans_get_geo_pos_t	get_cur_pos;
	fans_get_spd_t		get_cur_spd;
	fans_get_alt_t		get_cur_alt;
	fans_get_vvi_t		get_cur_vvi;
	fans_get_alt_t		get_sel_alt;
	fans_get_wpt_info_t	get_prev_wpt;
	fans_get_wpt_info_t	get_next_wpt;
	fans_get_wpt_info_t	get_next_next_wpt;
	fans_get_dest_info_t	get_dest_info;
	fans_get_offset_t	get_offset;
	fans_get_fuel_t		get_fuel;
	fans_get_temp_t		get_sat;
	fans_get_wind_t		get_wind;
	fans_get_souls_t	get_souls;
	fans_msgs_updated_cb_t	msgs_updated;
} fans_funcs_t;

/*
 * The box can be set up for either a completely custom connection,
 * or for a specific network preset. This simplifies the user settings
 * that have to be done on subpage 2 of the LOGON/STATUS page.
 */
typedef enum {
	FANS_NETWORK_CUSTOM,
	FANS_NETWORK_PILOTEDGE
} fans_network_t;

fans_t *fans_alloc(const fans_funcs_t *funcs, void *userinfo);
void fans_free(fans_t *box);

cpdlc_client_t *fans_get_client(const fans_t *box);
cpdlc_msglist_t *fans_get_msglist(const fans_t *box);

const char *fans_get_flt_id(const fans_t *box);

const char *fans_get_logon_to(const fans_t *box);
void fans_set_logon_to(fans_t *box, const char *to);

fans_network_t fans_get_network(const fans_t *box);
void fans_set_network(fans_t *box, fans_network_t net);

void fans_set_shows_volume(fans_t *box, bool flag);
bool fans_get_shows_volume(const fans_t *box);
void fans_set_volume(fans_t *box, double volume);
double fans_get_volume(const fans_t *box);
/*
 * Only used by FANS_NETWORK_CUSTOM
 */
void fans_set_host(fans_t *box, const char *hostname);
const char *fans_get_host(const fans_t *box);
void fans_set_port(fans_t *box, int port);
int fans_get_port(const fans_t *box);
void fans_set_secret(fans_t *box, const char *secret);
const char *fans_get_secret(const fans_t *box);

const fms_char_t *fans_get_screen_row(const fans_t *box, unsigned row);

void fans_push_key(fans_t *box, fms_key_t key);
void fans_push_char(fans_t *box, char c);
void fans_update(fans_t *box);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FANS_H_ */
