/*
 * Copyright 2022 Saso Kiselkov
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

bool
fans_scratchpad_is_empty(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->scratchpad[0] == '\0');
}

const char *
fans_scratchpad_get(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	return (box->scratchpad);
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
		cpdlc_strlcpy(box->scratchpad, "-", sizeof (box->scratchpad));
	} else {
		if (box->scratchpad[n - 1] == '+') {
			box->scratchpad[n - 1] = '-';
		} else if (box->scratchpad[n - 1] == '-') {
			box->scratchpad[n - 1] = '+';
		} else if (n + 1 < (int)sizeof (box->scratchpad)) {
			strncat(box->scratchpad, "-",
			    sizeof (box->scratchpad) - 1);
		}
	}
}

bool
fans_scratchpad_xfer_auto(fans_t *box, char *dest, const char *autobuf,
    size_t cap, bool allow_mod, bool *read_back)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(dest != NULL);
	/* autobuf can be NULL */
	/* read_back can be NULL */

	if (read_back != NULL)
		*read_back = true;
	if (!allow_mod) {
		if (box->scratchpad[0] != '\0') {
			fans_set_error(box, FANS_ERR_NO_ENTRY_ALLOWED);
			return (true);
		}
		if (dest[0] != '\0') {
			cpdlc_strlcpy(box->scratchpad, dest,
			    sizeof (box->scratchpad));
		} else if (autobuf != NULL) {
			cpdlc_strlcpy(box->scratchpad, autobuf,
			    sizeof (box->scratchpad));
		}
		return (true);
	}
	if (fans_scratchpad_is_delete(box)) {
		if (dest[0] == '\0') {
			fans_set_error(box, FANS_ERR_INVALID_DELETE);
			return (false);
		}
		memset(dest, 0, cap);
		if (read_back != NULL)
			*read_back = false;
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
		if (read_back != NULL)
			*read_back = false;
	}
	return (true);
}

bool
fans_scratchpad_xfer(fans_t *box, char *dest, size_t cap, bool allow_mod,
    bool *read_back)
{
	return (fans_scratchpad_xfer_auto(box, dest, NULL, cap, allow_mod,
	    read_back));
}

bool
fans_scratchpad_xfer_multi(fans_t *box, void *userinfo, size_t buf_sz,
    fans_parse_func_t parse_func, fans_insert_func_t insert_func,
    fans_delete_func_t delete_func, fans_read_func_t read_func,
    bool *read_back)
{
	fans_err_t error = FANS_ERR_NONE;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(buf_sz != 0);
	CPDLC_ASSERT(parse_func != NULL);
	CPDLC_ASSERT(insert_func != NULL);
	CPDLC_ASSERT(delete_func != NULL);
	/* read_back can be NULL */

	if (read_back != NULL)
		*read_back = true;
	if (fans_scratchpad_is_empty(box)) {
		if (read_func != NULL) {
			char str[READ_FUNC_BUF_SZ] = { 0 };

			read_func(box, userinfo, str);
			if (strlen(str) != 0) {
				cpdlc_strlcpy(box->scratchpad, str,
				    sizeof (box->scratchpad));
			}
		}
	} else if (strcmp(box->scratchpad, "DELETE") == 0) {
		error = delete_func(box, userinfo);
		if (read_back != NULL)
			*read_back = false;
	} else {
		const char *start = box->scratchpad;
		const char *end = start + strlen(start);
		void *data_buf = safe_malloc(buf_sz);

		if (read_back != NULL)
			*read_back = false;

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
			if (error != FANS_ERR_NONE)
				break;
			error = insert_func(box, field_nr, data_buf, userinfo);
			if (error != FANS_ERR_NONE)
				break;
		}
		free(data_buf);
	}
	fans_set_error(box, error);
	return (error == FANS_ERR_NONE);
}

bool
fans_scratchpad_xfer_hdg(fans_t *box, fms_hdg_t *hdg, bool *read_back)
{
	char buf[8] = { 0 };
	int new_hdg;
	fans_err_t error = FANS_ERR_NONE;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(hdg != NULL);
	/* read_back can be NULL */

	if (read_back != NULL)
		*read_back = true;
	if (hdg->set) {
		if (hdg->tru)
			snprintf(buf, sizeof (buf), "%03dT", hdg->hdg);
		else
			snprintf(buf, sizeof (buf), "%03d", hdg->hdg);
	}
	if (!fans_scratchpad_xfer(box, buf, sizeof (buf), true, read_back))
		return (false);
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
				error = FANS_ERR_INVALID_ENTRY;
		} else {
			hdg->tru = false;
		}
	} else {
		error = FANS_ERR_INVALID_ENTRY;
	}
	fans_set_error(box, error);
	return (error == FANS_ERR_NONE);
}

bool
fans_scratchpad_xfer_alt(fans_t *box, cpdlc_arg_t *useralt,
    const cpdlc_arg_t *autoalt, bool *read_back)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };
	fans_err_t error = FANS_ERR_NONE;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(useralt != NULL);
	/* read_back can be NULL */

	if (useralt->alt.alt != 0)
		fans_print_alt(useralt, userbuf, sizeof (userbuf), false);
	if (autoalt != NULL && autoalt->alt.alt != 0)
		fans_print_alt(autoalt, autobuf, sizeof (autobuf), false);
	if (!fans_scratchpad_xfer_auto(box, userbuf, autobuf,
	    sizeof (userbuf), true, read_back)) {
		return (false);
	}
	if (error == FANS_ERR_NONE) {
		if (strlen(userbuf) != 0)
			error = fans_parse_alt(userbuf, 0, useralt);
		else
			memset(useralt, 0, sizeof (*useralt));
	}
	fans_set_error(box, error);
	return (error == FANS_ERR_NONE);
}

bool
fans_scratchpad_xfer_pos_impl(fans_t *box, fms_pos_t *pos, bool *read_back)
{
	char buf[32];

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(pos != NULL);
	/* read_back can be NULL */

	fans_print_pos(pos, buf, sizeof (buf), POS_PRINT_NORM);
	if (fans_scratchpad_xfer(box, buf, sizeof (buf), true, read_back)) {
		if (!(*read_back)) {
			fans_err_t err = fans_parse_pos(buf, pos);
			if (err != FANS_ERR_NONE) {
				fans_set_error(box, err);
				return (false);
			}
		}
		return (true);
	} else {
		return (false);
	}
}

bool
fans_scratchpad_xfer_pos(fans_t *box, fms_pos_t *pos,
    unsigned ret_page, pos_pick_done_cb_t done_cb, bool *read_back)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(pos != NULL);
	CPDLC_ASSERT3U(ret_page, <, FMS_NUM_PAGES);
	CPDLC_ASSERT(done_cb != NULL);

	if (pos->set) {
		return (fans_scratchpad_xfer_pos_impl(box, pos, read_back));
	} else {
		if (read_back != NULL)
			*read_back = true;
		fans_pos_pick_start(box, done_cb, ret_page, pos);
		return (true);
	}
}

bool
fans_scratchpad_xfer_uint(fans_t *box, unsigned *value, bool *set,
    unsigned minval, unsigned maxval, bool *read_back)
{
	char buf[16] = { 0 };
	fans_err_t error = FANS_ERR_NONE;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(value != NULL);
	CPDLC_ASSERT(set != NULL);
	/* read_back can be NULL */

	if (*set)
		snprintf(buf, sizeof (buf), "%d", *value);
	if (!fans_scratchpad_xfer(box, buf, sizeof (buf), true, read_back))
		return (false);
	if (error == FANS_ERR_NONE) {
		if (strlen(buf) != 0) {
			unsigned tmp;

			if (sscanf(buf, "%d", &tmp) != 1 || tmp < minval ||
			    tmp > maxval) {
				error = FANS_ERR_INVALID_ENTRY;
			} else {
				*value = tmp;
				*set = true;
			}
		} else {
			*set = false;
		}
	}
	fans_set_error(box, error);
	return (error == FANS_ERR_NONE);
}

bool
fans_scratchpad_xfer_time(fans_t *box, fms_time_t *usertime,
    const fms_time_t *autotime, bool *read_back)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };
	int hrs, mins;
	fans_err_t error = FANS_ERR_NONE;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(usertime != NULL);
	/* read_back can be NULL */

	if (usertime->set) {
		snprintf(userbuf, sizeof (userbuf), "%02d%02d", usertime->hrs,
		    usertime->mins);
	}
	if (autotime != NULL && autotime->set) {
		snprintf(autobuf, sizeof (autobuf), "%02d%02d", autotime->hrs,
		    autotime->mins);
	}
	if (!fans_scratchpad_xfer_auto(box, userbuf, autobuf,
	    sizeof (userbuf), true, read_back)) {
		return (false);
	}
	if (error == FANS_ERR_NONE) {
		if (strlen(userbuf) != 0) {
			if (!fans_parse_time(userbuf, &hrs, &mins)) {
				error = FANS_ERR_INVALID_ENTRY;
			} else {
				usertime->hrs = hrs;
				usertime->mins = mins;
				usertime->set = true;
			}
		} else {
			usertime->set = false;
		}
	}
	fans_set_error(box, error);
	return (error == FANS_ERR_NONE);
}

static bool
parse_dir(char *buf, fms_off_t *useroff, cpdlc_dir_t *dir)
{
	char first, last, c;

	CPDLC_ASSERT(buf != NULL);
	CPDLC_ASSERT(strlen(buf) != 0);
	CPDLC_ASSERT(useroff != NULL);
	CPDLC_ASSERT(dir != NULL);

	first = buf[0];
	last = buf[strlen(buf) - 1];
	if (!isdigit(first)) {
		c = first;
		memmove(&buf[0], &buf[1], strlen(buf));
	} else if (!isdigit(last)) {
		c = last;
		buf[strlen(buf) - 1] = '\0';
	} else if (useroff->nm == 0 || useroff->dir == CPDLC_DIR_ANY)
		return (false);
	if (c == 'L')
		*dir = CPDLC_DIR_LEFT;
	else if (c == 'R')
		*dir = CPDLC_DIR_RIGHT;
	else if (useroff->nm != 0 && useroff->dir != CPDLC_DIR_ANY)
		*dir = useroff->dir;
	else
		return (false);
	return (true);
}

bool
fans_scratchpad_xfer_offset(fans_t *box, fms_off_t *useroff,
    const fms_off_t *autooff, bool *read_back)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };
	fans_err_t error = FANS_ERR_NONE;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(useroff != NULL);
	/* read_back can be NULL */

	fans_print_off(useroff, userbuf, sizeof (userbuf));
	if (autooff != NULL)
		fans_print_off(autooff, autobuf, sizeof (autobuf));
	
	if (!fans_scratchpad_xfer_auto(box, userbuf, autobuf,
	    sizeof (userbuf), true, read_back)) {
		return (false);
	}
	if (error == FANS_ERR_NONE) {
		if (strlen(userbuf) != 0) {
			float nm = 0;
			cpdlc_dir_t dir;

			if (strlen(userbuf) == 0 ||
			    !parse_dir(userbuf, useroff, &dir) ||
			    (sscanf(userbuf, "%f", &nm) != 1 &&
			    useroff->nm == 0) ||
			    (nm <= 0 && useroff->nm == 0) || nm > 999) {
				error = FANS_ERR_INVALID_ENTRY;
			} else {
				useroff->dir = dir;
				if (nm == 0)
					nm = useroff->nm;
				useroff->nm = nm;
			}
		} else {
			useroff->nm = 0;
		}
	}
	fans_set_error(box, error);
	return (error == FANS_ERR_NONE);
}

bool
fans_scratchpad_xfer_spd(fans_t *box, cpdlc_arg_t *userspd,
    const cpdlc_arg_t *autospd, bool *read_back)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };
	fans_err_t error = FANS_ERR_NONE;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(userspd != NULL);
	/* read_back can be NULL */

	if (userspd->spd.spd != 0) {
		fans_print_spd(userspd, userbuf, sizeof (userbuf), false,
		    false);
	}
	if (autospd != NULL && autospd->spd.spd != 0) {
		fans_print_spd(autospd, autobuf, sizeof (autobuf), false,
		    false);
	}
	if (!fans_scratchpad_xfer_auto(box, userbuf, autobuf,
	    sizeof (userbuf), true, read_back)) {
		return (false);
	}
	if (error == FANS_ERR_NONE) {
		if (strlen(userbuf) != 0) {
			cpdlc_arg_t new_spd = { .spd.spd = 0 };

			error = fans_parse_spd(userbuf, 0, &new_spd);
			if (error == FANS_ERR_NONE)
				memcpy(userspd, &new_spd, sizeof (new_spd));
		} else {
			memset(userspd, 0, sizeof (*userspd));
		}
	}
	fans_set_error(box, error);
	return (error == FANS_ERR_NONE);
}

bool
fans_scratchpad_xfer_temp(fans_t *box, fms_temp_t *usertemp,
    const fms_temp_t *autotemp, bool *read_back)
{
	char userbuf[8] = { 0 }, autobuf[8] = { 0 };
	int new_temp;
	fans_err_t error = FANS_ERR_NONE;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(usertemp != NULL);
	/* read_back can be NULL */

	if (usertemp->set)
		snprintf(userbuf, sizeof (userbuf), "%d", usertemp->temp);
	if (autotemp != NULL && autotemp->set)
		snprintf(autobuf, sizeof (autobuf), "%d", autotemp->temp);
	if (!fans_scratchpad_xfer_auto(box, userbuf, autobuf,
	    sizeof (userbuf), true, read_back)) {
		return (false);
	}
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
			error = FANS_ERR_INVALID_ENTRY;
		}
	} else {
		usertemp->set = false;
	}
	fans_set_error(box, error);
	return (error == FANS_ERR_NONE);
}

bool
fans_scratchpad_xfer_wind(fans_t *box, fms_wind_t *wind, bool *read_back)
{
	return (fans_scratchpad_xfer_multi(box, wind, sizeof (fms_wind_t),
	    fans_parse_wind, fans_insert_wind_block,
	    fans_delete_wind, fans_read_wind_block, read_back));
}
