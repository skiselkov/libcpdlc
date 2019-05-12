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

#ifndef	_LIBCPDLC_CPDLC_ALLOC_H_
#define	_LIBCPDLC_CPDLC_ALLOC_H_

#include <stdlib.h>
#include <string.h>

#ifdef	CPDLC_HAVE_GCRYPT
#include <gcrypt.h>
#endif

#include "cpdlc_assert.h"

#ifdef	__cplusplus
extern "C" {
#endif

static inline void *
safe_malloc(size_t size)
{
	void *p = malloc(size);
	if (size > 0) {
		VERIFY_MSG(p != NULL, "Cannot allocate %lu bytes: "
		    "out of memory", (long unsigned)size);
	}
	return (p);
}

static inline void *
safe_calloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);
	if (nmemb > 0 && size > 0) {
		VERIFY_MSG(p != NULL, "Cannot allocate %lu bytes: "
		    "out of memory", (long unsigned)(nmemb * size));
	}
	return (p);
}

static inline void *
safe_realloc(void *oldptr, size_t size)
{
	void *p = realloc(oldptr, size);
	if (size > 0) {
		VERIFY_MSG(p != NULL, "Cannot allocate %lu bytes: "
		    "out of memory", (long unsigned)size);
	}
	return (p);
}

#ifdef	CPDLC_HAVE_GCRYPT

#define	secure_malloc	gcry_xmalloc_secure
#define	secure_calloc	gcry_xcalloc_secure
#define	secure_realloc	gcry_xrealloc_secure
#define	secure_free	gcry_free

#else	/* !CPDLC_HAVE_GCRYPT */

#define	secure_malloc	safe_malloc
#define	secure_calloc	safe_calloc
#define	secure_realloc	safe_realloc
#define	secure_free	free

#endif	/* !CPDLC_HAVE_GCRYPT */

static inline char *
secure_strdup(const char *s)
{
	int l = strlen(s);
	char *ns = secure_malloc(l + 1);
	strcpy(ns, s);
	return (ns);
}

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_CPDLC_ALLOC_H_ */
