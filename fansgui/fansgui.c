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

#include <errno.h>
#if	APL || LIN
#include <libgen.h>
#endif
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef	_WIN32
#include <windows.h>
#endif

#include <acfutils/conf.h>
#include <acfutils/glew.h>
#include <acfutils/log.h>
#include <acfutils/png.h>
#include <acfutils/time.h>
#include <acfutils/wav.h>

#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include <cpdlc_assert.h>
#include <cpdlc_string.h>

#include "../fans/fans.h"
#include "mtcr_mini.h"
#include "xpintf.h"

#define	EVENT_POLL_TIMEOUT	0.1	/* seconds */

typedef struct {
	double	x, y, w, h;
	void	(*fmsfunc)(fans_t *box, int arg);
	int	arg;
} clickspot_t;

static GLFWwindow*		window = NULL;
static GLFWcursor		*hand_cursor = NULL;
static uint64_t			dirty = 0;
static mtcr_t			*mtcr = NULL;

static fans_t			*box = NULL;
static double			mouse_x, mouse_y;
static uint64_t			clr_press_microtime = 0;

static alc_t			*alc = NULL;
static wav_t			*fans_alert = NULL;

static cairo_surface_t		*bgimg = NULL;
static unsigned			bgimg_w, bgimg_h;
static double			win_ratio;
static cairo_surface_t		*screen = NULL;
static const char		*font_sz_names[FMS_FONT_LARGE + 1] =
    { "MCDU_small.png", "MCDU.png" };
static uint32_t			font_fg_colors[FMS_COLOR_MAGENTA + 1] = {
	0x00ffffffu,	/* FMS_COLOR_WHITE */
	0x00000000u,	/* FMS_COLOR_WHITE_INV */
	0x0000ff00u,	/* FMS_COLOR_GREEN */
	0x00ffff00u,	/* FMS_COLOR_AMBER */
	0x0000ffffu,	/* FMS_COLOR_CYAN */
	0x00ff00ffu	/* FMS_COLOR_MAGENTA */
};
static uint32_t			font_bg_colors[FMS_COLOR_MAGENTA + 1] = {
	0x00000000u,	/* FMS_COLOR_WHITE */
	0x00ffffffu,	/* FMS_COLOR_WHITE_INV */
	0x00000000u,	/* FMS_COLOR_GREEN */
	0x00000000u,	/* FMS_COLOR_AMBER */
	0x00000000u,	/* FMS_COLOR_CYAN */
	0x00000000u	/* FMS_COLOR_MAGENTA */
};
static cairo_surface_t *font_bitmaps[FMS_FONT_LARGE + 1][FMS_COLOR_MAGENTA + 1];
#define	FONT_CELL_W	13	/* pixels */
#define	FONT_CELL_H	20	/* pixels */
#define	FONT_X(nr)	((nr) * FONT_CELL_W)
#define	FONT_Y(nr)	((nr) * FONT_CELL_H)

#define	TEXT_AREA_X	123	/* pixels */
#define	TEXT_AREA_Y	75	/* pixels */
#define	TEXT_CHAR_W	16	/* pixels */
#define	TEXT_CHAR_H	24	/* pixels */

#define	_(func)	((void (*)(fans_t *, int))(func))
static clickspot_t	clickspots[] = {
    { .013, .142, .068, .042, _(fans_push_key), FMS_KEY_LSK_L1 },
    { .013, .203, .068, .042, _(fans_push_key), FMS_KEY_LSK_L2 },
    { .013, .264, .068, .042, _(fans_push_key), FMS_KEY_LSK_L3 },
    { .013, .325, .068, .042, _(fans_push_key), FMS_KEY_LSK_L4 },
    { .013, .387, .068, .042, _(fans_push_key), FMS_KEY_LSK_L5 },
    { .013, .450, .068, .042, _(fans_push_key), FMS_KEY_LSK_L6 },
    { .916, .142, .068, .042, _(fans_push_key), FMS_KEY_LSK_R1 },
    { .916, .203, .068, .042, _(fans_push_key), FMS_KEY_LSK_R2 },
    { .916, .264, .068, .042, _(fans_push_key), FMS_KEY_LSK_R3 },
    { .916, .325, .068, .042, _(fans_push_key), FMS_KEY_LSK_R4 },
    { .916, .387, .068, .042, _(fans_push_key), FMS_KEY_LSK_R5 },
    { .916, .450, .068, .042, _(fans_push_key), FMS_KEY_LSK_R6 },
    { .015, .690, .069, .058, _(fans_push_key), FMS_KEY_IDX },
    { .093, .690, .069, .058, _(fans_push_char), '1' },
    { .176, .690, .069, .058, _(fans_push_char), '2' },
    { .259, .690, .069, .058, _(fans_push_char), '3' },
    { .342, .690, .069, .058, _(fans_push_char), 'A' },
    { .425, .690, .069, .058, _(fans_push_char), 'B' },
    { .508, .690, .069, .058, _(fans_push_char), 'C' },
    { .591, .690, .069, .058, _(fans_push_char), 'D' },
    { .674, .690, .069, .058, _(fans_push_char), 'E' },
    { .756, .690, .069, .058, _(fans_push_char), 'F' },
    { .838, .690, .069, .058, _(fans_push_char), 'G' },
    { .919, .690, .069, .058, _(fans_push_key), FMS_KEY_CLR_DEL },
    { .093, .769, .069, .058, _(fans_push_char), '4' },
    { .176, .769, .069, .058, _(fans_push_char), '5' },
    { .259, .769, .069, .058, _(fans_push_char), '6' },
    { .342, .769, .069, .058, _(fans_push_char), 'H' },
    { .425, .769, .069, .058, _(fans_push_char), 'I' },
    { .508, .769, .069, .058, _(fans_push_char), 'J' },
    { .591, .769, .069, .058, _(fans_push_char), 'K' },
    { .674, .769, .069, .058, _(fans_push_char), 'L' },
    { .756, .769, .069, .058, _(fans_push_char), 'M' },
    { .838, .769, .069, .058, _(fans_push_char), 'N' },
    { .093, .849, .069, .058, _(fans_push_char), '7' },
    { .176, .849, .069, .058, _(fans_push_char), '8' },
    { .259, .849, .069, .058, _(fans_push_char), '9' },
    { .342, .849, .069, .058, _(fans_push_char), 'O' },
    { .425, .849, .069, .058, _(fans_push_char), 'P' },
    { .508, .849, .069, .058, _(fans_push_char), 'Q' },
    { .591, .849, .069, .058, _(fans_push_char), 'R' },
    { .674, .849, .069, .058, _(fans_push_char), 'S' },
    { .756, .849, .069, .058, _(fans_push_char), 'T' },
    { .838, .849, .069, .058, _(fans_push_char), 'U' },
    { .093, .928, .069, .058, _(fans_push_char), '.' },
    { .176, .928, .069, .058, _(fans_push_char), '0' },
    { .259, .928, .069, .058, _(fans_push_key), FMS_KEY_PLUS_MINUS },
    { .342, .928, .069, .058, _(fans_push_char), 'V' },
    { .425, .928, .069, .058, _(fans_push_char), 'W' },
    { .508, .928, .069, .058, _(fans_push_char), 'X' },
    { .591, .928, .069, .058, _(fans_push_char), 'Y' },
    { .674, .928, .069, .058, _(fans_push_char), 'Z' },
    { .756, .928, .069, .058, _(fans_push_char), ' ' },
    { .838, .928, .069, .058, _(fans_push_char), '/' },
    { .817, .617, .069, .058, _(fans_push_key), FMS_KEY_PREV },
    { .919, .617, .069, .058, _(fans_push_key), FMS_KEY_NEXT },
    { .w = 0 }	/* list terminator */
};
#undef	_
static int		cur_clickspot = -1;
static char		auto_flt_id[8] = {};

static bool get_auto_flt_id(void *userinfo, char flt_id[8]);
static void msgs_updated_cb(void *userinfo, cpdlc_msg_thr_id_t *thr_ids,
    unsigned n);

static void get_time(void *userinfo, unsigned *hours, unsigned *mins);
static bool get_cur_pos(void *userinfo, double *lat, double *lon);
static bool get_cur_spd(void *userinfo, bool *mach, unsigned *spd);
static float get_cur_alt(void *userinfo);
static float get_cur_vvi(void *userinfo);
static float get_sel_alt(void *userinfo);
static bool get_sat(void *userinfo, int *temp);
static bool get_wind(void *userinfo, unsigned *deg_true, unsigned *knots);
static float get_offset(void *userinfo);
static bool get_fuel(void *userinfo, unsigned *hours, unsigned *mins);
static bool get_prev_wpt(void *userinfo, fms_wpt_info_t *info);
static bool get_next_wpt(void *userinfo, fms_wpt_info_t *info);
static bool get_next_next_wpt(void *userinfo, fms_wpt_info_t *info);
static bool get_dest_info(void *userinfo, fms_wpt_info_t *info,
    float *dist_NM, unsigned *flt_dur_sec);

static const fans_funcs_t funcs = {
    .get_flt_id = get_auto_flt_id,
    .msgs_updated = msgs_updated_cb,
    .get_time = get_time,
    .get_cur_pos = get_cur_pos,
    .get_cur_spd = get_cur_spd,
    .get_cur_alt = get_cur_alt,
    .get_cur_vvi = get_cur_vvi,
    .get_sel_alt = get_sel_alt,
    .get_sat = get_sat,
    .get_wind = get_wind,
    .get_offset = get_offset,
    .get_fuel = get_fuel,
    .get_prev_wpt = get_prev_wpt,
    .get_next_wpt = get_next_wpt,
    .get_next_next_wpt = get_next_next_wpt,
    .get_dest_info = get_dest_info
};

static void
get_time(void *userinfo, unsigned *hours, unsigned *mins)
{
	const struct tm *tm;
	time_t now;

	UNUSED(userinfo);
	ASSERT(hours != NULL);
	ASSERT(mins != NULL);

	if (xpintf_get_time(hours, mins))
		return;
	now = time(NULL);
	tm = gmtime(&now);
	*hours = tm->tm_hour;
	*mins = tm->tm_min;
}

static bool
get_cur_pos(void *userinfo, double *lat, double *lon)
{
	UNUSED(userinfo);
	ASSERT(lat != NULL);
	ASSERT(lon != NULL);
	return (xpintf_get_cur_pos(lat, lon));
}

static bool
get_cur_spd(void *userinfo, bool *mach, unsigned *spd)
{
	UNUSED(userinfo);
	ASSERT(mach != NULL);
	ASSERT(spd != NULL);

	if (xpintf_get_cur_spd(mach, spd))
		return (true);
	return (false);
}

static float
get_cur_alt(void *userinfo)
{
	UNUSED(userinfo);
	return (xpintf_get_cur_alt());
}

static float
get_cur_vvi(void *userinfo)
{
	UNUSED(userinfo);
	return (xpintf_get_cur_vvi());
}

static float
get_sel_alt(void *userinfo)
{
	UNUSED(userinfo);
	return (xpintf_get_sel_alt());
}

static bool
get_sat(void *userinfo, int *temp_C)
{
	UNUSED(userinfo);
	if (xpintf_get_sat(temp_C))
		return (true);
	return (false);
}

static bool
get_wind(void *userinfo, unsigned *deg_true, unsigned *knots)
{
	UNUSED(userinfo);
	if (xpintf_get_wind(deg_true, knots))
		return (true);
	return (false);
}

static float
get_offset(void *userinfo)
{
	UNUSED(userinfo);
	return (xpintf_get_offset());
}

static bool
get_fuel(void *userinfo, unsigned *hours, unsigned *mins)
{
	UNUSED(userinfo);
	return (xpintf_get_fuel(hours, mins));
}

static bool
get_prev_wpt(void *userinfo, fms_wpt_info_t *info)
{
	UNUSED(userinfo);
	ASSERT(info != NULL);
	return (xpintf_get_prev_wpt(info));
}

static bool
get_next_wpt(void *userinfo, fms_wpt_info_t *info)
{
	UNUSED(userinfo);
	ASSERT(info != NULL);
	return (xpintf_get_next_wpt(info));
}

static bool
get_next_next_wpt(void *userinfo, fms_wpt_info_t *info)
{
	UNUSED(userinfo);
	ASSERT(info != NULL);
	return (xpintf_get_next_next_wpt(info));
}

static bool
get_dest_info(void *userinfo, fms_wpt_info_t *info, float *dist_NM,
    unsigned *flt_time_sec)
{
	UNUSED(userinfo);
	ASSERT(info != NULL);
	ASSERT(dist_NM != NULL);
	ASSERT(flt_time_sec != NULL);
	return (xpintf_get_dest_info(info, dist_NM, flt_time_sec));
}

static void
play_alert(void)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(fans_alert != NULL);

	wav_set_gain(fans_alert, fans_get_volume(box));
	wav_play(fans_alert);
}

static void
msgs_updated_cb(void *userinfo, cpdlc_msg_thr_id_t *thr_ids, unsigned n)
{
	cpdlc_msglist_t *msglist;

	CPDLC_ASSERT(box != NULL);
	CPDLC_UNUSED(userinfo);
	CPDLC_ASSERT(thr_ids != NULL || n == 0);
	msglist = fans_get_msglist(box);

	for (unsigned i = 0; i < n; i++) {
		bool dirty;

		cpdlc_msglist_get_thr_status(msglist, thr_ids[i], &dirty);
		if (dirty) {
			play_alert();
			break;
		}
	}
}

static bool
get_auto_flt_id(void *userinfo, char flt_id[8])
{
	CPDLC_UNUSED(userinfo);
	CPDLC_ASSERT(flt_id);
	if (strlen(auto_flt_id) != 0) {
		cpdlc_strlcpy(flt_id, auto_flt_id, 8);
		return (true);
	} else {
		return (false);
	}
}

static void
do_log_msg(const char *msg)
{
	fputs(msg, stderr);
}

cairo_surface_t *
load_cairo_png(const char *filename)
{
	cairo_surface_t *surf;
	cairo_status_t st;

	surf = cairo_image_surface_create_from_png(filename);
	if ((st = cairo_surface_status(surf)) != CAIRO_STATUS_SUCCESS) {
		logMsg("Can't load PNG file %s: %s", filename,
		    cairo_status_to_string(st));
		surf = NULL;
	}

	return (surf);
}

static void
char2fontcell(char c, int *font_x, int *font_y)
{
	if (c >= 'A' && c <= 'Z') {
		*font_x = FONT_X(c - 'A');
		*font_y = FONT_Y(1);
	} else if (c >= '0' && c <= '9') {
		*font_x = FONT_X(c - '0' + ('Z' - 'A') + 1);
		*font_y = FONT_Y(1);
	} else {
#define	FONT_MAP(__c, __x, __y) \
	case __c: \
		*font_x = FONT_X(__x); \
		*font_y = FONT_Y(__y); \
		break
		switch (c) {
			FONT_MAP('/', 0, 2);
			FONT_MAP('+', 1, 2);
			FONT_MAP('-', 2, 2);
			FONT_MAP('"', 4, 2);
			FONT_MAP('\'', 5, 2);
			FONT_MAP('.', 6, 2);
			FONT_MAP(',', 7, 2);
			FONT_MAP('[', 8, 2);
			FONT_MAP(']', 9, 2);
			FONT_MAP('\\', 10, 2);
			FONT_MAP(' ', 11, 2);
			FONT_MAP('<', 12, 2);
			FONT_MAP('>', 13, 2);
			FONT_MAP('{', 14, 2);
			FONT_MAP('}', 15, 2);
			FONT_MAP('(', 16, 2);
			FONT_MAP(')', 17, 2);
			FONT_MAP(':', 18, 2);
			FONT_MAP(';', 19, 2);
			FONT_MAP('|', 20, 2);
			FONT_MAP('?', 21, 2);
			FONT_MAP('~', 22, 2);
			FONT_MAP('=', 23, 2);
			FONT_MAP('*', 24, 2);
			FONT_MAP('!', 26, 2);
			FONT_MAP('@', 27, 2);
			FONT_MAP('$', 28, 2);
			FONT_MAP('%', 29, 2);
			FONT_MAP('#', 30, 2);
			FONT_MAP('&', 31, 2);
			FONT_MAP('`', 32, 2);
			FONT_MAP('^', 33, 2);	/* special for fans_t */
			FONT_MAP('v', 34, 2);
			FONT_MAP('_', 35, 2);	/* special for fans_t */
		default:
			*font_x = FONT_X(35);
			*font_y = FONT_Y(2);
			break;
		}
#undef	FONT_MAP
	}
}

static bool
snd_sys_init(void)
{
	alc = openal_init(NULL, true);
	if (alc == NULL) {
		logMsg("OpenAL init failed");
		return (false);
	}
	fans_alert = wav_load("fans_alert.wav", "fans_alert.wav", alc);
	if (fans_alert == NULL) {
		logMsg("Couldn't read fans_alert.wav");
		return (false);
	}
	return (true);
}

static void
snd_sys_fini(void)
{
	if (fans_alert != NULL) {
		wav_free(fans_alert);
		fans_alert = NULL;
	}
	if (alc != NULL) {
		openal_fini(alc);
		alc = NULL;
	}
}

static void
put_fms_char(const fms_char_t *c, int scr_x, int scr_y)
{
	cairo_surface_t *font_surf = font_bitmaps[c->size][c->color];
	unsigned char *src = cairo_image_surface_get_data(font_surf);
	int src_stride = cairo_image_surface_get_stride(font_surf);
	unsigned char *dest = cairo_image_surface_get_data(screen);
	int dest_stride = cairo_image_surface_get_stride(screen);

	int draw_x = TEXT_CHAR_W * scr_x + 1;
	int draw_y = TEXT_CHAR_H * scr_y + 1;
	int font_x, font_y;

	char2fontcell(c->c, &font_x, &font_y);

	for (unsigned y = 0; y < FONT_CELL_H; y++) {
		uint32_t *src_row =
		    (uint32_t *)&src[(y + font_y) * src_stride];
		uint32_t *dest_row =
		    (uint32_t *)&dest[(y + draw_y) * dest_stride];

		for (unsigned x = 0; x < FONT_CELL_W; x++)
			dest_row[x + draw_x] = src_row[x + font_x];
	}
}

static void
render_cb(cairo_t *cr, unsigned w, unsigned h, void *userinfo)
{
	CPDLC_UNUSED(w);
	CPDLC_UNUSED(h);
	CPDLC_UNUSED(userinfo);

	cairo_set_source_surface(cr, bgimg, 0, 0);
	cairo_paint(cr);

	for (unsigned row_i = 0; row_i < FMS_ROWS; row_i++) {
		const fms_char_t *row = fans_get_screen_row(box, row_i);

		for (unsigned col_i = 0; col_i < FMS_COLS; col_i++)
			put_fms_char(&row[col_i], col_i, row_i);
	}

	cairo_translate(cr, TEXT_AREA_X, TEXT_AREA_Y);
	cairo_scale(cr, 1.37, 1.1);
	cairo_set_source_surface(cr, screen, 0, 0);
	cairo_paint(cr);
	cairo_identity_matrix(cr);

	if (cur_clickspot != -1) {
		const clickspot_t *cs = &clickspots[cur_clickspot];

		cairo_set_source_rgb(cr, 0, 1, 1);
		cairo_set_line_width(cr, 5);
		cairo_new_path(cr);
		mtcr_rounded_rectangle(cr, w * cs->x, h * cs->y,
		    w * cs->w, h * cs->h, 10);
		cairo_stroke(cr);
	}
}

static bool
clickspot_mouse_check(clickspot_t *cs)
{
	return (mouse_x >= cs->x && mouse_x < cs->x + cs->w &&
	    mouse_y >= cs->y && mouse_y < cs->y + cs->h);
}

static void
mouse_button_cb(GLFWwindow *window, int button, int action, int mods)
{
	CPDLC_UNUSED(window);
	CPDLC_UNUSED(mods);
	CPDLC_UNUSED(button);

	if (action != GLFW_PRESS) {
		if (cur_clickspot != -1 &&
		    clickspot_mouse_check(&clickspots[cur_clickspot])) {
			clickspot_t *cs = &clickspots[cur_clickspot];

			ASSERT(cs->fmsfunc != NULL);
			cs->fmsfunc(box, cs->arg);
		}
		cur_clickspot = -1;
		clr_press_microtime = 0;
		dirty = 0;
		return;
	}

	cur_clickspot = -1;
	for (int i = 0; clickspots[i].w != 0; i++) {
		clickspot_t *cs = &clickspots[i];
		if (clickspot_mouse_check(cs)) {
			cur_clickspot = i;
			if (cs->fmsfunc ==
			    (void(*)(fans_t *, int))fans_push_key &&
			    cs->arg == FMS_KEY_CLR_DEL) {
				clr_press_microtime = microclock();
			}
			break;
		}
	}
	dirty = 0;
}

static void
mouse_hover_cb(GLFWwindow *window, double x, double y)
{
	int win_w, win_h;
	bool found = false;

	glfwGetWindowSize(window, &win_w, &win_h);

	mouse_x = x / win_w;
	mouse_y = y / win_h;

	for (int i = 0; clickspots[i].w != 0; i++) {
		if (clickspot_mouse_check(&clickspots[i])) {
			found = true;
			break;
		}
	}
	glfwSetCursor(window, found ? hand_cursor : NULL);
}

static void
key_cb(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	CPDLC_UNUSED(window);
	CPDLC_UNUSED(scancode);
	CPDLC_UNUSED(mods);

	if (action != GLFW_PRESS) {
		if (action == GLFW_RELEASE)
			clr_press_microtime = 0;
		return;
	}

	if ((key >= GLFW_KEY_0 && key <= GLFW_KEY_9) ||
	    (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) ||
	    key == GLFW_KEY_SPACE || key == GLFW_KEY_PERIOD ||
	    key == GLFW_KEY_SLASH) {
		fans_push_char(box, toupper(key));
	} else if (key == GLFW_KEY_DELETE || key == GLFW_KEY_BACKSPACE) {
		clr_press_microtime = microclock();
		fans_push_key(box, FMS_KEY_CLR_DEL);
	} else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_EQUAL) {
		fans_push_key(box, FMS_KEY_PLUS_MINUS);
	} else if (key == GLFW_KEY_PAGE_UP) {
		fans_push_key(box, FMS_KEY_PREV);
	} else if (key == GLFW_KEY_PAGE_DOWN) {
		fans_push_key(box, FMS_KEY_NEXT);
	}
	dirty = 0;
}

static void
resize_cb(GLFWwindow *window, int x, int y)
{
	CPDLC_UNUSED(window);
	CPDLC_UNUSED(x);
	CPDLC_UNUSED(y);
	dirty = 0;
}

static cairo_surface_t *
load_bitmap_font_img(const char *filename, uint32_t fg_color, uint32_t bg_color)
{
	cairo_surface_t *surf;
	unsigned char *bytes;
	int stride;
	unsigned w, h;

	ASSERT(filename != NULL);

	surf = load_cairo_png(filename);
	if (surf == NULL)
		return (NULL);

	bytes = cairo_image_surface_get_data(surf);
	stride = cairo_image_surface_get_stride(surf);
	w = cairo_image_surface_get_width(surf);
	h = cairo_image_surface_get_height(surf);
	for (unsigned y = 0; y < h; y++) {
		uint32_t *pixels = (uint32_t *)&bytes[y * stride];

		for (unsigned x = 0; x < w; x++) {
			if (pixels[x] != 0xff000000)
				pixels[x] = bg_color;
			else
				pixels[x] = fg_color;
		}
	}

	return (surf);
}

static bool
load_imgs(void)
{
	bgimg = load_cairo_png("fansgui.png");
	if (bgimg == NULL)
		return (false);
	bgimg_w = cairo_image_surface_get_width(bgimg);
	bgimg_h = cairo_image_surface_get_height(bgimg);
	win_ratio = bgimg_w / (double)bgimg_h;

	for (int sz = 0; sz < FMS_FONT_LARGE + 1; sz++) {
		for (int color = 0; color < FMS_COLOR_MAGENTA + 1; color++) {
			font_bitmaps[sz][color] = load_bitmap_font_img(
			    font_sz_names[sz], font_fg_colors[color],
			    font_bg_colors[color]);
			if (font_bitmaps[sz][color] == NULL)
				return (false);
		}
	}
	screen = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
	    FMS_COLS * TEXT_CHAR_W, FMS_ROWS * TEXT_CHAR_H);

	return (true);
}

static bool
window_init(void)
{
	if (!glfwInit())
		return (false);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(bgimg_w, bgimg_h, "FANS GUI", NULL, NULL);
	VERIFY(window != NULL);
	hand_cursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	VERIFY(hand_cursor != NULL);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_cb);
	glfwSetWindowSizeCallback(window, resize_cb);
	glfwSetMouseButtonCallback(window, mouse_button_cb);
	glfwSetCursorPosCallback(window, mouse_hover_cb);

	return (true);
}

static void
parse_conf(const conf_t *conf)
{
	const char *str;

	CPDLC_ASSERT(conf != NULL);
	CPDLC_ASSERT(box != NULL);

	if (conf_get_str(conf, "network", &str)) {
		if (strcmp(str, "custom") == 0) {
			fans_set_network(box, FANS_NETWORK_CUSTOM);
		} else if (strcmp(str, "pilotedge") == 0) {
			fans_set_network(box, FANS_NETWORK_PILOTEDGE);
		} else {
			logMsg("Unknown \"network\" stanza in config file.");
		}
	}
	if (fans_get_network(box) == FANS_NETWORK_CUSTOM) {
		int port;

		if (conf_get_str(conf, "hostname", &str))
			fans_set_host(box, str);
		if (conf_get_i(conf, "port", &port))
			fans_set_port(box, port);
	}
	if (conf_get_str(conf, "autofltid", &str))
		cpdlc_strlcpy(auto_flt_id, str, sizeof (auto_flt_id));
}

static void
parse_user(const conf_t *conf)
{
	const char *str;
	double vol;
	int network;

	CPDLC_ASSERT(conf != NULL);
	CPDLC_ASSERT(box != NULL);

	if (conf_get_d(conf, "volume", &vol))
		fans_set_volume(box, vol);
	if (conf_get_str(conf, "fltid", &str))
		cpdlc_strlcpy(auto_flt_id, str, sizeof (auto_flt_id));
	if (conf_get_str(conf, "logon_to", &str))
		fans_set_logon_to(box, str);
	if (conf_get_i(conf, "network", &network)) {
		network = MAX(MIN(network, FANS_NETWORK_PILOTEDGE), 0);
		fans_set_network(box, network);
		if (network == FANS_NETWORK_CUSTOM) {
			const char *host, *secret;
			int port;

			if (conf_get_str(conf, "host", &host))
				fans_set_host(box, host);
			if (conf_get_i(conf, "port", &port))
				fans_set_port(box, port);
			if (conf_get_str(conf, "secret", &secret))
				fans_set_secret(box, secret);
		}
	}
}

static void
save_user(void)
{
	fans_network_t network;
	conf_t *conf = conf_create_empty();
	const char *flt_id, *logon_to;

	CPDLC_ASSERT(box != NULL);
	flt_id = fans_get_flt_id(box);
	logon_to = fans_get_logon_to(box);
	network = fans_get_network(box);

	conf_set_f(conf, "volume", fans_get_volume(box));
	if (flt_id != NULL)
		conf_set_str(conf, "fltid", flt_id);
	if (logon_to != NULL)
		conf_set_str(conf, "logon_to", logon_to);
	conf_set_i(conf, "network", network);
	if (network == FANS_NETWORK_CUSTOM) {
		conf_set_str(conf, "host", fans_get_host(box));
		conf_set_i(conf, "port", fans_get_port(box));
		if (fans_get_secret(box) != NULL)
			conf_set_str(conf, "secret", fans_get_secret(box));
	}

	conf_write_file(conf, "user.conf");
	conf_free(conf);
}

static void
fms_init(void)
{
	cpdlc_client_t *cl;

	box = fans_alloc(&funcs, NULL);
	VERIFY(box != NULL);

	cl = fans_get_client(box);
	ASSERT(cl != NULL);
	cpdlc_client_set_ca_file(cl, "ca_cert.pem");

	fans_set_shows_volume(box, true);

	if (file_exists("user.conf", NULL)) {
		conf_t *conf = conf_read_file("user.conf", NULL);

		if (conf != NULL) {
			parse_user(conf);
			conf_free(conf);
		}
	}
	if (file_exists("client.conf", NULL)) {
		conf_t *conf = conf_read_file("client.conf", NULL);

		if (conf != NULL) {
			parse_conf(conf);
			conf_free(conf);
		}
	}
}

static void
cleanup(void)
{
	if (box != NULL)
		save_user();
	if (mtcr != NULL) {
		mtcr_fini(mtcr);
		mtcr = NULL;
	}
	if (bgimg != NULL) {
		cairo_surface_destroy(bgimg);
		bgimg = NULL;
	}
	for (int sz = 0; sz < FMS_FONT_LARGE + 1; sz++) {
		for (int color = 0; color < FMS_COLOR_MAGENTA + 1; color++) {
			if (font_bitmaps[sz][color] != NULL) {
				cairo_surface_destroy(font_bitmaps[sz][color]);
				font_bitmaps[sz][color] = NULL;
			}
		}
	}
	if (screen != NULL) {
		cairo_surface_destroy(screen);
		screen = NULL;
	}
	if (box != NULL) {
		fans_free(box);
		box = NULL;
	}
	xpintf_fini();
	if (hand_cursor != NULL) {
		glfwDestroyCursor(hand_cursor);
		hand_cursor = NULL;
	}
	glfwTerminate();
	lacf_glew_fini();
	snd_sys_fini();

#ifdef	_WIN32
	WSACleanup();
#endif
}

static void
window_draw(void)
{
	mat4 pvm;
	int win_w, win_h, opt_win_w;
	float xscale, yscale;

	glfwGetWindowContentScale(window, &xscale, &yscale);

	glfwGetWindowSize(window, &win_w, &win_h);
	opt_win_w = win_ratio * win_h;

	if (win_w != opt_win_w) {
		/* Keep the window aspect ratio */
		win_w = opt_win_w;
		glfwSetWindowSize(window, win_w, win_h);
	}

	glViewport(0, 0, win_w * xscale, win_h * yscale);
	glm_ortho(0, win_w, 0, win_h, 0, 1, pvm);

	glClear(GL_COLOR_BUFFER_BIT);
	mtcr_once_wait(mtcr);
	mtcr_draw(mtcr, ZERO_VECT2, VECT2(win_w, win_h), (GLfloat *)pvm);

	glfwSwapBuffers(window);
}

#ifdef	_WIN32
static bool
do_wsa_startup(void)
{
	WSADATA wsa_data;
	int err;

	err = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (err != 0) {
		logMsg("WSAStartup failed with error: %d", err);
		return (false);
	}
	/* confirm that the WinSock DLL supports 2.2. */
	if (LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2) {
		logMsg("Could not find a usable version of Winsock.dll");
		return (false);
	}

	return (true);
}

#endif	/* defined(_WIN32) */

int
main(int argc, char **argv)
{
	GLenum err;
#if	APL || LIN
	char *progname, *dir, *chdir_tgt, *appdir;
#endif

	UNUSED(argc);
	UNUSED(argv);

#if	APL || LIN
	progname = safe_strdup(argv[0]);
	dir = dirname(progname);
	/* chdir into the Resources subdirectory */
	appdir = strstr(dir, ".app/Contents/MacOS");
	if (appdir != NULL) {
		appdir[13] = '\0';
		chdir_tgt = sprintf_alloc("%s/Resources", dir);
	} else {
		chdir_tgt = safe_strdup(dir);
	}
	if (chdir(chdir_tgt) < 0) {
		fprintf(stderr, "Cannot chdir to %s: %s", chdir_tgt,
		    strerror(errno));
		return (1);
	}
	free(chdir_tgt);
	free(progname);
#endif	/* APL || LIN */

	memset(font_bitmaps, 0, sizeof (font_bitmaps));
	log_init(do_log_msg, "fansgui");
	lacf_glew_init();

#ifdef	_WIN32
	if (!do_wsa_startup())
		goto errout;
#endif	/* defined(_WIN32) */

	if (!snd_sys_init())
		goto errout;
	if (!load_imgs())
		goto errout;
	if (!window_init())
		goto errout;
	/*
	 * Must go ahead of fms_init, since fms_init can immediately
	 * call the get_time function.
	 */
	if (!xpintf_init(NULL, 0))
		goto errout;
	fms_init();

	glewExperimental = GL_TRUE;
	err = glewInit();
	if (err != GLEW_OK) {
		/* Problem: glewInit failed, something is seriously wrong. */
		logMsg("FATAL ERROR: cannot initialize libGLEW: %s",
		    glewGetErrorString(err));
		goto errout;
	}
	mtcr = mtcr_init(bgimg_w, bgimg_h, 0, NULL, render_cb, NULL, NULL);

	while (!glfwWindowShouldClose(window)) {
		uint64_t now = microclock();

		if (clr_press_microtime != 0 &&
		    now - clr_press_microtime >= SEC2USEC(1)) {
			fans_push_key(box, FMS_KEY_CLR_DEL_LONG);
			clr_press_microtime = 0;
			cur_clickspot = -1;
			dirty = 0;
		}

		if (now - dirty > SEC2USEC(0.5)) {
			window_draw();
			dirty = now;
		}
		fans_update(box);
		xpintf_update();
		glfwWaitEventsTimeout(EVENT_POLL_TIMEOUT);
	}

	cleanup();
	return (0);
errout:
	cleanup();
	return (1);
}

#ifdef	_WIN32
BOOL WINAPI
DllMain(HINSTANCE hinst, DWORD reason, LPVOID resvd)
{
	CPDLC_UNUSED(hinst);
	CPDLC_UNUSED(resvd);
	lacf_glew_dllmain_hook(reason);
	return (TRUE);
}
#endif	/* _WIN32 */
