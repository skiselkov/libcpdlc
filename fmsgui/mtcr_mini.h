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

#ifndef	_MTCR_MINI_H_
#define	_MTCR_MINI_H_

#include <cairo.h>
#include <cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <acfutils/geom.h>
#include <acfutils/glew.h>
#include <acfutils/log.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef bool_t (*mtcr_init_cb_t)(cairo_t *cr, void *userinfo);
typedef void (*mtcr_fini_cb_t)(cairo_t *cr, void *userinfo);
typedef void (*mtcr_render_cb_t)(cairo_t *cr, unsigned w, unsigned h,
    void *userinfo);
typedef struct mtcr_s mtcr_t;

mtcr_t *mtcr_init(unsigned w, unsigned h, double fps, mtcr_init_cb_t init_cb,
    mtcr_render_cb_t render_cb, mtcr_fini_cb_t fini_cb, void *userinfo);

void mtcr_fini(mtcr_t *mtcr);
void mtcr_set_fps(mtcr_t *mtcr, double fps);
double mtcr_get_fps(mtcr_t *mtcr);
void mtcr_once(mtcr_t *mtcr);
void mtcr_once_wait(mtcr_t *mtcr);
void mtcr_draw(mtcr_t *mtcr, vect2_t pos, vect2_t size, const GLfloat *pvm);
unsigned mtcr_get_width(mtcr_t *mtcr);
unsigned mtcr_get_height(mtcr_t *mtcr);

void mtcr_rounded_rectangle(cairo_t *cr, double x, double y,
    double w, double h, double radius);

#ifdef	__cplusplus
}
#endif

#endif	/* _MTCR_MINI_H_ */
