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

#ifndef	_LIBCPDLC_CPDLC_STRING_H_
#define	_LIBCPDLC_CPDLC_STRING_H_

#include <string.h>

#include "cpdlc_core.h"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * strlcpy is a BSD function not available on Windows, so we roll a simple
 * version of it ourselves. Also, on BSD it's SLOOOOW. Inline for max perf.
 */
static inline void
cpdlc_strlcpy(char *restrict dest, const char *restrict src, size_t cap)
{
	size_t l;
	CPDLC_ASSERT(cap != 0);
	/*
	 * We MUSTN'T use strlen here, because src may be SIGNIFICANTLY
	 * larger than dest and we don't want to measure the ENTIRE body
	 * of src. We only care for length UP TO the destination capacity.
	 */
	for (l = 0; l + 1 < cap && src[l] != '\0'; l++)
		;
	/*
	 * Due to a bug in GCC, we can't use strncpy, as it sometimes throws
	 * "call to __builtin___strncpy_chk will always overflow destination
	 * buffer", even when it's absolutely NOT the case.
	 */
	memcpy(dest, src, MIN(cap - 1, l + 1));
	/* Insure the string is ALWAYS terminated */
	dest[cap - 1] = '\0';
}

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_CPDLC_STRING_H_ */
