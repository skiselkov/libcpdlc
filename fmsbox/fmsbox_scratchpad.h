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

#ifndef	_LIBCPDLC_FMSBOX_SCRATCHPAD_H_
#define	_LIBCPDLC_FMSBOX_SCRATCHPAD_H_

#include "fmsbox_parsing.h"

#ifdef	__cplusplus
extern "C" {
#endif

void fmsbox_update_scratchpad(fmsbox_t *box);

bool fmsbox_scratchpad_is_delete(fmsbox_t *box);
void fmsbox_scratchpad_clear(fmsbox_t *box);
void fmsbox_scratchpad_pm(fmsbox_t *box);
void fmsbox_scratchpad_xfer(fmsbox_t *box, char *dest, size_t cap,
    bool allow_mod);
bool fmsbox_scratchpad_xfer_multi(fmsbox_t *box, void *userinfo, size_t buf_sz,
    fmsbox_parse_func_t parse_func, fmsbox_insert_func_t insert_func,
    fmsbox_delete_func_t delete_func, fmsbox_read_func_t read_func);
void fmsbox_scratchpad_xfer_hdg(fmsbox_t *box, fms_hdg_t *hdg);
void fmsbox_scratchpad_xfer_alt(fmsbox_t *box, cpdlc_arg_t *alt);
void fmsbox_scratchpad_xfer_pos(fmsbox_t *box, fms_pos_t *pos,
    unsigned fms_page, pos_pick_done_cb_t done_cb);
void fmsbox_scratchpad_xfer_pos_impl(fmsbox_t *box, fms_pos_t *pos);
void fmsbox_scratchpad_xfer_uint(fmsbox_t *box, unsigned *value, bool *set,
    unsigned minval, unsigned maxval);
void fmsbox_scratchpad_xfer_time(fmsbox_t *box, fms_time_t *t);
void fmsbox_scratchpad_xfer_offset(fmsbox_t *box, fms_off_t *off);
void fmsbox_scratchpad_xfer_spd(fmsbox_t *box, cpdlc_arg_t *spd);
void fmsbox_scratchpad_xfer_temp(fmsbox_t *box, fms_temp_t *temp);
void fmsbox_scratchpad_xfer_wind(fmsbox_t *box, fms_wind_t *wind);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FMSBOX_SCRATCHPAD_H_ */
