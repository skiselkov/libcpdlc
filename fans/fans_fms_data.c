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

#include <math.h>

#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_string.h"

#include "fans_impl.h"

static void
fms_data_draw_page1(fans_t *box)
{
	cpdlc_arg_t spd;
	float value_f32;
	fms_wpt_info_t wpt;
	fms_temp_t temp;

	CPDLC_ASSERT(box != NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "CUR SPD");
	if (fans_get_cur_spd(box, &spd)) {
		if (spd.spd.mach) {
			fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
			    FMS_FONT_SMALL, "M.%02d", spd.spd.spd);
		} else {
			fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
			    FMS_FONT_SMALL, "%3d KT", spd.spd.spd);
		}
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "CUR ALT");
	value_f32 = fans_get_cur_alt(box);
	if (!isnan(value_f32)) {
		fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%5.0f FT", value_f32);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "CUR VVI");
	value_f32 = fans_get_cur_vvi(box);
	if (!isnan(value_f32)) {
		fans_put_str(box, LSK3_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%5.0f FPM", value_f32);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_L4, "SEL ALT");
	value_f32 = fans_get_sel_alt(box);
	if (!isnan(value_f32)) {
		fans_put_str(box, LSK4_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%5.0f FT", value_f32);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_L5, "PREV WPT");
	if (fans_get_prev_wpt(box, &wpt)) {
		fans_put_str(box, LSK5_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", wpt.wpt_name);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R1, "NEXT WPT");
	if (fans_get_next_wpt(box, &wpt)) {
		fans_put_str(box, LSK1_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", wpt.wpt_name);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R2, "NEXTNEXT WPT");
	if (fans_get_next_next_wpt(box, &wpt)) {
		fans_put_str(box, LSK2_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", wpt.wpt_name);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R3, "DEST WPT");
	if (fans_get_dest_wpt(box, &wpt)) {
		fans_put_str(box, LSK3_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%s", wpt.wpt_name);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R4, "OFFSET");
	value_f32 = fans_get_offset(box);
	if (!isnan(value_f32)) {
		fans_put_str(box, LSK4_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%.1f NM", value_f32);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R5, "SAT");
	fans_get_sat(box, &temp);
	if (temp.set) {
		fans_put_str(box, LSK5_ROW, 0, true, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%d`C", temp.temp);
	}
}

static void
fms_data_draw_page2(fans_t *box)
{
	unsigned value_u32;
	fms_wind_t wind;
	unsigned hrs, mins;

	CPDLC_ASSERT(box != NULL);

	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "WIND");
	fans_get_wind(box, &wind);
	if (wind.set) {
		fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%03dT/%3dKT", wind.deg, wind.spd);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_L2, "FUEL");
	if (fans_get_fuel(box, &hrs, &mins)) {
		fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%02d:%02d", hrs, mins);
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_L3, "SOULS");
	if (fans_get_souls(box, &value_u32)) {
		fans_put_str(box, LSK3_ROW, 0, false, FMS_COLOR_GREEN,
		    FMS_FONT_SMALL, "%d", value_u32);
	}
}

void
fans_fms_data_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_put_page_title(box, "FANS  FMS DATA");
	fans_set_num_subpages(box, 2);
	fans_put_page_ind(box, FMS_COLOR_WHITE);

	if (fans_get_subpage(box) == 0)
		fms_data_draw_page1(box);
	else
		fms_data_draw_page2(box);
}

bool
fans_fms_data_key_cb(fans_t *box, fms_key_t key)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_UNUSED(key);
	return (false);
}
