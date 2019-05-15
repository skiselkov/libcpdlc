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
#include "fmsbox_scratchpad.h"

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
	return (strcmp(box->scratchpad, "DELETE") == 0);
}

void
fmsbox_scratchpad_clear(fmsbox_t *box)
{
	memset(box->scratchpad, 0, sizeof (box->scratchpad));
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

void
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
}

void
fmsbox_scratchpad_xfer_hdg(fmsbox_t *box, bool *hdg_set, unsigned *hdg,
    bool *hdg_true)
{
	char buf[8] = { 0 };
	int new_hdg;
	const char *error = NULL;

	ASSERT(box != NULL);
	ASSERT(hdg_set != NULL);
	ASSERT(hdg != NULL);
	ASSERT(hdg_true != NULL);

	if (*hdg_set) {
		if (*hdg_true)
			snprintf(buf, sizeof (buf), "%03dT", *hdg);
		else
			snprintf(buf, sizeof (buf), "%03d", *hdg);
	}
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	if (strlen(buf) == 0) {
		*hdg_set = false;
	} else if (sscanf(buf, "%d", &new_hdg) == 1 && new_hdg >= 0 &&
	    new_hdg <= 360) {
		char last = buf[strlen(buf) - 1];
		*hdg_set = true;
		*hdg = new_hdg % 360;
		if (!isdigit(last)) {
			if (last == 'T')
				*hdg_true = true;
			else if (last == 'M')
				*hdg_true = false;
			else
				error = "FORMAT ERROR";
		} else {
			*hdg_true = false;
		}
	} else {
		error = "FORMAT ERROR";
	}

	fmsbox_set_error(box, error);
}

void
fmsbox_scratchpad_xfer_pos(fmsbox_t *box, fms_pos_t *pos)
{
	char buf[32];

	ASSERT(box != NULL);
	ASSERT(pos != NULL);

	fmsbox_print_pos(pos, buf, sizeof (buf), POS_PRINT_NORM);
	fmsbox_scratchpad_xfer(box, buf, sizeof (buf), true);
	fmsbox_set_error(box, fmsbox_scan_pos(buf, pos));
}
