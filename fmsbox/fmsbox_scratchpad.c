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
