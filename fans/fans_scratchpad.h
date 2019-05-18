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

#ifndef	_LIBCPDLC_FANS_SCRATCHPAD_H_
#define	_LIBCPDLC_FANS_SCRATCHPAD_H_

#include "fans_parsing.h"

#ifdef	__cplusplus
extern "C" {
#endif

void fans_update_scratchpad(fans_t *box);

bool fans_scratchpad_is_delete(fans_t *box);
void fans_scratchpad_clear(fans_t *box);
void fans_scratchpad_pm(fans_t *box);
void fans_scratchpad_xfer(fans_t *box, char *dest, size_t cap, bool allow_mod);
void fans_scratchpad_xfer_auto(fans_t *box, char *dest, const char *autobuf,
    size_t cap, bool allow_mod);
bool fans_scratchpad_xfer_multi(fans_t *box, void *userinfo, size_t buf_sz,
    fans_parse_func_t parse_func, fans_insert_func_t insert_func,
    fans_delete_func_t delete_func, fans_read_func_t read_func);
void fans_scratchpad_xfer_hdg(fans_t *box, fms_hdg_t *hdg);
void fans_scratchpad_xfer_pos(fans_t *box, fms_pos_t *pos,
    unsigned fms_page, pos_pick_done_cb_t done_cb);
void fans_scratchpad_xfer_pos_impl(fans_t *box, fms_pos_t *pos);
void fans_scratchpad_xfer_uint(fans_t *box, unsigned *value, bool *set,
    unsigned minval, unsigned maxval);
void fans_scratchpad_xfer_time(fans_t *box, fms_time_t *usertime,
    const fms_time_t *autotime);
void fans_scratchpad_xfer_offset(fans_t *box, fms_off_t *useroff,
    const fms_off_t *autooff);
void fans_scratchpad_xfer_alt(fans_t *box, cpdlc_arg_t *useralt,
    const cpdlc_arg_t *autoalt);
void fans_scratchpad_xfer_spd(fans_t *box, cpdlc_arg_t *userspd,
    const cpdlc_arg_t *autospd);
void fans_scratchpad_xfer_temp(fans_t *box, fms_temp_t *usertemp,
    const fms_temp_t *autotemp);
void fans_scratchpad_xfer_wind(fans_t *box, fms_wind_t *wind);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_FANS_SCRATCHPAD_H_ */
