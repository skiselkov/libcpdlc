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

#include "fans.h"
#include "fans_impl.h"
#include "fans_parsing.h"
#include "fans_pos_pick.h"
#include "fans_scratchpad.h"

#define	TEMP_MAX	99
#define	TEMP_MIN	-99

void
fans_update_scratchpad(fans_t *box)
{
	fans_put_str(box, SCRATCHPAD_ROW, 0, false, FMS_COLOR_CYAN,
	    FMS_FONT_LARGE, "[");
	fans_put_str(box, SCRATCHPAD_ROW, 1, false, FMS_COLOR_WHITE,
	    FMS_FONT_LARGE, "%s", box->scratchpad);
	fans_put_str(box, SCRATCHPAD_ROW, 0, true, FMS_COLOR_CYAN,
	    FMS_FONT_LARGE, "]");
}

bool
fans_scratchpad_is_delete(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (strcmp(box->scratchpad, "DELETE") == 0);
}

void
fans_scratchpad_clear(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	memset(box->scratchpad, 0, sizeof (box->scratchpad));
}

void
fans_scratchpad_pm(fans_t *box)
{
	int n;

	CPDLC_ASSERT(box != NULL);

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
fans_scratchpad_xfer_auto(fans_t *box, char *dest, const char *autobuf,
    size_t cap, bool allow_mod)
{
	if (!allow_mod) {
		if (box->scratchpad[0] == '\0') {
			if (dest[0] != '\0') {
				cpdlc_strlcpy(box->scratchpad, dest,
				    sizeof (box->scratchpad));
			} else if (autobuf != NULL) {
				cpdlc_strlcpy(box->scratchpad, autobuf,
				    sizeof (box->scratchpad));
			}
		} else {
			fans_set_error(box, "MOD NOT ALLOWED");
		}
		return;
	}
	if (fans_scratchpad_is_delete(box)) {
		memset(dest, 0, cap);
		memset(box->scratchpad, 0, sizeof (box->scratchpad));
	} else if (box->scratchpad[0] == '\0') {
		if (dest[0] != '\0') {
			cpdlc_strlcpy(box->scratchpad, dest,
			    sizeof (box->scratchpad));
		} else if (autobuf != NULL) {
			cpdlc_strlcpy(box->scratchpad, autobuf,
			    sizeof (box->scratchpad));
		}
	} else {
		cpdlc_strlcpy(dest, box->scratchpad, cap);
		memset(box->scratchpad, 0, sizeof (box->scratchpad));
	}
}

void
fans_scratchpad_xfer(fans_t *box, char *dest, size_t cap, bool allow_mod)
{
	fans_scratchpad_xfer_auto(box, dest, NULL, cap, allow_mod);
}

bool
fans_scratchpad_xfer_multi(fans_t *box, void *userinfo, size_t buf_sz,
    fans_parse_func_t parse_func, fans_insert_func_t insert_func,
    fans_delete_func_t delete_func, fans_read_func_t read_func)
{
	const char *error = NULL;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(buf_sz != 0);
	CPDLC_ASSERT(parse_func != NULL);
	CPDLC_ASSERT(insert_func != NULL);
	CPDLC_ASSERT(delete_func != NULL);

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

	fans_set_error(box, error);

	return (error == NULL);
}

void
fans_scratchpad_xfer_hdg(fans_t *box, fms_hdg_t *hdg)
{
	char buf[8] = { 0 };
	int new_hdg;
	const char *error = NULL;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(hdg != NULL);

	if (hdg->set) {
		if (hdg->tru)
			snprintf(buf, sizeof (buf), "%03dT", hdg->hdg);
		else
			snprintf(buf, sizeof (buf), "%03d", hdg->hdg);
	}
	fans_scratchpad_xfer(box, buf, sizeof (buf), true);
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

	fans_set_error(box, error);
}

void
fans_scratchpad_xfer_alt(fans_t *box, cpdlc_arg_t *useralt,
    const cpdlc_arg_t *autoalt)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(useralt != NULL);

	if (useralt->alt.alt != 0)
		fans_print_alt(useralt, userbuf, sizeof (userbuf));
	if (autoalt != NULL && autoalt->alt.alt != 0)
		fans_print_alt(autoalt, autobuf, sizeof (autobuf));
	fans_scratchpad_xfer_auto(box, userbuf, autobuf, sizeof (userbuf),
	    true);
	if (strlen(userbuf) != 0)
		fans_set_error(box, fans_parse_alt(userbuf, 0, useralt));
	else
		memset(useralt, 0, sizeof (*useralt));
}

void
fans_scratchpad_xfer_pos_impl(fans_t *box, fms_pos_t *pos)
{
	char buf[32];

	fans_print_pos(pos, buf, sizeof (buf), POS_PRINT_NORM);
	fans_scratchpad_xfer(box, buf, sizeof (buf), true);
	fans_set_error(box, fans_parse_pos(buf, pos));
}

void
fans_scratchpad_xfer_pos(fans_t *box, fms_pos_t *pos,
    unsigned ret_page, pos_pick_done_cb_t done_cb)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(pos != NULL);
	CPDLC_ASSERT3U(ret_page, <, FMS_NUM_PAGES);
	CPDLC_ASSERT(done_cb != NULL);

	if (pos->set)
		fans_scratchpad_xfer_pos_impl(box, pos);
	else
		fans_pos_pick_start(box, done_cb, ret_page, pos);
}

void
fans_scratchpad_xfer_uint(fans_t *box, unsigned *value, bool *set,
    unsigned minval, unsigned maxval)
{
	char buf[16] = { 0 };

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(value != NULL);
	CPDLC_ASSERT(set != NULL);

	if (*set)
		snprintf(buf, sizeof (buf), "%d", *value);
	fans_scratchpad_xfer(box, buf, sizeof (buf), true);
	if (strlen(buf) != 0) {
		unsigned tmp;

		if (sscanf(buf, "%d", &tmp) != 1 || tmp < minval ||
		    tmp > maxval) {
			fans_set_error(box, "FORMAT ERROR");
		} else {
			*value = tmp;
			*set = true;
		}
	} else {
		*set = false;
	}
}

void
fans_scratchpad_xfer_time(fans_t *box, fms_time_t *usertime,
    const fms_time_t *autotime)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };
	int hrs, mins;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(usertime != NULL);

	if (usertime->set) {
		snprintf(userbuf, sizeof (userbuf), "%02d%02d", usertime->hrs,
		    usertime->mins);
	}
	if (autotime != NULL && autotime->set) {
		snprintf(autobuf, sizeof (autobuf), "%02d%02d", autotime->hrs,
		    autotime->mins);
	}
	fans_scratchpad_xfer_auto(box, userbuf, autobuf, sizeof (userbuf),
	    true);
	if (strlen(userbuf) != 0) {
		if (!fans_parse_time(userbuf, &hrs, &mins)) {
			fans_set_error(box, "FORMAT ERROR");
		} else {
			usertime->hrs = hrs;
			usertime->mins = mins;
			usertime->set = true;
		}
	} else {
		usertime->set = false;
	}
}

static bool
parse_dir(char *buf, cpdlc_dir_t *dir)
{
	char first, last, c;

	CPDLC_ASSERT(buf != NULL);
	CPDLC_ASSERT(strlen(buf) != 0);
	CPDLC_ASSERT(dir != NULL);

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
fans_scratchpad_xfer_offset(fans_t *box, fms_off_t *useroff,
    const fms_off_t *autooff)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(useroff != NULL);

	fans_print_off(useroff, userbuf, sizeof (userbuf));
	if (autooff != NULL)
		fans_print_off(autooff, autobuf, sizeof (autobuf));
	
	fans_scratchpad_xfer_auto(box, userbuf, autobuf, sizeof (userbuf),
	    true);
	if (strlen(userbuf) != 0) {
		unsigned nm;
		cpdlc_dir_t dir;

		if (strlen(userbuf) < 2 || !parse_dir(userbuf, &dir) ||
		    sscanf(userbuf, "%d", &nm) != 1 || nm == 0 || nm > 999) {
			fans_set_error(box, "FORMAT ERROR");
		} else {
			useroff->dir = dir;
			useroff->nm = nm;
		}
	} else {
		useroff->nm = 0;
	}
}

void
fans_scratchpad_xfer_spd(fans_t *box, cpdlc_arg_t *userspd,
    const cpdlc_arg_t *autospd)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(userspd != NULL);

	if (userspd->spd.spd != 0)
		fans_print_spd(userspd, userbuf, sizeof (userbuf));
	if (autospd != NULL && autospd->spd.spd != 0)
		fans_print_spd(autospd, autobuf, sizeof (autobuf));
	fans_scratchpad_xfer_auto(box, userbuf, autobuf, sizeof (userbuf),
	    true);
	if (strlen(userbuf) != 0) {
		cpdlc_arg_t new_spd = { .spd.spd = 0 };
		const char *error = fans_parse_spd(userbuf, 0, &new_spd);

		fans_set_error(box, error);
		if (error == NULL)
			memcpy(userspd, &new_spd, sizeof (new_spd));
	} else {
		memset(userspd, 0, sizeof (*userspd));
	}
}

void
fans_scratchpad_xfer_temp(fans_t *box, fms_temp_t *usertemp,
    const fms_temp_t *autotemp)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };
	int new_temp;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(usertemp != NULL);

	if (usertemp->set)
		snprintf(userbuf, sizeof (userbuf), "%d", usertemp->temp);
	if (autotemp != NULL && autotemp->set)
		snprintf(autobuf, sizeof (autobuf), "%d", autotemp->temp);
	fans_scratchpad_xfer_auto(box, userbuf, autobuf, sizeof (userbuf),
	    true);
	if (strlen(userbuf) != 0) {
		int mult = 1;

		if (userbuf[0] == 'P' || userbuf[0] == 'M') {
			if (userbuf[0] == 'M')
				mult = -1;
			memmove(&userbuf[0], &userbuf[1], sizeof (userbuf) - 1);
		}

		if (sscanf(userbuf, "%d", &new_temp) == 1 &&
		    new_temp >= TEMP_MIN && new_temp <= TEMP_MAX) {
			usertemp->set = true;
			usertemp->temp = new_temp * mult;
		} else {
			fans_set_error(box, "FORMAT ERROR");
		}
	} else {
		usertemp->set = false;
	}
}

void
fans_scratchpad_xfer_wind(fans_t *box, fms_wind_t *wind)
{
	fans_scratchpad_xfer_multi(box, wind, sizeof (fms_wind_t),
	    fans_parse_wind, fans_insert_wind_block,
	    fans_delete_wind, fans_read_wind_block);
}
