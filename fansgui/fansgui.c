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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef	_WIN32
#include <windows.h>
#endif

#include <acfutils/glew.h>
#include <acfutils/log.h>
#include <acfutils/png.h>
#include <acfutils/time.h>

#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include "../fans/fans.h"
#include "mtcr_mini.h"

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
	UNUSED(w);
	UNUSED(h);
	UNUSED(userinfo);

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
	UNUSED(window);
	UNUSED(mods);
	UNUSED(button);

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
	UNUSED(window);
	UNUSED(scancode);
	UNUSED(mods);

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
	UNUSED(window);
	UNUSED(x);
	UNUSED(y);
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

static bool
fms_init(void)
{
	const char *hostname = "localhost";
	const char *ca_file = "cpdlc_cert.pem";
	int port = 0;

	box = fans_alloc(hostname, port, ca_file, NULL, NULL);
	return (box != NULL);
}

static void
cleanup(void)
{
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
	if (hand_cursor != NULL) {
		glfwDestroyCursor(hand_cursor);
		hand_cursor = NULL;
	}
	glfwTerminate();
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

int
main(void)
{
	GLenum err;

	memset(font_bitmaps, 0, sizeof (font_bitmaps));
	log_init(do_log_msg, "fansgui");

	if (!load_imgs())
		goto errout;
	if (!window_init())
		goto errout;
	if (!fms_init())
		goto errout;

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

		if (now - dirty > SEC2USEC(1)) {
			window_draw();
			dirty = now;
		}
		fans_update(box);
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
	UNUSED(hinst);
	UNUSED(resvd);
	lacf_glew_dllmain_hook(reason);
	return (TRUE);
}
#endif	/* _WIN32 */
