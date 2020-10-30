/*
 * Copyright 2020 Saso Kiselkov
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

#ifndef	_XPINTF_H_
#define	_XPINTF_H_

#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif

bool xpintf_init(const char *host, int port);
void xpintf_fini(void);
void xpintf_update(void);

bool xpintf_get_time(unsigned *hours, unsigned *mins);
bool xpintf_get_sat(int *temp_C);
bool xpintf_get_cur_spd(bool *mach, unsigned *spd);
bool xpintf_get_cur_alt(int *alt_ft);
bool xpintf_get_sel_alt(int *alt_ft);
bool xpintf_get_wind(unsigned *deg_true, unsigned *knots);

#ifdef	__cplusplus
}
#endif

#endif	/* _XPINTF_H_ */
