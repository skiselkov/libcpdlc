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

#ifndef	_LIBCPDLC_CPDLC_ASSERT_H_
#define	_LIBCPDLC_CPDLC_ASSERT_H_

#include <stdlib.h>

#include "cpdlc_core.h"

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	VERIFY
/*
 * ASSERT() and VERIFY() are assertion test macros. If the condition
 * expression provided as the argument to the macro evaluates as non-true,
 * the program prints a debug message specifying exactly where and what
 * condition was violated, a stack backtrace and a dumps core by
 * calling abort().
 *
 * The difference between ASSERT and VERIFY is that ASSERT compiles to
 * a no-op unless -DDEBUG is provided to the compiler. VERIFY always
 * checks its condition and dumps if it is non-true.
 */

#define	VERIFY_MSG(x, fmt, ...) \
	do { \
		if (!(x)) { \
			cpdlc_assfail_impl(cpdlc_ass_basename(__FILE__), \
			    __LINE__, "assertion \"%s\" failed: " fmt, #x, \
			    __VA_ARGS__); \
			abort(); \
		} \
	} while (0)
#define	VERIFY(x)	VERIFY_MSG(x, "%s", "")

#define	VERIFY3_impl(x, op, y, type, fmt) \
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
#define	VERIFY3S(x, op, y)	VERIFY3_impl(x, op, y, long, "%lu")
#define	VERIFY3U(x, op, y)	VERIFY3_impl(x, op, y, unsigned long, "0x%lx")
#define	VERIFY3F(x, op, y)	VERIFY3_impl(x, op, y, double, "%f")
#define	VERIFY3P(x, op, y)	VERIFY3_impl(x, op, y, void *, "%p")
#define	VERIFY0(x)		VERIFY3S((x), ==, 0)
#endif	/* !defined(VERIFY) */

#ifndef	ASSERT
#ifdef	DEBUG
#define	ASSERT(x)		VERIFY(x)
#define	ASSERT3S(x, op, y)	VERIFY3S(x, op, y)
#define	ASSERT3U(x, op, y)	VERIFY3U(x, op, y)
#define	ASSERT3F(x, op, y)	VERIFY3F(x, op, y)
#define	ASSERT3P(x, op, y)	VERIFY3P(x, op, y)
#define	ASSERT0(x)		VERIFY0(x)
#define	ASSERT_MSG(x, fmt, ...)	VERIFY_MSG(x, fmt, __VA_ARGS__)
#else	/* !DEBUG */
#define	ASSERT(x)		UNUSED(x)
#define	ASSERT3S(x, op, y)	UNUSED((x) op (y))
#define	ASSERT3U(x, op, y)	UNUSED((x) op (y))
#define	ASSERT3F(x, op, y)	UNUSED((x) op (y))
#define	ASSERT3P(x, op, y)	UNUSED((x) op (y))
#define	ASSERT0(x)		UNUSED(x)
#define	ASSERT_MSG(x, fmt, ...)	UNUSED(x)
#endif	/* !DEBUG */
#endif	/* !defined(ASSERT) */

/*
 * Compile-time assertion. The condition 'x' must be constant.
 */
#ifndef	CTASSERT
#if	defined(__GNUC__) || defined(__clang__)
#define	CTASSERT(x)		_CTASSERT(x, __LINE__)
#define	_CTASSERT(x, y)		__CTASSERT(x, y)
#define	__CTASSERT(x, y)	\
	typedef char __compile_time_assertion__ ## y [(x) ? 1 : -1] \
	    __attribute__((unused))
#else	/* !defined(__GNUC__) && !defined(__clang__) */
#define	CTASSERT(x)
#endif	/* !defined(__GNUC__) && !defined(__clang__) */
#endif	/* !defined(CTASSERT) */

#if	defined(__GNUC__) || defined(__clang__)
#define	BUILTIN_STRRCHR __builtin_strrchr
#else	/* !defined(__GNUC__) && !defined(__clang__) */
#define	BUILTIN_STRRCHR strrchr
#endif	/* !defined(__GNUC__) && !defined(__clang__) */

/*
 * This lets us chop out the basename (last path component) from __FILE__
 * at compile time. This works on GCC and Clang. The fallback mechanism
 * below just chops it out at compile time.
 */
#define	cpdlc_ass_basename(f) \
	(BUILTIN_STRRCHR(f, '/') ? BUILTIN_STRRCHR(f, '/') + 1 : \
	    (BUILTIN_STRRCHR(f, '\\') ? BUILTIN_STRRCHR(f, '\\') + 1 : (f)))

typedef void (*cpdlc_assfail_t)(const char *filename, int line,
    const char *msg, void *userinfo);

extern cpdlc_assfail_t cpdlc_assfail;
extern void *cpdlc_assfail_userinfo;

void cpdlc_assfail_impl(const char *filename, int line,
    PRINTF_FORMAT(const char *fmt), ...) PRINTF_ATTR(3);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_CPDLC_ASSERT_H_ */
