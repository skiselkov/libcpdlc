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

#ifndef	_LIBCPDLC_MSG_IMPL_H_
#define	_LIBCPDLC_MSG_IMPL_H_

#ifdef	__cplusplus
extern "C" {
#endif

#define	APPEND_SNPRINTF(__total_bytes, __bufptr, __bufcap, ...) \
	do { \
		int __needed = snprintf((__bufptr), (__bufcap), __VA_ARGS__); \
		int __consumed = MIN(__needed, (int)__bufcap); \
		(__bufptr) += __consumed; \
		(__bufcap) -= __consumed; \
		(__total_bytes) += __needed; \
	} while (0)

/* IMPORTANT: Bounds check must come first! */
#define	SKIP_SPACE(__start, __end) \
	do { \
		while ((__start) < (__end) && isspace((__start)[0])) \
			(__start)++; \
	} while (0)

/* IMPORTANT: Bounds check must come first! */
#define	SKIP_NONSPACE(__start, __end) \
	do { \
		while ((__start) < (__end) && !isspace((__start)[0])) \
			(__start)++; \
	} while (0)

#define	MALFORMED_MSG(...) \
	do { \
		set_error(reason, reason_cap, "Malformed message: " \
		    __VA_ARGS__); \
	} while (0)

static inline bool
is_valid_crs(unsigned deg)
{
	return (deg >= 1 && deg <= 360);
}

static inline bool
is_valid_lat_lon(cpdlc_lat_lon_t ll)
{
	return (ll.lat >= -90 && ll.lat <= 90 &&
	    ll.lon >= -180 && ll.lon <= 180);
}

static void
set_error(char *reason, unsigned cap, const char *fmt, ...)
{
	va_list ap;

	CPDLC_ASSERT(fmt != NULL);

	if (cap == 0)
		return;

	va_start(ap, fmt);
	vsnprintf(reason, cap, fmt, ap);
	va_end(ap);
}

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_MSG_IMPL_H_ */
