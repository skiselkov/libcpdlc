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

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../src/cpdlc_alloc.h"
#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_string.h"
#include "fans.h"
#include "fans_impl.h"
#include "fans_parsing.h"

bool
fans_parse_time(const char *buf, int *hrs_p, int *mins_p)
{
	int num, hrs, mins, len;

	CPDLC_ASSERT(buf != NULL);

	len = strlen(buf);
	for (int i = 0; i < MIN(len, 4); i++) {
		if (!isdigit(buf[i]))
			return (false);
	}
	if (!isdigit(buf[len - 1]) && buf[len - 1] != 'Z')
		return false;
	if (sscanf(buf, "%d", &num) != 1)
		return (false);
	switch (len) {
	case 1:
	case 2:
		hrs = num;
		mins = 0;
		break;
	case 3:
		hrs = num / 10;
		mins = (num % 10) * 10;
		break;
	case 4:
	case 5:
		hrs = num / 100;
		mins = num % 100;
		break;
	default:
		return (false);
	}
	if (hrs < 0 || hrs > 23 || mins < 0 || mins > 59)
		return (false);
	if (hrs_p != NULL)
		*hrs_p = hrs;
	if (mins_p != NULL)
		*mins_p = mins;
	return (true);
}

const char *
fans_parse_alt(const char *str, unsigned field_nr, void *data)
{
	cpdlc_arg_t arg;

	CPDLC_ASSERT(str != NULL);
	CPDLC_UNUSED(field_nr);
	CPDLC_ASSERT(data != NULL);

	memset(&arg, 0, sizeof (arg));

	if (strlen(str) < 2)
		goto errout;
	if (str[0] == 'F' && str[1] == 'L') {
		if (sscanf(&str[2], "%d", &arg.alt.alt) != 1)
			goto errout;
		arg.alt.fl = true;
		arg.alt.alt *= 100;
	} else if (str[0] == 'F') {
		if (sscanf(&str[1], "%d", &arg.alt.alt) != 1)
			goto errout;
		arg.alt.fl = true;
		arg.alt.alt *= 100;
	} else {
		if (sscanf(str, "%d", &arg.alt.alt) != 1)
			goto errout;
		if (arg.alt.alt < 1000) {
			arg.alt.fl = true;
			arg.alt.alt *= 100;
		} else {
			/* Round to nearest 100 ft */
			arg.alt.fl = false;
			arg.alt.alt = round(arg.alt.alt / 100.0) * 100;
		}
	}
	if (arg.alt.alt < 1000 || arg.alt.alt > 60000)
		goto errout;

	memcpy(data, &arg, sizeof (arg));

	return (NULL);
errout:
	return ("FORMAT ERROR");
}

const char *
fans_insert_alt_block(fans_t *box, unsigned field_nr, void *data,
    void *userinfo)
{
	cpdlc_arg_t *inarg, *outarg;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(data != NULL);
	inarg = data;
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;

	if (field_nr >= 2)
		goto errout;
	if (field_nr == 0) {
		memcpy(outarg, inarg, sizeof (*inarg));
		memset(&outarg[1], 0, sizeof (*inarg));
	} else {
		if (/* lower alt must actually be set */
		    outarg[0].alt.alt == 0 ||
		    /* lower alt must really be lower */
		    outarg[0].alt.alt >= inarg->alt.alt ||
		    /* if lower alt is FL, higher must as well */
		    (outarg[0].alt.fl && !inarg->alt.fl)) {
			goto errout;
		}
		memcpy(&outarg[1], inarg, sizeof (*inarg));
	}

	return (NULL);
errout:
	return ("INVALID INSERT");
}

void
fans_read_alt_block(fans_t *box, void *userinfo, char str[READ_FUNC_BUF_SZ])
{
	cpdlc_arg_t *outarg;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;

	if (outarg->alt.alt != 0) {
		int l = fans_print_alt(outarg, str, READ_FUNC_BUF_SZ, false);
		if (outarg[1].alt.alt != 0) {
			strncat(str, "/", READ_FUNC_BUF_SZ - l - 1);
			l++;
			fans_print_alt(&outarg[1], &str[l],
			    READ_FUNC_BUF_SZ - l, false);
		}
	}
}

const char *
fans_parse_spd(const char *str, unsigned field_nr, void *data)
{
	cpdlc_arg_t *arg;

	CPDLC_ASSERT(str != NULL);
	CPDLC_UNUSED(field_nr);
	CPDLC_ASSERT(data != NULL);
	arg = data;

	if (strlen(str) < 2)
		goto errout;
	if (str[0] == 'M' || str[0] == '.') {
		if (sscanf(&str[1], "%d", &arg->spd.spd) != 1 ||
		    arg->spd.spd < 10 || arg->spd.spd > 99)
			goto errout;
		arg->spd.mach = true;
		arg->spd.spd *= 10;
	} else {
		if (sscanf(str, "%d", &arg->spd.spd) != 1 ||
		    arg->spd.spd < 10 || arg->spd.spd > 999)
			goto errout;
		if (arg->spd.spd < 100) {
			arg->spd.mach = true;
			arg->spd.spd *= 10;
		}
	}

	return (NULL);
errout:
	return ("FORMAT ERROR");
}

const char *
fans_insert_spd_block(fans_t *box, unsigned field_nr, void *data,
    void *userinfo)
{
	cpdlc_arg_t *inarg, *outarg;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(data != NULL);
	inarg = data;
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;

	if (field_nr >= 2)
		goto errout;
	if (field_nr == 0) {
		memcpy(outarg, inarg, sizeof (*inarg));
		memset(&outarg[1], 0, sizeof (*inarg));
	} else {
		if (/* both speeds must be mach or airspeed */
		    outarg[0].spd.mach != inarg->spd.mach ||
		    /* lower airspeed must be set */
		    outarg[0].spd.spd == 0 ||
		    /* lower speed must really be lower */
		    (outarg[0].spd.spd >= inarg->spd.spd)) {
			goto errout;
		}
		memcpy(&outarg[1], inarg, sizeof (*inarg));
	}

	return (NULL);
errout:
	return ("INVALID INSERT");
}

void
fans_read_spd_block(fans_t *box, void *userinfo, char str[READ_FUNC_BUF_SZ])
{
	cpdlc_arg_t *outarg;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;

	if (outarg->alt.alt != 0) {
		int l = fans_print_spd(outarg, str, READ_FUNC_BUF_SZ, false);
		if (outarg[1].spd.spd != 0) {
			strncat(str, "/", READ_FUNC_BUF_SZ - l - 1);
			l++;
			fans_print_spd(&outarg[1], &str[l],
			    READ_FUNC_BUF_SZ - l, false);
		}
	}
}

const char *
fans_parse_wind(const char *str, unsigned field_nr, void *data)
{
	fms_wind_t *out_wind;

	CPDLC_ASSERT(str != NULL);
	CPDLC_ASSERT(field_nr == 0 || field_nr == 1);
	CPDLC_ASSERT(data != NULL);
	out_wind = data;

	if (field_nr == 0) {
		if (sscanf(str, "%d", &out_wind->deg) != 1 ||
		    out_wind->deg > 360) {
			return ("FORMAT ERROR");
		}
		out_wind->deg %= 360;
	} else {
		if (sscanf(str, "%d", &out_wind->spd) != 1 ||
		    out_wind->spd > MAX_WIND) {
			return ("FORMAT ERROR");
		}
	}

	return (NULL);
}

const char *
fans_insert_wind_block(fans_t *box, unsigned field_nr, void *data,
    void *userinfo)
{
	fms_wind_t *in_wind, *out_wind;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(data != NULL);
	CPDLC_ASSERT(field_nr == 0 || field_nr == 1);
	CPDLC_ASSERT(userinfo != NULL);

	in_wind = data;
	out_wind = userinfo;

	if (field_nr == 0) {
		out_wind->deg = in_wind->deg;
		if (out_wind->spd)
			out_wind->set = true;
	} else {
		out_wind->spd = in_wind->spd;
		out_wind->set = true;
	}

	return (NULL);
}

void
fans_read_wind_block(fans_t *box, void *userinfo,
    char str[READ_FUNC_BUF_SZ])
{
	fms_wind_t *wind;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(userinfo != NULL);
	wind = userinfo;
	if (wind->set) {
		snprintf(str, READ_FUNC_BUF_SZ, "%03d/%d",
		    wind->deg, wind->spd);
	}
}

const char *
fans_delete_wind(fans_t *box, void *userinfo)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(userinfo != NULL);
	memset(userinfo, 0, sizeof (fms_wind_t));
	return (NULL);
}

const char *
fans_delete_cpdlc_arg_block(fans_t *box, void *userinfo)
{
	cpdlc_arg_t *outarg;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_arg_t));
	outarg = ((void *)box) + offset;
	memset(outarg, 0, 2 * sizeof (cpdlc_arg_t));

	return (NULL);
}

int
fans_print_alt(const cpdlc_arg_t *arg, char *str, size_t cap, bool units)
{
	CPDLC_ASSERT(arg != NULL);
	if (arg->alt.fl)
		return (snprintf(str, cap, "FL%d", arg->alt.alt / 100));
	return (snprintf(str, cap, "%d%s", arg->alt.alt, units ? "FT" : ""));
}

int
fans_print_spd(const cpdlc_arg_t *arg, char *str, size_t cap, bool units)
{
	CPDLC_ASSERT(arg != NULL);
	if (arg->spd.mach) {
		if (units)
			return (snprintf(str, cap, "M%03d", arg->spd.spd / 10));
		else
			return (snprintf(str, cap, "M%02d", arg->spd.spd / 10));
	} else {
		return (snprintf(str, cap, "%03d%s", arg->spd.spd,
		    units ? "KT" : ""));
	}
}

int
fans_print_off(const fms_off_t *off, char *buf, size_t cap)
{
	CPDLC_ASSERT(off != NULL);
	if (off->nm != 0) {
		return (snprintf(buf, cap, "%c%.0f",
		    off->dir == CPDLC_DIR_LEFT ? 'L' : 'R', off->nm));
	}
	cpdlc_strlcpy(buf, "", cap);
	return (1);
}

void
fans_print_pos(const fms_pos_t *pos, char *buf, size_t bufsz,
    pos_print_style_t style)
{
	CPDLC_ASSERT(pos != NULL);
	CPDLC_ASSERT(buf != NULL);

	if (!pos->set) {
		if (style == POS_PRINT_PRETTY) {
			switch (pos->type) {
			case FMS_POS_NAVAID:
			case FMS_POS_ARPT:
				cpdlc_strlcpy(buf, "____", bufsz);
				break;
			case FMS_POS_FIX:
				cpdlc_strlcpy(buf, "_____", bufsz);
				break;
			case FMS_POS_LAT_LON:
				snprintf(buf, bufsz, "___`__.__ ____`__.__");
				break;
			case FMS_POS_PBD:
				snprintf(buf, bufsz, "___/___/___");
				break;
			}
		} else {
			buf[0] = '\0';
		}
		return;
	}

	switch (pos->type) {
	case FMS_POS_NAVAID:
	case FMS_POS_ARPT:
		cpdlc_strlcpy(buf, pos->name, bufsz);
		break;
	case FMS_POS_FIX:
		cpdlc_strlcpy(buf, pos->name, bufsz);
		break;
	case FMS_POS_LAT_LON: {
		int lat = fabs(pos->lat);
		int lon = fabs(pos->lon);
		char ns = (pos->lat >= 0 ? 'N' : 'S');
		char ew = (pos->lon >= 0 ? 'E' : 'W');
		double lat_mins = fabs(pos->lat - trunc(pos->lat)) * 60;
		double lon_mins = fabs(pos->lon - trunc(pos->lon)) * 60;

		if (style == POS_PRINT_PRETTY) {
			snprintf(buf, bufsz, "%c%02d`%05.2f %c%03d`%05.2f",
			    ns, lat, lat_mins, ew, lon, lon_mins);
		} else if (style == POS_PRINT_COMPACT) {
			snprintf(buf, bufsz, "%c%02d%c%03d", ns, lat, ew, lon);
		} else if (style == POS_PRINT_NORM_SPACE) {
			snprintf(buf, bufsz, "%c%02d%05.2f %c%03d%05.2f",
			    ns, lat, lat_mins, ew, lon, lon_mins);
		} else {
			snprintf(buf, bufsz, "%c%02d%05.2f%c%03d%05.2f",
			    ns, lat, lat_mins, ew, lon, lon_mins);
		}
		break;
	}
	case FMS_POS_PBD:
		snprintf(buf, bufsz, "%s/%03d/%d", pos->name, pos->brg,
		    pos->dist);
		break;
	}
}

static bool
parse_deg_mins(const char *buf, unsigned num_deg_digits,
    double *degs, double *mins)
{
	double deg_max;

	CPDLC_ASSERT(buf != NULL);
	CPDLC_ASSERT(num_deg_digits == 2 || num_deg_digits == 3);
	deg_max = (num_deg_digits == 2 ? 89 : 179);

	if (strlen(buf) <= num_deg_digits) {
		*mins = 0;
		return (sscanf(buf, "%lf", degs) == 1 &&
		    *degs >= 0 && *degs <= deg_max);
	} else if (strlen(buf) >= num_deg_digits + 2) {
		char deg_buf[4];
		memset(deg_buf, 0, sizeof (deg_buf));
		for (unsigned i = 0; i < num_deg_digits; i++)
			deg_buf[i] = buf[i];

		return (sscanf(deg_buf, "%lf", degs) == 1 &&
		    sscanf(&buf[num_deg_digits], "%lf", mins) == 1 &&
		    *degs >= 0 && *degs <= deg_max &&
		    *mins >= 0 && *mins < 60);
	} else {
		return (false);
	}
}

/*
 * Checks if a string is a valid ICAO airport code. ICAO airport codes always:
 * 1) are 4 characters long
 * 2) are all upper case
 * 3) contain only the letters A-Z
 * 4) may not start with I, J, Q or X
 */
static bool
is_valid_icao_code(const char *icao)
{
	if (strlen(icao) != 4)
		return (false);
	for (int i = 0; i < 4; i++)
		if (icao[i] < 'A' || icao[i] > 'Z')
			return (false);
	if (icao[0] == 'I' || icao[0] == 'J' || icao[0] == 'Q' ||
	    icao[0] == 'X')
	        return (false);
	return (true);
}

static void
strip_spaces(char *buf)
{
	CPDLC_ASSERT(buf != NULL);
	for (int i = 0, n = strlen(buf); i < n; i++) {
		if (isspace(buf[i]))
			memmove(&buf[i], &buf[i + 1], (n - i) + 1);
	}
}

const char *
fans_parse_pos(const char *buf, fms_pos_t *pos)
{
	CPDLC_ASSERT(buf != NULL);
	CPDLC_ASSERT(pos != NULL);

	if (strlen(buf) == 0) {
		pos->set = false;
		return (NULL);
	}

	switch (pos->type) {
	case FMS_POS_NAVAID:
		if (strlen(buf) > 4) {
			return ("FORMAT ERROR");
		} else {
			cpdlc_strlcpy(pos->name, buf, sizeof (pos->name));
			pos->set = true;
		}
		break;
	case FMS_POS_ARPT:
		if (!is_valid_icao_code(buf)) {
			return ("FORMAT ERROR");
		} else {
			cpdlc_strlcpy(pos->name, buf, sizeof (pos->name));
			pos->set = true;
		}
		break;
	case FMS_POS_FIX:
		if (strlen(buf) > 5) {
			return ("FORMAT ERROR");
		} else {
			cpdlc_strlcpy(pos->name, buf, sizeof (pos->name));
			pos->set = true;
		}
		break;
	case FMS_POS_LAT_LON: {
		char inbuf[32], lat_buf[16], lon_buf[16];
		const char *lat_str, *lon_str;
		double lat_sign = 1, lon_sign = 1;
		double degs, mins;

		cpdlc_strlcpy(inbuf, buf, sizeof (inbuf));
		strip_spaces(inbuf);

		lat_str = strchr(inbuf, 'N');
		if (lat_str == NULL) {
			lat_str = strchr(inbuf, 'S');
			lat_sign = -1;
		}
		lon_str = strchr(inbuf, 'E');
		if (lon_str == NULL) {
			lon_str = strchr(inbuf, 'W');
			lon_sign = -1;
		}
		if (lat_str == NULL || lon_str == NULL || lat_str > lon_str ||
		    lon_str - lat_str > (intptr_t)sizeof (lat_buf))
			return ("FORMAT ERROR");

		cpdlc_strlcpy(lat_buf, &lat_str[1], lon_str - lat_str);
		cpdlc_strlcpy(lon_buf, &lon_str[1], sizeof (lon_buf));
		if (!parse_deg_mins(lat_buf, 2, &degs, &mins))
			return ("FORMAT ERROR");
		pos->lat = lat_sign * (degs + (mins / 60));
		if (!parse_deg_mins(lon_buf, 3, &degs, &mins))
			return ("FORMAT ERROR");
		pos->lon = lon_sign * (degs + (mins / 60));
		pos->set = true;

		break;
	}
	case FMS_POS_PBD: {
		int brg, dist;
		const char *s1, *s2;

		s1 = strchr(buf, '/');
		s2 = strrchr(buf, '/');
		if (s1 == NULL || s2 == NULL || s1 - buf > 5 ||
		    s2 - s1 > (intptr_t)sizeof (3) || strlen(s2) > 4)
			return ("FORMAT ERROR");
		if (sscanf(&s1[1], "%d", &brg) != 1 ||
		    sscanf(&s2[1], "%d", &dist) != 1 ||
		    brg < 0 || brg >= 360 || dist < 1 || dist > 999) {
			return ("FORMAT ERROR");
		}

		cpdlc_strlcpy(pos->name, buf, (s1 - buf) + 1);
		pos->brg = brg % 360;
		pos->dist = dist;
		pos->set = true;
		break;
	}
	}

	return (NULL);
}
