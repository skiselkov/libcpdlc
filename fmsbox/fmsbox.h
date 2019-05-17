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

#ifndef	_LIBCPDLC_FMSBOX_H_
#define	_LIBCPDLC_FMSBOX_H_

#ifdef	__cplusplus
extern "C" {
#endif

#define	FMSBOX_COLS	24
#define	FMSBOX_ROWS	15

typedef struct fmsbox_s fmsbox_t;

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
} fmsbox_char_t;

fmsbox_t *fmsbox_alloc(const char *hostname, unsigned port,
    const char *ca_file);
void fmsbox_free(fmsbox_t *box);

const fmsbox_char_t *fmsbox_get_screen_row(const fmsbox_t *box, unsigned row);

void fmsbox_push_key(fmsbox_t *box, fms_key_t key);
void fmsbox_push_char(fmsbox_t *box, char c);
void fmsbox_update(fmsbox_t *box);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FMSBOX_H_ */
