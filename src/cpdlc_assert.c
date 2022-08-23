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

#include <stdarg.h>
#include <stdio.h>

#include "cpdlc_assert.h"

static void assfail_default(const char *filename, int line,
    const char *buf, void *userinfo);

static cpdlc_assfail_t assfail = assfail_default;
static void *assfail_userinfo = NULL;

/* To avoid having to use the allocator or the stack, which might be b0rked. */
static char buf[1024];

static void
assfail_default(const char *filename, int line, const char *buf, void *userinfo)
{
	CPDLC_UNUSED(userinfo);
	fprintf(stderr, "%s:%d: %s\n", filename, line, buf);
}

void
cpdlc_assfail_impl(const char *filename, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof (buf), fmt, ap);
	va_end(ap);

	assfail(filename, line, buf, assfail_userinfo);
}

void
cpdlc_assfail_set(cpdlc_assfail_t cb, void *userinfo)
{
	if (cb != NULL) {
		assfail = cb;
		assfail_userinfo = userinfo;
	} else {
		assfail = assfail_default;
		assfail_userinfo = NULL;
	}
}
