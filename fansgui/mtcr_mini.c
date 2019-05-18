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

#include <acfutils/assert.h>
#include <acfutils/geom.h>
#include <acfutils/glew.h>
#include <acfutils/safe_alloc.h>
#include <acfutils/glutils.h>
#include <acfutils/shader.h>
#include <acfutils/thread.h>
#include <acfutils/time.h>

#include "mtcr_mini.h"

#ifdef	_USE_MATH_DEFINES
#undef	_USE_MATH_DEFINES
#endif

#if	IBM
#ifdef	__SSE2__
#undef	__SSE2__
#endif
#endif	/* IBM */

#include <cglm/cglm.h>

typedef struct {
	GLfloat		pos[3];
	GLfloat		tex0[2];
} vtx_t;

typedef	struct {
	bool_t		chg;
	bool_t		rdy;

	GLuint		tex;
	cairo_surface_t	*surf;
	cairo_t		*cr;
} render_surf_t;

struct mtcr_s {
	unsigned		w, h;
	double			fps;
	mtcr_render_cb_t	render_cb;
	mtcr_fini_cb_t		fini_cb;
	void			*userinfo;

	int			cur_rs;
	render_surf_t		rs[2];

	thread_t		thr;
	condvar_t		cv;
	condvar_t		render_done_cv;
	bool_t			one_shot_block;
	mutex_t			lock;
	bool_t			started;
	bool_t			shutdown;

	/* Only accessed from OpenGL drawing thread, so no locking req'd */
	struct {
		double		x1, x2, y1, y2;
		vect2_t		pos;
		vect2_t		size;
	} last_draw;
	GLuint			vao;
	GLuint			vtx_buf;
	GLuint			idx_buf;
	GLuint			shader;
	GLint			shader_loc_pvm;
	GLint			shader_loc_tex;
};

static const char *vert_shader =
    "#version 120\n"
    "uniform mat4	pvm;\n"
    "attribute vec3	vtx_pos;\n"
    "attribute vec2	vtx_tex0;\n"
    "varying vec2	tex_coord;\n"
    "void main() {\n"
    "	tex_coord = vtx_tex0;\n"
    "	gl_Position = pvm * vec4(vtx_pos, 1.0);\n"
    "}\n";

static const char *frag_shader =
    "#version 120\n"
    "uniform sampler2D	tex;\n"
    "varying vec2	tex_coord;\n"
    "void main() {\n"
    "	gl_FragColor = texture2D(tex, tex_coord);\n"
    "}\n";

static const char *vert_shader410 =
    "#version 410\n"
    "uniform mat4			pvm;\n"
    "layout(location = %d) in vec3	vtx_pos;\n"
    "layout(location = %d) in vec2	vtx_tex0;\n"
    "layout(location = 0) out vec2	tex_coord;\n"
    "void main() {\n"
    "	tex_coord = vtx_tex0;\n"
    "	gl_Position = pvm * vec4(vtx_pos, 1.0);\n"
    "}\n";

static const char *frag_shader410 =
    "#version 410\n"
    "uniform sampler2D			tex;\n"
    "layout(location = 0) in vec2	tex_coord;\n"
    "layout(location = 0) out vec4	color_out;\n"
    "void main() {\n"
    "	color_out = texture(tex, tex_coord);\n"
    "}\n";

/*
 * Main mtcr_t worker thread. Simply waits around for the
 * required interval and fires off the rendering callback. This performs
 * no canvas clearing between calls, so the callback is responsible for
 * making sure its output canvas looks right.
 */
static void
worker(mtcr_t *mtcr)
{
	char name[32];

	snprintf(name, sizeof (name), "mtcr %dx%d", mtcr->w, mtcr->h);
	thread_set_name(name);

	mutex_enter(&mtcr->lock);

	while (!mtcr->shutdown) {
		render_surf_t *rs;

		if (!mtcr->one_shot_block) {
			if (mtcr->fps > 0) {
				cv_timedwait(&mtcr->cv, &mtcr->lock,
				    microclock() + SEC2USEC(1.0 / mtcr->fps));
			} else {
				cv_wait(&mtcr->cv, &mtcr->lock);
			}
		}
		if (mtcr->shutdown)
			break;
		mutex_exit(&mtcr->lock);

		/* always draw into the non-current texture */
		rs = &mtcr->rs[!mtcr->cur_rs];

		ASSERT(mtcr->render_cb != NULL);
		mtcr->render_cb(rs->cr, mtcr->w, mtcr->h, mtcr->userinfo);
		rs->chg = B_TRUE;

		mutex_enter(&mtcr->lock);
		mtcr->cur_rs = !mtcr->cur_rs;
		cv_broadcast(&mtcr->render_done_cv);
	}
	mutex_exit(&mtcr->lock);
}

static void
setup_vao(mtcr_t *mtcr)
{
	glGenVertexArrays(1, &mtcr->vao);
	glBindVertexArray(mtcr->vao);

	glGenBuffers(1, &mtcr->vtx_buf);

	glBindBuffer(GL_ARRAY_BUFFER, mtcr->vtx_buf);
	glutils_enable_vtx_attr_ptr(VTX_ATTRIB_POS, 3, GL_FLOAT,
	    GL_FALSE, sizeof (vtx_t), offsetof(vtx_t, pos));
	glutils_enable_vtx_attr_ptr(VTX_ATTRIB_TEX0, 2, GL_FLOAT,
	    GL_FALSE, sizeof (vtx_t), offsetof(vtx_t, tex0));

	mtcr->idx_buf = glutils_make_quads_IBO(4);
}

/*
 * Creates a new mtcr_t surface.
 * @param w Width of the rendered surface (in pixels).
 * @param h Height of the rendered surface (in pixels).
 * @param fps Framerate at which the surface should be rendered.
 *	This can be changed at any time later. Pass a zero fps value
 *	to make the renderer only run on-request (see mtcr_once).
 * @param init_cb An optional initialization callback that can be
 *	used to initialize private resources needed during rendering.
 *	Called once for every cairo_t instance (two instances for
 *	every mtcr_t, due to double-buffering).
 * @param render_cb A mandatory rendering callback. This is called for
 *	each rendered frame.
 * @param fini_cb An optional finalization callback that can be used
 *	to free resources allocated during init_cb.
 * @param userinfo An optional user info pointer for the callbacks.
 */
mtcr_t *
mtcr_init(unsigned w, unsigned h, double fps, mtcr_init_cb_t init_cb,
    mtcr_render_cb_t render_cb, mtcr_fini_cb_t fini_cb, void *userinfo)
{
	mtcr_t *mtcr = safe_calloc(1, sizeof (*mtcr));

	ASSERT(w != 0);
	ASSERT(h != 0);
	ASSERT(render_cb != NULL);

	mtcr->w = w;
	mtcr->h = h;
	mtcr->cur_rs = -1;
	mtcr->render_cb = render_cb;
	mtcr->fini_cb = fini_cb;
	mtcr->userinfo = userinfo;
	mtcr->fps = fps;

	mutex_init(&mtcr->lock);
	cv_init(&mtcr->cv);
	cv_init(&mtcr->render_done_cv);

	for (int i = 0; i < 2; i++) {
		render_surf_t *rs = &mtcr->rs[i];

		rs->surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		    mtcr->w, mtcr->h);
		rs->cr = cairo_create(rs->surf);
		if (init_cb != NULL && !init_cb(rs->cr, userinfo)) {
			mtcr_fini(mtcr);
			return (NULL);
		}
	}
	/* empty both surfaces to assure their data is populated */
	for (int i = 0; i < 2; i++) {
		cairo_set_operator(mtcr->rs[i].cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(mtcr->rs[i].cr);
		cairo_set_operator(mtcr->rs[i].cr, CAIRO_OPERATOR_OVER);
	}

	mtcr->last_draw.pos = NULL_VECT2;
	if (GLEW_VERSION_4_1) {
		char *vert_shader_text = sprintf_alloc(vert_shader410,
		    VTX_ATTRIB_POS, VTX_ATTRIB_TEX0);
		mtcr->shader = shader_prog_from_text("mtcr_shader",
		    vert_shader_text, frag_shader410, NULL);
		free(vert_shader_text);
	} else {
		mtcr->shader = shader_prog_from_text("mtcr_shader",
		    vert_shader, frag_shader, "vtx_pos", VTX_ATTRIB_POS,
		    "vtx_tex0", VTX_ATTRIB_TEX0, NULL);
	}
	VERIFY(mtcr->shader != 0);
	mtcr->shader_loc_pvm = glGetUniformLocation(mtcr->shader, "pvm");
	mtcr->shader_loc_tex = glGetUniformLocation(mtcr->shader, "tex");

	setup_vao(mtcr);

	VERIFY(thread_create(&mtcr->thr, worker, mtcr));
	mtcr->started = B_TRUE;

	return (mtcr);
}

void
mtcr_fini(mtcr_t *mtcr)
{
	if (mtcr->started) {
		mutex_enter(&mtcr->lock);
		mtcr->shutdown = B_TRUE;
		cv_broadcast(&mtcr->cv);
		mutex_exit(&mtcr->lock);
		thread_join(&mtcr->thr);
	}

	if (mtcr->vao != 0)
		glDeleteVertexArrays(1, &mtcr->vao);
	if (mtcr->vtx_buf != 0)
		glDeleteBuffers(1, &mtcr->vtx_buf);
	if (mtcr->idx_buf != 0)
		glDeleteBuffers(1, &mtcr->idx_buf);

	for (int i = 0; i < 2; i++) {
		render_surf_t *rs = &mtcr->rs[i];

		if (rs->cr != NULL) {
			if (mtcr->fini_cb != NULL)
				mtcr->fini_cb(rs->cr, mtcr->userinfo);
			cairo_destroy(rs->cr);
			cairo_surface_destroy(rs->surf);
		}
		if (rs->tex != 0)
			glDeleteTextures(1, &rs->tex);
	}
	if (mtcr->shader != 0)
		glDeleteProgram(mtcr->shader);

	mutex_destroy(&mtcr->lock);
	cv_destroy(&mtcr->cv);
	cv_destroy(&mtcr->render_done_cv);

	free(mtcr);
}

void
mtcr_set_fps(mtcr_t *mtcr, double fps)
{
	mutex_enter(&mtcr->lock);
	if (mtcr->fps != fps) {
		mtcr->fps = fps;
		cv_broadcast(&mtcr->cv);
	}
	mutex_exit(&mtcr->lock);
}

double
mtcr_get_fps(mtcr_t *mtcr)
{
	return (mtcr->fps);
}

/*
 * Fires the renderer off once to produce a new frame. This can be especially
 * useful for renderers with fps = 0, which are only invoked on request.
 */
void
mtcr_once(mtcr_t *mtcr)
{
	mutex_enter(&mtcr->lock);
	cv_broadcast(&mtcr->cv);
	mutex_exit(&mtcr->lock);
}

/*
 * Same as mtcr_once, but waits for the new frame to finish
 * rendering.
 */
void
mtcr_once_wait(mtcr_t *mtcr)
{
	mutex_enter(&mtcr->lock);
	mtcr->one_shot_block = B_TRUE;
	cv_broadcast(&mtcr->cv);
	cv_wait(&mtcr->render_done_cv, &mtcr->lock);
	mtcr->one_shot_block = B_FALSE;
	mutex_exit(&mtcr->lock);
}

static void
bind_tex_sync(mtcr_t *mtcr, render_surf_t *rs)
{
	ASSERT(rs->tex != 0);
	glBindTexture(GL_TEXTURE_2D, rs->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mtcr->w, mtcr->h, 0, GL_BGRA,
	    GL_UNSIGNED_BYTE, cairo_image_surface_get_data(rs->surf));
	rs->rdy = B_TRUE;
	rs->chg = B_FALSE;
}

static void
rs_tex_alloc(render_surf_t *rs)
{
	if (rs->tex != 0)
		return;
	glGenTextures(1, &rs->tex);
	glBindTexture(GL_TEXTURE_2D, rs->tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

static bool_t
bind_cur_tex(mtcr_t *mtcr)
{
	render_surf_t *rs = &mtcr->rs[mtcr->cur_rs];

	glActiveTexture(GL_TEXTURE0);
	rs_tex_alloc(rs);
	if (rs->chg)
		bind_tex_sync(mtcr, rs);
	glBindTexture(GL_TEXTURE_2D, rs->tex);

	return (B_TRUE);
}

static void
prepare_vtx_buffer(mtcr_t *mtcr, vect2_t pos, vect2_t size,
    double x1, double x2, double y1, double y2)
{
	vtx_t buf[4];

	if (VECT2_EQ(mtcr->last_draw.pos, pos) &&
	    VECT2_EQ(mtcr->last_draw.size, size) &&
	    mtcr->last_draw.x1 == x1 && mtcr->last_draw.x2 == x2 &&
	    mtcr->last_draw.y1 == y1 && mtcr->last_draw.y2 == y2)
		return;

	buf[0].pos[0] = pos.x;
	buf[0].pos[1] = pos.y;
	buf[0].pos[2] = 0;
	buf[0].tex0[0] = x1;
	buf[0].tex0[1] = y2;

	buf[1].pos[0] = pos.x;
	buf[1].pos[1] = pos.y + size.y;
	buf[1].pos[2] = 0;
	buf[1].tex0[0] = x1;
	buf[1].tex0[1] = y1;

	buf[2].pos[0] = pos.x + size.x;
	buf[2].pos[1] = pos.y + size.y;
	buf[2].pos[2] = 0;
	buf[2].tex0[0] = x2;
	buf[2].tex0[1] = y1;

	buf[3].pos[0] = pos.x + size.x;
	buf[3].pos[1] = pos.y;
	buf[3].pos[2] = 0;
	buf[3].tex0[0] = x2;
	buf[3].tex0[1] = y2;

	ASSERT(mtcr->vtx_buf != 0);
	glBindBuffer(GL_ARRAY_BUFFER, mtcr->vtx_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof (buf), buf, GL_STATIC_DRAW);

	mtcr->last_draw.pos = pos;
	mtcr->last_draw.size = size;
	mtcr->last_draw.x1 = x1;
	mtcr->last_draw.x2 = x2;
	mtcr->last_draw.y1 = y1;
	mtcr->last_draw.y2 = y2;
}

void
mtcr_draw(mtcr_t *mtcr, vect2_t pos, vect2_t size, const GLfloat *pvm)
{
	double x1 = 0, x2 = 1;
	double y1 = 0, y2 = 1;

	if (mtcr->cur_rs == -1) {
		mutex_exit(&mtcr->lock);
		return;
	}
	ASSERT3S(mtcr->cur_rs, >=, 0);
	ASSERT3S(mtcr->cur_rs, <, 2);

	mutex_enter(&mtcr->lock);
	if (!bind_cur_tex(mtcr)) {
		mutex_exit(&mtcr->lock);
		return;
	}
	mutex_exit(&mtcr->lock);

	glBindVertexArray(mtcr->vao);
	glEnable(GL_BLEND);
	glUseProgram(mtcr->shader);

	prepare_vtx_buffer(mtcr, pos, size, x1, x2, y1, y2);

	glUniformMatrix4fv(mtcr->shader_loc_pvm,
	    1, GL_FALSE, (const GLfloat *)pvm);
	glUniform1i(mtcr->shader_loc_tex, 0);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

API_EXPORT unsigned
mtcr_get_width(mtcr_t *mtcr)
{
	ASSERT(mtcr != NULL);
	return (mtcr->w);
}

API_EXPORT unsigned
mtcr_get_height(mtcr_t *mtcr)
{
	ASSERT(mtcr != NULL);
	return (mtcr->h);
}

void
mtcr_rounded_rectangle(cairo_t *cr, double x, double y,
    double w, double h, double radius)
{
	cairo_move_to(cr, x + radius, y);
	cairo_line_to(cr, x + w - radius, y);
	cairo_arc(cr, x + w - radius, y + radius, radius,
	    DEG2RAD(270), DEG2RAD(360));
	cairo_line_to(cr, x + w, y + h - radius);
	cairo_arc(cr, x + w - radius, y + h - radius, radius,
	    DEG2RAD(0), DEG2RAD(90));
	cairo_line_to(cr, x + radius, y + h);
	cairo_arc(cr, x + radius, y + h - radius, radius,
	    DEG2RAD(90), DEG2RAD(180));
	cairo_line_to(cr, x, y + radius);
	cairo_arc(cr, x + radius, y + radius, radius,
	    DEG2RAD(180), DEG2RAD(270));
}
