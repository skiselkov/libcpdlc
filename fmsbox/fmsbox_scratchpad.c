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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/cpdlc_alloc.h"
#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_string.h"

#include "fmsbox.h"
#include "fmsbox_impl.h"
#include "fmsbox_parsing.h"
#include "fmsbox_pos_pick.h"
#include "fmsbox_scratchpad.h"

#define	TEMP_MAX	99
#define	TEMP_MIN	-99

void
fmsbox_update_scratchpad(fmsbox_t *box)
{
	fmsbox_put_str(box, SCRATCHPAD_ROW, 0, false, FMS_COLOR_CYAN,
	    FMS_FONT_LARGE, "[");
	fmsbox_put_str(box, SCRATCHPAD_ROW, 1, false, FMS_COLOR_WHITE,
	    FMS_FONT_LARGE, "%s", box->scratchpad);
	fmsbox_put_str(box, SCRATCHPAD_ROW, 0, true, FMS_COLOR_CYAN,
	    FMS_FONT_LARGE, "]");
}

bool
fmsbox_scratchpad_is_delete(fmsbox_t *box)
{
	ASSERT(box != NULL);
	return (strcmp(box->scratchpad, "DELETE") == 0);
}

void
fmsbox_scratchpad_clear(fmsbox_t *box)
{
	ASSERT(box != NULL);
	memset(box->scratchpad, 0, sizeof (box->scratchpad));
}

void
fmsbox_scratchpad_pm(fmsbox_t *box)
{
	int n;

	ASSERT(box != NULL);

	n = strlen(box->scratchpad);
	if (n == 0) {
		cpdlc_strlcpy(box->scratchpad, "+", sizeof (box->scratchpad));
	} else {
		if (box->scratchpad[n - 1] == '+') {
			box->scratchpad[n - 1] = '-';
		} else if (box->scratchpad[n - 1] == '-') {
			box->scratchpad[n - 1] = '+';
		} else if (n + 1 < (int)sizeof (box->scratchpad)) {
			strncat(box->scratchpad, "+",
			    sizeof (box->scratchpad) - 1);
		}
	}
}

void
fmsbox_scratchpad_xfer(fmsbox_t *box, char *dest, size_t cap, bool allow_mod)
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

bool
fmsbox_scratchpad_xfer_multi(fmsbox_t *box, void *userinfo, size_t buf_sz,
    fmsbox_parse_func_t parse_func, fmsbox_insert_func_t insert_func,
    fmsbox_delete_func_t delete_func, fmsbox_read_func_t read_func)
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
			char substr[SCRATCHPAD_MAX + 1];
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
		free(data_buf);
		memset(box->scratchpad, 0, sizeof (box->scratchpad));
	}

	fmsbox_set_error(box, error);

	return (error == NULL);
}

void
fmsbox_scratchpad_xfer_hdg(fmsbox_t *box, fms_hdg_t *hdg)
{
	char buf[8] = { 0 };
	int new_hdg;
	const char *error = NULL;

	ASSERT(box != NULL);
	ASSERT(hdg != NULL);

	if (hdg->set) {
		if (hdg->tru)
			snprintf(buf, sizeof (buf), "%03dT", hdg->hdg);
		else
			snprintf(buf, sizeof (buf), "%03d", hdg->hdg);
	}
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	if (strlen(buf) == 0) {
		hdg->set = false;
	} else if (sscanf(buf, "%d", &new_hdg) == 1 && new_hdg >= 0 &&
	    new_hdg <= 360) {
		char last = buf[strlen(buf) - 1];
		hdg->set = true;
		hdg->hdg = new_hdg % 360;
		if (!isdigit(last)) {
			if (last == 'T')
				hdg->tru = true;
			else if (last == 'M')
				hdg->tru = false;
			else
				error = "FORMAT ERROR";
		} else {
			hdg->tru = false;
		}
	} else {
		error = "FORMAT ERROR";
	}

	fmsbox_set_error(box, error);
}

void
fmsbox_scratchpad_xfer_alt(fmsbox_t *box, cpdlc_arg_t *alt)
{
	char buf[8] = { 0 };

	ASSERT(box != NULL);
	ASSERT(alt != NULL);

	if (alt->alt.alt != 0)
		fmsbox_print_alt(alt, buf, sizeof (buf));
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	if (strlen(buf) != 0)
		fmsbox_set_error(box, fmsbox_parse_alt(buf, 0, alt));
	else
		memset(alt, 0, sizeof (*alt));
}

void
fmsbox_scratchpad_xfer_pos_impl(fmsbox_t *box, fms_pos_t *pos)
{
	char buf[32];

	fmsbox_print_pos(pos, buf, sizeof (buf), POS_PRINT_NORM);
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	fmsbox_set_error(box, fmsbox_parse_pos(buf, pos));
}

void
fmsbox_scratchpad_xfer_pos(fmsbox_t *box, fms_pos_t *pos,
    unsigned ret_page, pos_pick_done_cb_t done_cb)
{
	ASSERT(box != NULL);
	ASSERT(pos != NULL);
	ASSERT3U(ret_page, <, FMS_NUM_PAGES);
	ASSERT(done_cb != NULL);

	if (pos->set)
		fmsbox_scratchpad_xfer_pos_impl(box, pos);
	else
		fmsbox_pos_pick_start(box, done_cb, ret_page, pos);
}

void
fmsbox_scratchpad_xfer_uint(fmsbox_t *box, unsigned *value, bool *set,
    unsigned minval, unsigned maxval)
{
	char buf[16] = { 0 };

	ASSERT(box != NULL);
	ASSERT(value != NULL);
	ASSERT(set != NULL);

	if (*set)
		snprintf(buf, sizeof (buf), "%d", *value);
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	if (strlen(buf) != 0) {
		unsigned tmp;

		if (sscanf(buf, "%d", &tmp) != 1 || tmp < minval ||
		    tmp > maxval) {
			fmsbox_set_error(box, "FORMAT ERROR");
		} else {
			*value = tmp;
			*set = true;
		}
	} else {
		*set = false;
	}
}

void
fmsbox_scratchpad_xfer_time(fmsbox_t *box, fms_time_t *t)
{
	char buf[8] = { 0 };
	int hrs, mins;

	ASSERT(box != NULL);
	ASSERT(t != NULL);

	if (t->set)
		snprintf(buf, sizeof (buf), "%02d%02d", t->hrs, t->mins);
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	if (strlen(buf) != 0) {
		if (!fmsbox_parse_time(buf, &hrs, &mins)) {
			fmsbox_set_error(box, "FORMAT ERROR");
		} else {
			t->hrs = hrs;
			t->mins = mins;
			t->set = true;
		}
	} else {
		t->set = false;
	}
}

static bool
parse_dir(char *buf, cpdlc_dir_t *dir)
{
	char first, last, c;

	ASSERT(buf != NULL);
	ASSERT(strlen(buf) != 0);
	ASSERT(dir != NULL);

	first = buf[0];
	last = buf[strlen(buf) - 1];
	if (!isdigit(first)) {
		c = first;
		memmove(&buf[0], &buf[1], strlen(buf));
	} else if (!isdigit(last)) {
		c = last;
	} else {
		return (false);
	}
	if (c == 'L')
		*dir = CPDLC_DIR_LEFT;
	else if (c == 'R')
		*dir = CPDLC_DIR_RIGHT;
	else
		return (false);
	return (true);
}

void
fmsbox_scratchpad_xfer_offset(fmsbox_t *box, fms_off_t *off)
{
	char buf[8];

	ASSERT(box != NULL);
	ASSERT(off != NULL);

	fmsbox_print_off(off, buf, sizeof (buf));
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	if (strlen(buf) != 0) {
		unsigned nm;
		cpdlc_dir_t dir;

		if (strlen(buf) < 2 || !parse_dir(buf, &dir) ||
		    sscanf(buf, "%d", &nm) != 1 || nm == 0 || nm > 999) {
			fmsbox_set_error(box, "FORMAT ERROR");
		} else {
			off->dir = dir;
			off->nm = nm;
		}
	} else {
		off->nm = 0;
	}
}

void
fmsbox_scratchpad_xfer_spd(fmsbox_t *box, cpdlc_arg_t *spd)
{
	char buf[8] = { 0 };
	cpdlc_arg_t new_spd = { .spd.spd = 0 };
	const char *error;

	ASSERT(box != NULL);
	ASSERT(spd != NULL);

	if (spd->spd.spd != 0)
		fmsbox_print_spd(spd, buf, sizeof (buf));
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	error = fmsbox_parse_spd(buf, 0, &new_spd);
	fmsbox_set_error(box, error);
	if (error == NULL)
		memcpy(spd, &new_spd, sizeof (new_spd));
}

void
fmsbox_scratchpad_xfer_temp(fmsbox_t *box, fms_temp_t *temp)
{
	char buf[8] = { 0 };
	int new_temp;

	ASSERT(box != NULL);
	ASSERT(temp != NULL);

	if (temp->set)
		snprintf(buf, sizeof (buf), "%d", temp->temp);
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	if (strlen(buf) == 0) {
		temp->set = false;
	} else {
		int mult = 1;

		if (buf[0] == 'P' || buf[0] == 'M') {
			if (buf[0] == 'M')
				mult = -1;
			memmove(&buf[0], &buf[1], sizeof (buf) - 1);
		}

		if (sscanf(buf, "%d", &new_temp) == 1 &&
		    new_temp >= TEMP_MIN && new_temp <= TEMP_MAX) {
			temp->set = true;
			temp->temp = new_temp * mult;
		} else {
			fmsbox_set_error(box, "FORMAT ERROR");
		}
	}
}

void
fmsbox_scratchpad_xfer_wind(fmsbox_t *box, fms_wind_t *wind)
{
	fmsbox_scratchpad_xfer_multi(box, wind, sizeof (fms_wind_t),
	    fmsbox_parse_wind, fmsbox_insert_wind_block,
	    fmsbox_delete_wind, fmsbox_read_wind_block);
}
