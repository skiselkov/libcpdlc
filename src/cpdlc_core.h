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

#ifndef	_LIBCPDLC_CPDLC_CORE_H_
#define	_LIBCPDLC_CPDLC_CORE_H_

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_MSC_VER)
#define	CPDLC_API	__declspec(dllexport)
#define	UNUSED_ATTR	__attribute__((unused))
#else	/* !defined(_MSC_VER) */
#define	CPDLC_API
#define	UNUSED_ATTR	__attribute__((unused))
#endif	/* !defined(_MSC_VER) */

#define	UNUSED(x)	(void)(x)

#if	__STDC_VERSION__ < 199901L
# if	defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
#  define	restrict        __restrict
# else
#  define	restrict
# endif
# if		defined(_MSC_VER)
#  define	inline  __inline
# else
#  define	inline
# endif
#endif	/* __STDC_VERSION__ < 199901L */

#ifndef	MIN
#define	MIN(x, y)	((x) < (y) ? (x) : (y))
#endif
#ifndef	MAX
#define	MAX(x, y)	((x) > (y) ? (x) : (y))
#endif

#if	defined(__GNUC__) || defined(__clang__)
#define	PRINTF_ATTR(x)		__attribute__((format(printf, x, x + 1)))
#define	PRINTF_ATTR2(x,y)	__attribute__((format(printf, x, y)))
#define	PRINTF_FORMAT(f)	f
#ifndef	BSWAP32
#define	BSWAP16(x)	__builtin_bswap16((x))
#define	BSWAP32(x)	__builtin_bswap32((x))
#define	BSWAP64(x)	__builtin_bswap64((x))
#endif	/* BSWAP32 */

#define	COND_LIKELY(x)		__builtin_expect(x, 1)
#define	COND_UNLIKELY(x)	__builtin_expect(x, 0)

#else	/* !__GNUC__ && !__clang__ */

#define	PRINTF_ATTR(x)
#define	PRINTF_ATTR2(x,y)

#define	COND_LIKELY(x)		x
#define	COND_UNLIKELY(x)	x

#if	_MSC_VER >= 1400
# include <sal.h>
# if	_MSC_VER > 1400
#  define	PRINTF_FORMAT(f)	_Printf_format_string_ f
# else	/* _MSC_VER == 1400 */
#  define	PRINTF_FORMAT(f)	__format_string f
# endif /* FORMAT_STRING */
#else	/* _MSC_VER < 1400 */
# define	PRINTF_FORMAT(f)	f
#endif	/* _MSC_VER */

#ifndef	BSWAP32
#define	BSWAP16(x)	\
	((((x) & 0xff00u) >> 8) | \
	(((x) & 0x00ffu) << 8))
#define	BSWAP32(x)	\
	((((x) & 0xff000000u) >> 24) | \
	(((x) & 0x00ff0000u) >> 8) | \
	(((x) & 0x0000ff00u) << 8) | \
	(((x) & 0x000000ffu) << 24))
#define	BSWAP64(x)	\
	((((x) & 0x00000000000000ffllu) >> 56) | \
	(((x) & 0x000000000000ff00llu) << 40) | \
	(((x) & 0x0000000000ff0000llu) << 24) | \
	(((x) & 0x00000000ff000000llu) << 8) | \
	(((x) & 0x000000ff00000000llu) >> 8) | \
	(((x) & 0x0000ff0000000000llu) >> 24) | \
	(((x) & 0x00ff000000000000llu) >> 40) | \
	(((x) & 0xff00000000000000llu) << 56))
#endif	/* BSWAP32 */
#endif	/* !__GNUC__ && !__clang__ */

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_CPDLC_CORE_H_ */
