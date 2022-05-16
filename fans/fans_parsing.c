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

fans_err_t
fans_parse_alt(const char *str, unsigned field_nr, void *data)
{
	cpdlc_alt_t alt;
	cpdlc_alt_t *alt_out;

	CPDLC_ASSERT(str != NULL);
	CPDLC_UNUSED(field_nr);
	CPDLC_ASSERT(data != NULL);
	alt_out = data;

	if (strlen(str) < 2)
		return (FANS_ERR_INVALID_ENTRY);
	if (str[0] == 'F' && str[1] == 'L') {
		if (sscanf(&str[2], "%d", &alt.alt) != 1)
			return (FANS_ERR_INVALID_ENTRY);
		alt.fl = true;
		alt.alt *= 100;
	} else if (str[0] == 'F') {
		if (sscanf(&str[1], "%d", &alt.alt) != 1)
			return (FANS_ERR_INVALID_ENTRY);
		alt.fl = true;
		alt.alt *= 100;
	} else {
		if (sscanf(str, "%d", &alt.alt) != 1)
			return (FANS_ERR_INVALID_ENTRY);
		if (alt.alt < 1000) {
			alt.fl = true;
			alt.alt *= 100;
		} else {
			/* Round to nearest 100 ft */
			alt.fl = false;
			alt.alt = round(alt.alt / 100.0) * 100;
		}
	}
	if (alt.alt < -2000 || alt.alt > 60000)
		return (FANS_ERR_INVALID_ENTRY);

	*alt_out = alt;

	return (FANS_ERR_NONE);
}

fans_err_t
fans_insert_alt_block(fans_t *box, unsigned field_nr, void *data,
    void *userinfo)
{
	cpdlc_alt_t *inalt, *outalt;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(data != NULL);
	inalt = data;
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_alt_t));
	outalt = ((void *)box) + offset;

	if (field_nr >= 2)
		return (FANS_ERR_INVALID_ENTRY);
	if (field_nr == 0) {
		outalt[0] = *inalt;
		outalt[1] = CPDLC_NULL_ALT;
	} else {
		if (/* lower alt must actually be set */
		    CPDLC_IS_NULL_ALT(outalt[0]) ||
		    /* lower alt must really be lower */
		    outalt[0].alt >= inalt->alt ||
		    /* if lower alt is FL, higher must as well */
		    (outalt[0].fl && !inalt->fl)) {
			return (FANS_ERR_INVALID_ENTRY);
		}
		outalt[1] = *inalt;
	}

	return (FANS_ERR_NONE);
}

void
fans_read_alt_block(fans_t *box, void *userinfo, char str[READ_FUNC_BUF_SZ])
{
	cpdlc_alt_t *outalt;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_alt_t));
	outalt = ((void *)box) + offset;

	if (!CPDLC_IS_NULL_ALT(*outalt)) {
		int l = fans_print_alt(outalt, str, READ_FUNC_BUF_SZ, false);
		if (outalt[1].alt != 0) {
			strncat(str, "/", READ_FUNC_BUF_SZ - l - 1);
			l++;
			fans_print_alt(&outalt[1], &str[l],
			    READ_FUNC_BUF_SZ - l, false);
		}
	}
}

fans_err_t
fans_parse_spd(const char *str, unsigned field_nr, void *data)
{
	cpdlc_spd_t *spd;

	CPDLC_ASSERT(str != NULL);
	CPDLC_UNUSED(field_nr);
	CPDLC_ASSERT(data != NULL);
	spd = data;

	if (strlen(str) < 2)
		return (FANS_ERR_INVALID_ENTRY);
	if (str[0] == 'M' || str[0] == '.') {
		if (sscanf(&str[1], "%d", &spd->spd) != 1 ||
		    spd->spd < 10 || spd->spd > 99)
			return (FANS_ERR_INVALID_ENTRY);
		spd->mach = true;
		spd->spd *= 10;
	} else {
		if (sscanf(str, "%d", &spd->spd) != 1 ||
		    spd->spd < 10 || spd->spd > 999)
			return (FANS_ERR_INVALID_ENTRY);
		if (spd->spd < 100) {
			spd->mach = true;
			spd->spd *= 10;
		}
	}

	return (FANS_ERR_NONE);
}

fans_err_t
fans_insert_spd_block(fans_t *box, unsigned field_nr, void *data,
    void *userinfo)
{
	cpdlc_spd_t *inspd, *outspd;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(data != NULL);
	inspd = data;
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_spd_t));
	outspd = ((void *)box) + offset;

	if (field_nr >= 2)
		return (FANS_ERR_INVALID_ENTRY);
	if (field_nr == 0) {
		outspd[0] = *inspd;
		outspd[1] = CPDLC_NULL_SPD;
	} else {
		if (/* both speeds must be mach or airspeed */
		    outspd[0].mach != inspd->mach ||
		    /* lower airspeed must be set */
		    CPDLC_IS_NULL_SPD(outspd[0]) ||
		    /* lower speed must really be lower */
		    (outspd[0].spd >= inspd->spd)) {
			return (FANS_ERR_INVALID_ENTRY);
		}
		outspd[1] = *inspd;
	}

	return (FANS_ERR_NONE);
}

void
fans_read_spd_block(fans_t *box, void *userinfo, char str[READ_FUNC_BUF_SZ])
{
	cpdlc_spd_t *outspd;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_spd_t));
	outspd = ((void *)box) + offset;

	if (!CPDLC_IS_NULL_SPD(*outspd)) {
		int l = fans_print_spd(outspd, str, READ_FUNC_BUF_SZ, false,
		    false);
		if (outspd[1].spd != 0) {
			strncat(str, "/", READ_FUNC_BUF_SZ - l - 1);
			l++;
			fans_print_spd(&outspd[1], &str[l],
			    READ_FUNC_BUF_SZ - l, false, false);
		}
	}
}

fans_err_t
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
			return (FANS_ERR_INVALID_ENTRY);
		}
		out_wind->deg %= 360;
	} else {
		if (sscanf(str, "%d", &out_wind->spd) != 1 ||
		    out_wind->spd > MAX_WIND) {
			return (FANS_ERR_INVALID_ENTRY);
		}
	}

	return (FANS_ERR_NONE);
}

fans_err_t
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

	return (FANS_ERR_NONE);
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

fans_err_t
fans_delete_wind(fans_t *box, void *userinfo)
{
	CPDLC_ASSERT(box != NULL);
	CPDLC_ASSERT(userinfo != NULL);
	memset(userinfo, 0, sizeof (fms_wind_t));
	return (FANS_ERR_NONE);
}

fans_err_t
fans_delete_cpdlc_alt_block(fans_t *box, void *userinfo)
{
	cpdlc_alt_t *outalt;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_alt_t));
	outalt = ((void *)box) + offset;
	outalt[0] = CPDLC_NULL_ALT;
	outalt[1] = CPDLC_NULL_ALT;

	return (FANS_ERR_NONE);
}

fans_err_t
fans_delete_cpdlc_spd_block(fans_t *box, void *userinfo)
{
	cpdlc_spd_t *outspd;
	size_t offset;

	CPDLC_ASSERT(box != NULL);
	offset = (uintptr_t)userinfo;
	CPDLC_ASSERT3U(offset, <=, sizeof (*box) - 2 * sizeof (cpdlc_spd_t));
	outspd = ((void *)box) + offset;
	outspd[0] = CPDLC_NULL_SPD;
	outspd[1] = CPDLC_NULL_SPD;

	return (FANS_ERR_NONE);
}

int
fans_print_alt(const cpdlc_alt_t *alt, char *str, size_t cap, bool units)
{
	CPDLC_ASSERT(alt != NULL);
	if (alt->fl) {
		return (snprintf(str, cap, "FL%d",
		    (int)round(alt->alt / 100.0)));
	}
	return (snprintf(str, cap, "%d%s", alt->alt, units ? "FT" : ""));
}

int
fans_print_spd(const cpdlc_spd_t *spd, char *str, size_t cap, bool pretty,
    bool units)
{
	CPDLC_ASSERT(spd != NULL);
	if (spd->mach) {
		if (pretty) {
			return (snprintf(str, cap, ".%02d MACH",
			    (int)round(spd->spd / 10.0)));
		} else if (units) {
			return (snprintf(str, cap, "M.%02d",
			    (int)round(spd->spd / 10.0)));
		} else {
			return (snprintf(str, cap, ".%02d",
			    (int)round(spd->spd / 10.0)));
		}
	} else {
		if (pretty && units)
			return (snprintf(str, cap, "%03d KT", spd->spd));
		else if (units)
			return (snprintf(str, cap, "%03dKT", spd->spd));
		else
			return (snprintf(str, cap, "%03d", spd->spd));
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
fans_print_pos(const cpdlc_pos_t *pos, char *buf, size_t bufsz,
    pos_print_style_t style)
{
	CPDLC_ASSERT(pos != NULL);
	CPDLC_ASSERT(buf != NULL);

	if (!pos->set) {
		if (style == POS_PRINT_PRETTY) {
			switch (pos->type) {
			case CPDLC_POS_NAVAID:
			case CPDLC_POS_AIRPORT:
				cpdlc_strlcpy(buf, "____", bufsz);
				break;
			case CPDLC_POS_FIXNAME:
				cpdlc_strlcpy(buf, "_____", bufsz);
				break;
			case CPDLC_POS_LAT_LON:
				snprintf(buf, bufsz, "___`__.__ ____`__.__");
				break;
			case CPDLC_POS_PBD:
				snprintf(buf, bufsz, "___/___/___");
				break;
			default:
				CPDLC_VERIFY_MSG(0, "Invalid position type %x",
				    pos->type);
			}
		} else {
			buf[0] = '\0';
		}
		return;
	}
	switch (pos->type) {
	case CPDLC_POS_NAVAID:
		cpdlc_strlcpy(buf, pos->navaid, bufsz);
		break;
	case CPDLC_POS_AIRPORT:
		cpdlc_strlcpy(buf, pos->airport, bufsz);
		break;
	case CPDLC_POS_FIXNAME:
		cpdlc_strlcpy(buf, pos->fixname, bufsz);
		break;
	case CPDLC_POS_LAT_LON: {
		int lat = fabs(pos->lat_lon.lat);
		int lon = fabs(pos->lat_lon.lon);
		char ns = (pos->lat_lon.lat >= 0 ? 'N' : 'S');
		char ew = (pos->lat_lon.lon >= 0 ? 'E' : 'W');
		double lat_mins = fabs(pos->lat_lon.lat -
		    trunc(pos->lat_lon.lat)) * 60;
		double lon_mins = fabs(pos->lat_lon.lon -
		    trunc(pos->lat_lon.lon)) * 60;

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
	case CPDLC_POS_PBD:
		snprintf(buf, bufsz, "%s/%03d/%.*f", pos->pbd.fixname,
		    pos->pbd.degrees, pos->pbd.dist_nm < 99.95 ? 1 : 0,
		    pos->pbd.dist_nm);
		break;
	case CPDLC_POS_UNKNOWN:
		cpdlc_strlcpy(buf, pos->str, bufsz);
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

fans_err_t
fans_parse_pos(const char *buf, cpdlc_pos_t *pos)
{
	CPDLC_ASSERT(buf != NULL);
	CPDLC_ASSERT(pos != NULL);

	if (strlen(buf) == 0) {
		pos->set = false;
		return (FANS_ERR_NONE);
	}
	switch (pos->type) {
	case CPDLC_POS_NAVAID:
		if (strlen(buf) > 4)
			return (FANS_ERR_INVALID_ENTRY);
		pos->set = true;
		cpdlc_strlcpy(pos->navaid, buf, sizeof (pos->navaid));
		break;
	case CPDLC_POS_AIRPORT:
		if (!is_valid_icao_code(buf))
			return (FANS_ERR_INVALID_ENTRY);
		pos->set = true;
		cpdlc_strlcpy(pos->airport, buf, sizeof (pos->airport));
		break;
	case CPDLC_POS_FIXNAME:
		if (strlen(buf) > 5)
			return (FANS_ERR_INVALID_ENTRY);
		pos->set = true;
		cpdlc_strlcpy(pos->fixname, buf, sizeof (pos->fixname));
		break;
	case CPDLC_POS_LAT_LON: {
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
		    lon_str - lat_str > (intptr_t)sizeof (lat_buf)) {
			return (FANS_ERR_INVALID_ENTRY);
		}

		cpdlc_strlcpy(lat_buf, &lat_str[1], lon_str - lat_str);
		cpdlc_strlcpy(lon_buf, &lon_str[1], sizeof (lon_buf));
		if (!parse_deg_mins(lat_buf, 2, &degs, &mins))
			return (FANS_ERR_INVALID_ENTRY);
		pos->lat_lon.lat = lat_sign * (degs + (mins / 60));
		if (!parse_deg_mins(lon_buf, 3, &degs, &mins))
			return (FANS_ERR_INVALID_ENTRY);
		pos->set = true;
		pos->lat_lon.lon = lon_sign * (degs + (mins / 60));

		break;
	}
	case CPDLC_POS_PBD: {
		int brg;
		double dist_nm;
		const char *s1, *s2;

		s1 = strchr(buf, '/');
		s2 = strrchr(buf, '/');
		if (s1 == NULL || s2 == NULL || s1 - buf > 5 ||
		    s2 - s1 > (intptr_t)sizeof (3) || strlen(s2) > 4) {
			return (FANS_ERR_INVALID_ENTRY);
		}
		if (sscanf(&s1[1], "%d", &brg) != 1 ||
		    sscanf(&s2[1], "%lf", &dist_nm) != 1 ||
		    brg <= 0 || brg > 360 || dist_nm < 0.1 || dist_nm > 999) {
			return (FANS_ERR_INVALID_ENTRY);
		}
		cpdlc_strlcpy(pos->pbd.fixname, buf,
		    MIN(sizeof (pos->pbd.fixname), (unsigned)(s1 - buf) + 1));
		pos->pbd.degrees = brg;
		pos->pbd.dist_nm = dist_nm;
		pos->set = true;
		break;
	}
	case CPDLC_POS_UNKNOWN:
		CPDLC_VERIFY_MSG(0, "Invalid position type %x", pos->type);
	}

	return (FANS_ERR_NONE);
}
