/*
 * Copyright 2023 Saso Kiselkov
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

#ifndef	_LIBCPDLC_CPDLC_ASSERT_H_
#define	_LIBCPDLC_CPDLC_ASSERT_H_

#include <stdlib.h>

#include "cpdlc_core.h"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * CPDLC_ASSERT() and CPDLC_VERIFY() are assertion test macros. If the
 * condition expression provided as the argument to the macro evaluates
 * as non-true, the program prints a debug message specifying exactly
 * where and what condition was violated, a stack backtrace and a dumps
 * core by calling abort().
 *
 * The difference between CPDLC_ASSERT and CPDLC_VERIFY is that CPDLC_ASSERT
 * compiles to a no-op unless -DDEBUG is provided to the compiler.
 * CPDLC_VERIFY always checks its condition and dumps if it is non-true.
 */

#define	CPDLC_VERIFY_MSG(x, fmt, ...) \
	do { \
		if (!(x)) { \
			cpdlc_assfail_impl(cpdlc_ass_basename(__FILE__), \
			    __LINE__, "assertion \"%s\" failed: " fmt, #x, \
			    __VA_ARGS__); \
			abort(); \
		} \
	} while (0)
#define	CPDLC_VERIFY(x)	CPDLC_VERIFY_MSG(x, "%s", "")
#define	CPDLC_VERIFY_FAIL()	\
	do { \
		cpdlc_assfail_impl(cpdlc_ass_basename(__FILE__), \
		    __LINE__, "internal error"); \
		abort(); \
	} while (0)

#define	CPDLC_VERIFY3_impl(x, op, y, type, fmt) \
	do { \
		type tmp_x = (type)(x); \
		type tmp_y = (type)(y); \
		if (!(tmp_x op tmp_y)) { \
			cpdlc_assfail_impl(cpdlc_ass_basename(__FILE__), \
			    __LINE__, "assertion %s %s %s failed (" fmt " %s \
			    " fmt ")", #x, #op, #y, tmp_x, #op, tmp_y); \
			abort(); \
		} \
	} while (0)
#define	CPDLC_VERIFY3S(x, op, y) CPDLC_VERIFY3_impl(x, op, y, long, "%lu")
#define	CPDLC_VERIFY3U(x, op, y) \
	CPDLC_VERIFY3_impl(x, op, y, unsigned long, "0x%lx")
#define	CPDLC_VERIFY3F(x, op, y) CPDLC_VERIFY3_impl(x, op, y, double, "%f")
#define	CPDLC_VERIFY3P(x, op, y) CPDLC_VERIFY3_impl(x, op, y, void *, "%p")
#define	CPDLC_VERIFY0(x)	CPDLC_VERIFY3S((x), ==, 0)

#ifdef	DEBUG
#define	CPDLC_ASSERT(x)			CPDLC_VERIFY(x)
#define	CPDLC_ASSERT3S(x, op, y)	CPDLC_VERIFY3S(x, op, y)
#define	CPDLC_ASSERT3U(x, op, y)	CPDLC_VERIFY3U(x, op, y)
#define	CPDLC_ASSERT3F(x, op, y)	CPDLC_VERIFY3F(x, op, y)
#define	CPDLC_ASSERT3P(x, op, y)	CPDLC_VERIFY3P(x, op, y)
#define	CPDLC_ASSERT0(x)		CPDLC_VERIFY0(x)
#define	CPDLC_ASSERT_MSG(x, fmt, ...)	CPDLC_VERIFY_MSG(x, fmt, __VA_ARGS__)
#else	/* !DEBUG */
#define	CPDLC_ASSERT(x)			CPDLC_UNUSED(x)
#define	CPDLC_ASSERT3S(x, op, y)	CPDLC_UNUSED((x) op (y))
#define	CPDLC_ASSERT3U(x, op, y)	CPDLC_UNUSED((x) op (y))
#define	CPDLC_ASSERT3F(x, op, y)	CPDLC_UNUSED((x) op (y))
#define	CPDLC_ASSERT3P(x, op, y)	CPDLC_UNUSED((x) op (y))
#define	CPDLC_ASSERT0(x)		CPDLC_UNUSED(x)
#define	CPDLC_ASSERT_MSG(x, fmt, ...)	CPDLC_UNUSED(x)
#endif	/* !DEBUG */

/*
 * Compile-time assertion. The condition 'x' must be constant.
 */
#if     __STDC_VERSION__ >= 201112L
#define CPDLC_CTASSERT(x)		_Static_assert((x), #x)
#elif	defined(__GNUC__) || defined(__clang__)
#define	CPDLC_CTASSERT(x)		_CPDLC_CTASSERT(x, __LINE__)
#define	_CPDLC_CTASSERT(x, y)		__CPDLC_CTASSERT(x, y)
#define	__CPDLC_CTASSERT(x, y)	\
	typedef char __compile_time_assertion__ ## y [(x) ? 1 : -1] \
	    __attribute__((unused))
#else	/* !defined(__GNUC__) && !defined(__clang__) */
#define	CPDLC_CTASSERT(x)
#endif	/* !defined(__GNUC__) && !defined(__clang__) */

#if	defined(__GNUC__) || defined(__clang__)
#define	CPDLC_BUILTIN_STRRCHR __builtin_strrchr
#else	/* !defined(__GNUC__) && !defined(__clang__) */
#define	CPDLC_BUILTIN_STRRCHR strrchr
#endif	/* !defined(__GNUC__) && !defined(__clang__) */

/*
 * This lets us chop out the basename (last path component) from __FILE__
 * at compile time. This works on GCC and Clang. The fallback mechanism
 * below just chops it out at compile time.
 */
#define	cpdlc_ass_basename(f) \
	(CPDLC_BUILTIN_STRRCHR(f, '/') ? CPDLC_BUILTIN_STRRCHR(f, '/') + 1 : \
	    (CPDLC_BUILTIN_STRRCHR(f, '\\') ? \
	    CPDLC_BUILTIN_STRRCHR(f, '\\') + 1 : (f)))

typedef void (*cpdlc_assfail_t)(const char *filename, int line,
    const char *msg, void *userinfo);

void cpdlc_assfail_set(cpdlc_assfail_t cb, void *userinfo);
void cpdlc_assfail_impl(const char *filename, int line,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(3);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_CPDLC_ASSERT_H_ */
