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

#ifndef	_LIBCPDLC_FANS_PARSING_H_
#define	_LIBCPDLC_FANS_PARSING_H_

#include "../src/cpdlc_msg.h"
#include "fans.h"
#include "fans_impl.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
	POS_PRINT_NORM,
	POS_PRINT_PRETTY,
	POS_PRINT_COMPACT
} pos_print_style_t;

#define	READ_FUNC_BUF_SZ	(FMS_COLS + 1)
typedef const char *(*fans_parse_func_t)(const char *str, unsigned field_nr,
    void *data);
typedef const char *(*fans_insert_func_t)(fans_t *box, unsigned field_nr,
    void *data, void *userinfo);
typedef const char *(*fans_delete_func_t)(fans_t *box, void *userinfo);
typedef void (*fans_read_func_t)(fans_t *box, void *userinfo,
    char str[READ_FUNC_BUF_SZ]);

bool fans_parse_time(const char *buf, int *hrs_p, int *mins_p);

const char *fans_parse_alt(const char *str, unsigned field_nr, void *data);
const char *fans_insert_alt_block(fans_t *box, unsigned field_nr,
    void *data, void *userinfo);
void fans_read_alt_block(fans_t *box, void *userinfo,
    char str[READ_FUNC_BUF_SZ]);

const char *fans_parse_spd(const char *str, unsigned field_nr, void *data);
const char *fans_insert_spd_block(fans_t *box, unsigned field_nr,
    void *data, void *userinfo);
void fans_read_spd_block(fans_t *box, void *userinfo,
    char str[READ_FUNC_BUF_SZ]);

const char *fans_parse_wind(const char *str, unsigned field_nr, void *data);
const char *fans_insert_wind_block(fans_t *box, unsigned field_nr,
    void *data, void *userinfo);
void fans_read_wind_block(fans_t *box, void *userinfo,
    char str[READ_FUNC_BUF_SZ]);
const char *fans_delete_wind(fans_t *box, void *userinfo);

const char *fans_delete_cpdlc_arg_block(fans_t *box, void *userinfo);

int fans_print_alt(const cpdlc_arg_t *arg, char *str, size_t cap);
int fans_print_spd(const cpdlc_arg_t *arg, char *str, size_t cap);
int fans_print_off(const fms_off_t *off, char *buf, size_t cap);

void fans_print_pos(const fms_pos_t *pos, char *buf, size_t cap,
    pos_print_style_t style);
const char *fans_parse_pos(const char *buf, fms_pos_t *pos);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FANS_PARSING_H_ */
