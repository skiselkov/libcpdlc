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

#ifndef	_LIBCPDLC_FMSBOX_PARSING_H_
#define	_LIBCPDLC_FMSBOX_PARSING_H_

#include "../src/cpdlc_msg.h"
#include "fmsbox.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define	READ_FUNC_BUF_SZ	(FMSBOX_COLS + 1)
typedef const char *(*fmsbox_parse_func_t)(const char *str, unsigned field_nr,
    void *data);
typedef const char *(*fmsbox_insert_func_t)(fmsbox_t *box, unsigned field_nr,
    void *data, void *userinfo);
typedef const char *(*fmsbox_delete_func_t)(fmsbox_t *box, void *userinfo);
typedef void (*fmsbox_read_func_t)(fmsbox_t *box, void *userinfo,
    char str[READ_FUNC_BUF_SZ]);

bool fmsbox_parse_time(const char *buf, int *hrs_p, int *mins_p);

const char *fmsbox_parse_alt(const char *str, unsigned field_nr, void *data);
const char *fmsbox_insert_alt_block(fmsbox_t *box, unsigned field_nr,
    void *data, void *userinfo);
void fmsbox_read_alt_block(fmsbox_t *box, void *userinfo,
    char str[READ_FUNC_BUF_SZ]);

const char *fmsbox_parse_spd(const char *str, unsigned field_nr, void *data);
const char *fmsbox_insert_spd_block(fmsbox_t *box, unsigned field_nr,
    void *data, void *userinfo);
void fmsbox_read_spd_block(fmsbox_t *box, void *userinfo,
    char str[READ_FUNC_BUF_SZ]);

const char *fmsbox_delete_cpdlc_arg_block(fmsbox_t *box, void *userinfo);

int fmsbox_print_alt(const cpdlc_arg_t *arg, char *str, size_t cap);
int fmsbox_print_spd(const cpdlc_arg_t *arg, char *str, size_t cap);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FMSBOX_PARSING_H_ */
