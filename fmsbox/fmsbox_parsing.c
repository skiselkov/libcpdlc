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
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../src/cpdlc_alloc.h"
#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_string.h"
#include "fmsbox.h"
#include "fmsbox_impl.h"
#include "fmsbox_parsing.h"

bool
fmsbox_parse_time(const char *buf, int *hrs_p, int *mins_p)
{
	int num, hrs, mins;

	ASSERT(buf != NULL);

	if (strlen(buf) != 4 || !isdigit(buf[0]) || !isdigit(buf[1]) ||
	    !isdigit(buf[2]) || !isdigit(buf[3]) ||
	    sscanf(buf, "%d", &num) != 1) {
		return (false);
	}
	hrs = num / 100;
	mins = num % 100;
	if (hrs_p != NULL)
		*hrs_p = hrs;
	if (mins_p != NULL)
		*mins_p = mins;
	return (hrs >= 0 && hrs <= 23 && mins >= 0 && mins <= 59);
}

const char *
fmsbox_parse_alt(const char *str, unsigned field_nr, void *data)
{
	cpdlc_arg_t *arg;

	ASSERT(str != NULL);
	UNUSED(field_nr);
	ASSERT(data != NULL);
	arg = data;

	if (strlen(str) < 3)
		goto errout;
	if (str[0] == 'F' && str[1] == 'L') {
		if (sscanf(&str[2], "%d", &arg->alt.alt) != 1)
			goto errout;
		arg->alt.fl = true;
		arg->alt.alt *= 100;
	} else if (str[0] == 'F') {
		if (sscanf(&str[1], "%d", &arg->alt.alt) != 1)
			goto errout;
		arg->alt.fl = true;
		arg->alt.alt *= 100;
	} else {
		if (sscanf(str, "%d", &arg->alt.alt) != 1)
			goto errout;
		/* Round to nearest 100 ft */
		arg->alt.alt = round(arg->alt.alt / 100.0) * 100;
	}
	if (arg->alt.alt <= 0 || arg->alt.alt > 60000)
		goto errout;

	return (NULL);
errout:
	return ("BAD ALTITUDE");
}

const char *
fmsbox_insert_alt_block(fmsbox_t *box, unsigned field_nr, void *data,
    void *userinfo)
{
	cpdlc_arg_t *inarg, *outarg;
	size_t offset;

	ASSERT(box != NULL);
	ASSERT(data != NULL);
	inarg = data;
	offset = (uintptr_t)userinfo;
	ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;

	if (field_nr >= 2)
		goto errout;
	if (field_nr == 0) {
		memcpy(outarg, inarg, sizeof (*inarg));
		memset(&outarg[1], 0, sizeof (*inarg));
	} else {
		if (/* lower alt must actually be set */
		    outarg[0].alt.alt == 0 ||
		    /* lower alt must really be lower */
		    outarg[0].alt.alt >= inarg->alt.alt ||
		    /* if lower alt is FL, higher must as well */
		    (outarg[0].alt.fl && !inarg->alt.fl)) {
			goto errout;
		}
		memcpy(&outarg[1], inarg, sizeof (*inarg));
	}

	return (NULL);
errout:
	return ("BAD INSERT");
}

const char *
fmsbox_delete_alt_block(fmsbox_t *box, void *userinfo)
{
	cpdlc_arg_t *outarg;
	size_t offset;

	ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;
	memset(outarg, 0, 2 * sizeof (cpdlc_arg_t));

	return (NULL);
}

void
fmsbox_read_alt_block(fmsbox_t *box, void *userinfo, char str[READ_FUNC_BUF_SZ])
{
	cpdlc_arg_t *outarg;
	size_t offset;

	ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;

	if (outarg->alt.alt != 0) {
		int l = fmsbox_print_alt(outarg, str, READ_FUNC_BUF_SZ);
		if (outarg[1].alt.alt != 0) {
			strncat(str, "/", READ_FUNC_BUF_SZ - l - 1);
			l++;
			fmsbox_print_alt(&outarg[1], &str[l],
			    READ_FUNC_BUF_SZ - l);
		}
	}
}

int
fmsbox_print_alt(const cpdlc_arg_t *arg, char *str, size_t cap)
{
	ASSERT(arg != NULL);
	if (arg->alt.fl)
		return (snprintf(str, cap, "FL%d", arg->alt.alt / 100));
	return (snprintf(str, cap, "%d", arg->alt.alt));
}
