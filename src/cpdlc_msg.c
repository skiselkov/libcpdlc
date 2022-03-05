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
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "cpdlc_alloc.h"
#include "cpdlc_assert.h"
#include "cpdlc_string.h"
#include "cpdlc_msg.h"

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

static const cpdlc_msg_info_t *
msg_infos_lookup(bool is_dl, int msg_type, char msg_subtype)
{
	const cpdlc_msg_info_t *infos =
	    (is_dl ? cpdlc_dl_infos : cpdlc_ul_infos);

	CPDLC_ASSERT3S(msg_type, >=, 0);
	if (is_dl) {
		CPDLC_ASSERT3S(msg_type, <=,
		    CPDLC_DM80_DEVIATING_dir_dist_OF_ROUTE);
	} else {
		CPDLC_ASSERT3S(msg_type, <=,
		    CPDLC_UM208_FREETEXT_LOW_URG_LOW_ALERT_text);
	}
	if (is_dl && msg_type == 67) {
		CPDLC_ASSERT(msg_subtype == 0 ||
		    (msg_subtype >= CPDLC_DM67b_WE_CAN_ACPT_alt_AT_time &&
		    msg_subtype <= CPDLC_DM67i_WHEN_CAN_WE_EXPCT_DES_TO_alt));
	}

	for (int i = 0; infos[i].msg_type != -1; i++) {
		const cpdlc_msg_info_t *info = &infos[i];

		if (info->is_dl == is_dl && info->msg_type == msg_type &&
		    info->msg_subtype == msg_subtype) {
			return (info);
		}
	}

	return (NULL);
}

unsigned
cpdlc_escape_percent(const char *in_buf, char *out_buf, unsigned cap)
{
	CPDLC_ASSERT(in_buf != NULL);

	for (unsigned i = 0, j = 0;; i++, j++) {
		char c = in_buf[i];

		if (c == '\0') {
			if (j < cap)
				out_buf[j] = '\0';
			return (j + 1);
		}
		if (j + 1 == cap) {
			/* Terminate the output in case of short outbuf */
			out_buf[j] = '\0';
		}
		if (isalnum(c) || c == '.' || c == ',') {
			if (j + 1 < cap)
				out_buf[j] = in_buf[i];
		} else {
			if (j + 4 < cap)
				snprintf(&out_buf[j], 4, "%%%02x", c);
			j += 2;
		}
	}
}

int
cpdlc_unescape_percent(const char *in_buf, char *out_buf, unsigned cap)
{
	unsigned j = 0;

	for (unsigned i = 0, n = strlen(in_buf); i <= n; i++, j++) {
		char c = in_buf[i];

		if (c == '%') {
			char buf[4];
			int v;

			if (i + 2 > n)
				return (-1);
			buf[0] = in_buf[i + 1];
			buf[1] = in_buf[i + 2];
			buf[2] = '\0';
			if (!isxdigit(buf[0]) || !isxdigit(buf[1]) ||
			    sscanf(buf, "%02x", &v) != 1 ||
			    v == 0 /* Don't allow NUL bytes */)
				return (-1);
			if (j < cap)
				out_buf[j] = v;
			i += 2;
		} else if (j < cap) {
			out_buf[j] = c;
		}
		if (j + 1 == cap)
			out_buf[j] = '\0';
	}

	return (j);
}

static void
serialize_lat_lon(const cpdlc_lat_lon_t *lat_lon, bool readable,
    unsigned *len_p, char **outbuf_p, unsigned *cap_p)
{
	CPDLC_ASSERT(lat_lon != NULL);
	CPDLC_ASSERT(len_p != NULL);
	CPDLC_ASSERT(outbuf_p != NULL);
	CPDLC_ASSERT(cap_p != NULL);

	if (readable) {
		APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
		    "%c%02.0f" CPDLC_DEG_SYMBOL "%04.1f "
		    "%c%03.0f" CPDLC_DEG_SYMBOL "%04.1f ",
		    lat_lon->lat >= 0 ? 'N' : 'S', trunc(ABS(lat_lon->lat)),
		    fabs(lat_lon->lat - trunc(lat_lon->lat)),
		    lat_lon->lon >= 0 ? 'E' : 'W', trunc(ABS(lat_lon->lon)),
		    fabs(lat_lon->lon - trunc(lat_lon->lon)));
	} else {
		APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
		    "LATLON:%.4f,%.4f ", lat_lon->lat, lat_lon->lon);
	}
}

static void
serialize_pb(const cpdlc_pb_t *pb, bool readable, char *str, unsigned cap)
{
	CPDLC_ASSERT(pb != NULL);
	CPDLC_ASSERT(str != NULL);
	if (readable) {
		snprintf(str, cap, "%s%03d", pb->fixname, pb->degrees);
	} else if (!CPDLC_IS_NULL_LAT_LON(pb->lat_lon)) {
		snprintf(str, cap, "%s/%03d(%.4f,%.4f)", pb->fixname,
		    pb->degrees, pb->lat_lon.lat, pb->lat_lon.lon);
	} else {
		snprintf(str, cap, "%s/%03d", pb->fixname, pb->degrees);
	}
}

static void
serialize_pbd(const cpdlc_pbd_t *pbd, bool readable, unsigned *len_p,
    char **outbuf_p, unsigned *cap_p)
{
	CPDLC_ASSERT(pbd != NULL);
	CPDLC_ASSERT(len_p != NULL);
	CPDLC_ASSERT(outbuf_p != NULL);
	CPDLC_ASSERT(cap_p != NULL);
	if (readable) {
		APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p, "%s%03d/%.1f ",
		    pbd->fixname, pbd->degrees, pbd->dist_nm);
	} else if (!CPDLC_IS_NULL_LAT_LON(pbd->lat_lon)) {
		APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
		    "PBD:%s/%03d/%.1f(%.4f,%.4f) ", pbd->fixname, pbd->degrees,
		    pbd->dist_nm, pbd->lat_lon.lat, pbd->lat_lon.lon);
	} else {
		APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
		    "PBD:%s/%03d/%.1f ", pbd->fixname, pbd->degrees,
		    pbd->dist_nm);
	}
}

static void
serialize_trk_detail(const cpdlc_trk_detail_t *trk, bool readable,
    unsigned *len_p, char **outbuf_p, unsigned *cap_p)
{
	CPDLC_ASSERT(trk != NULL);
	CPDLC_ASSERT(len_p != NULL);
	CPDLC_ASSERT(outbuf_p != NULL);
	CPDLC_ASSERT(cap_p != NULL);

	if (readable) {
		APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p, "(%s) ", trk->name);
	} else {
		APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p, "TRK:%s(",
		    trk->name);
		for (unsigned i = 0; i < trk->num_lat_lon; i++) {
			APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
			    "%.4f,%.4f%s",
			    trk->lat_lon[i].lat, trk->lat_lon[i].lon,
			    i + 1< trk->num_lat_lon ? "," : "");
		}
		APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p, ") ");
	}
}

static void
serialize_route_info(const cpdlc_route_t *route,
    const cpdlc_route_info_t *info, bool readable,
    unsigned *len_p, char **outbuf_p, unsigned *cap_p)
{
	CPDLC_ASSERT(route != NULL);
	CPDLC_ASSERT(info != NULL);
	CPDLC_ASSERT(len_p != NULL);
	CPDLC_ASSERT(outbuf_p != NULL);
	CPDLC_ASSERT(cap_p != NULL);

	switch (info->type) {
	case CPDLC_ROUTE_PUB_IDENT:
		if (readable) {
			APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p, "%s ",
			    info->pub_ident.fixname);
		} else {
			APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p, "FIX:%s",
			    info->pub_ident.fixname);
			if (!CPDLC_IS_NULL_LAT_LON(info->pub_ident.lat_lon)) {
				APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
				    "(%.4f,%.4f)", info->pub_ident.lat_lon.lat,
				    info->pub_ident.lat_lon.lon);
			}
			APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p, " ");
		}
		break;
	case CPDLC_ROUTE_LAT_LON:
		serialize_lat_lon(&info->lat_lon, readable,
		    len_p, outbuf_p, cap_p);
		break;
	case CPDLC_ROUTE_PBPB: {
		char comps[2][32];

		for (int i = 0; i < 2; i++) {
			serialize_pb(&info->pbpb[i], readable, comps[i],
			    sizeof (comps[i]));
		}
		if (readable) {
			APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
			    "%s/%s ", comps[0], comps[1]);
		} else {
			APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
			    "PBPB:%s/%s ", comps[0], comps[1]);
		}
		break;
	}
	case CPDLC_ROUTE_PBD:
		serialize_pbd(&info->pbd, readable, len_p, outbuf_p, cap_p);
		break;
	case CPDLC_ROUTE_AWY:
		if (readable) {
			APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
			    "%s ", info->awy);
		} else {
			APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p,
			    "AWY:%s ", info->awy);
		}
		break;
	case CPDLC_ROUTE_TRACK_DETAIL:
		serialize_trk_detail(&route->trk_detail, readable,
		    len_p, outbuf_p, cap_p);
		break;
	case CPDLC_ROUTE_UNKNOWN:
		APPEND_SNPRINTF(*len_p, *outbuf_p, *cap_p, "%s ", info->str);
		break;
	default:
		CPDLC_VERIFY_MSG(0, "Invalid route info type %x", info->type);
	}
}

static void
serialize_route(const cpdlc_route_t *route, bool readable,
    char *outbuf, unsigned cap)
{
	unsigned len = 0;

	CPDLC_ASSERT(route != NULL);
	CPDLC_ASSERT(outbuf != NULL);

	if (route->orig_icao[0] != '\0') {
		APPEND_SNPRINTF(len, outbuf, cap, "%s%s ",
		    !readable ? "ADEP:" : "", route->orig_icao);
	}
	if (route->dest_icao[0] != '\0' && !readable) {
		APPEND_SNPRINTF(len, outbuf, cap, "ADES:%s ",
		    route->dest_icao);
	}
	if (route->orig_rwy[0] != '\0') {
		APPEND_SNPRINTF(len, outbuf, cap, "%s%s ",
		    readable ? "RW" : "DEPRWY:", route->orig_rwy);
	}
	if (route->sid.name[0] != '\0') {
		APPEND_SNPRINTF(len, outbuf, cap, "%s%s%s%s ",
		    !readable ? "SID:" : "", route->sid.name,
		    readable && route->sid.trans[0] != '\0' ? "." : "",
		    readable ? route->sid.trans : "");
		if (route->sid.trans[0] != '\0' && !readable) {
			APPEND_SNPRINTF(len, outbuf, cap, "SIDTR:%s ",
			    route->sid.trans);
		}
	}
	for (unsigned i = 0; i < route->num_info; i++) {
		serialize_route_info(route, &route->info[i], readable,
		    &len, &outbuf, &cap);
	}
	if (route->star.name[0] != '\0') {
		APPEND_SNPRINTF(len, outbuf, cap, "%s%s%s%s ",
		    !readable ? "STAR:" : "",
		    readable ? route->star.trans : "",
		    readable && route->star.trans[0] != '\0' ? "." : "",
		    route->star.name);
		if (route->star.trans[0] != '\0' && !readable) {
			APPEND_SNPRINTF(len, outbuf, cap, "STARTR:%s ",
			    route->star.trans);
		}
	}
	if (route->appch.name[0] != '\0') {
		APPEND_SNPRINTF(len, outbuf, cap, "%s%s%s%s ",
		    !readable ? "APP:" : "", readable ? route->appch.trans : "",
		    readable && route->appch.trans[0] != '\0' ? "." : "",
		    route->appch.name);
		if (route->appch.trans[0] != '\0' && !readable) {
			APPEND_SNPRINTF(len, outbuf, cap, "APPTR:%s ",
			    route->appch.trans);
		}
	}
	/*
	 * Bit of a special case for human readability.
	 * Empty route implies a DIRECT route.
	 */
	if (route->sid.name[0] == '\0' && route->star.name[0] == '\0' &&
	    route->appch.name[0] == '\0' && route->num_info == 0) {
		APPEND_SNPRINTF(len, outbuf, cap, readable ? "DIRECT" : " ");
	}
	if (route->dest_icao[0] != '\0' && readable)
		APPEND_SNPRINTF(len, outbuf, cap, "%s", route->dest_icao);
}

static void
serialize_pos(const cpdlc_pos_t *pos, bool readable, char *outbuf, unsigned cap)
{
	unsigned n = 0;

	CPDLC_ASSERT(pos != NULL);
	CPDLC_ASSERT(outbuf != NULL);

	switch (pos->type) {
	case CPDLC_POS_FIXNAME:
		APPEND_SNPRINTF(n, outbuf, cap, "%s%s",
		    readable ? "" : "FIX:", pos->fixname);
		break;
	case CPDLC_POS_NAVAID:
		APPEND_SNPRINTF(n, outbuf, cap, "%s%s",
		    readable ? "" : "NAV:", pos->navaid);
		break;
	case CPDLC_POS_AIRPORT:
		APPEND_SNPRINTF(n, outbuf, cap, "%s%s",
		    readable ? "" : "ARPT:", pos->airport);
		break;
	case CPDLC_POS_LAT_LON:
		serialize_lat_lon(&pos->lat_lon, readable, &n, &outbuf, &cap);
		break;
	case CPDLC_POS_PBD:
		serialize_pbd(&pos->pbd, readable, &n, &outbuf, &cap);
		break;
	case CPDLC_POS_UNKNOWN:
		APPEND_SNPRINTF(n, outbuf, cap, "%s", pos->str);
		break;
	}
}

static void
encode_alt(const cpdlc_alt_t *alt, bool readable, unsigned *n_bytes_p,
    char **buf_p, unsigned *cap_p)
{
	CPDLC_ASSERT(alt != NULL);
	CPDLC_ASSERT(n_bytes_p != NULL);
	CPDLC_ASSERT(buf_p != NULL);
	CPDLC_ASSERT(cap_p != NULL);

	if (readable) {
		const char *units;
		int value;

		if (alt->fl) {
			if (alt->met) {
				value = alt->alt;
				units = " M";
			} else {
				value = alt->alt / 100;
				units = "";
			}
		} else {
			if (alt->met) {
				value = alt->alt;
				units = " M";
			} else {
				value = alt->alt;
				units = " FT";
			}
		}
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s%d%s",
		    alt->fl ? "FL" : "", value, units);
	} else {
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s%d%s",
		    alt->fl ? "FL" : "", (alt->fl && !alt->met) ?
		    alt->alt / 100 : alt->alt, alt->met ? "M" : "");
	}
}

static void
encode_spd(const cpdlc_spd_t *spd, bool readable, unsigned *n_bytes_p,
    char **buf_p, unsigned *cap_p)
{
	CPDLC_ASSERT(spd != NULL);
	CPDLC_ASSERT(n_bytes_p != NULL);
	CPDLC_ASSERT(buf_p != NULL);
	CPDLC_ASSERT(cap_p != NULL);

	if (spd->mach) {
		if (spd->spd < 1000) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    "%sM.%03d", readable ? "" : " ", spd->spd);
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    "%sM%d.%03d", readable ? "" : " ",
			    spd->spd / 1000, spd->spd % 1000);
		}
	} else {
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    "%d KT", spd->spd);
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    " %d", spd->spd);
		}
	}
}

static const char *
turb2str(cpdlc_turb_t turb)
{
	switch (turb) {
	case CPDLC_TURB_NONE:
		return ("NIL");
	case CPDLC_TURB_LIGHT:
		return ("LGT");
	case CPDLC_TURB_MOD:
		return ("MOD");
	case CPDLC_TURB_SEV:
		return ("SEV");
	}
	CPDLC_VERIFY(0);
}

static const char *
icing2str(cpdlc_icing_t icing)
{
	switch (icing) {
	case CPDLC_ICING_NONE:
		return ("NIL");
	case CPDLC_ICING_TRACE:
		return ("TRACE");
	case CPDLC_ICING_LIGHT:
		return ("LGT");
	case CPDLC_ICING_MOD:
		return ("MOD");
	case CPDLC_ICING_SEV:
		return ("SEV");
	}
	CPDLC_VERIFY(0);
}

static void
serialize_posreport_readable(const cpdlc_pos_rep_t *rep, char *buf,
    unsigned cap)
{
	unsigned n = 0;
	char posbuf[128] = {};

	CPDLC_ASSERT(rep != NULL);
	CPDLC_ASSERT(buf != NULL);

	serialize_pos(&rep->cur_pos, true, posbuf, sizeof (posbuf));
	APPEND_SNPRINTF(n, buf, cap, "%s", posbuf);

	APPEND_SNPRINTF(n, buf, cap, "AT %02d%02dZ ",
	    rep->time_cur_pos.hrs, rep->time_cur_pos.mins);
	encode_alt(&rep->cur_alt, true, &n, &buf, &cap);
	APPEND_SNPRINTF(n, buf, cap, " ");
	if (!CPDLC_IS_NULL_SPD(rep->spd)) {
		APPEND_SNPRINTF(n, buf, cap, "TAS ");
		encode_spd(&rep->spd, true, &n, &buf, &cap);
		APPEND_SNPRINTF(n, buf, cap, " ");
	}
	if (!CPDLC_IS_NULL_SPD(rep->spd_gnd)) {
		APPEND_SNPRINTF(n, buf, cap, "GS ");
		encode_spd(&rep->spd_gnd, true, &n, &buf, &cap);
		APPEND_SNPRINTF(n, buf, cap, " ");
	}
	if (rep->vvi_set && ABS(rep->vvi) >= 200) {
		APPEND_SNPRINTF(n, buf, cap, "%s %d FPM ",
		    rep->vvi > 0 ? "CLB" : "DES", ABS(rep->vvi));
	}
	if (rep->trk != 0)
		APPEND_SNPRINTF(n, buf, cap, "TRK %03d ", rep->trk);
	if (rep->rpt_wpt_pos.set) {
		serialize_pos(&rep->rpt_wpt_pos, true, posbuf, sizeof (posbuf));
		APPEND_SNPRINTF(n, buf, cap, "RPT %s ", posbuf);
		if (!CPDLC_IS_NULL_TIME(rep->rpt_wpt_time)) {
			APPEND_SNPRINTF(n, buf, cap, "AT %02d%02dZ ",
			    rep->rpt_wpt_time.hrs, rep->rpt_wpt_time.mins);
		}
		if (!CPDLC_IS_NULL_ALT(rep->rpt_wpt_alt))
			encode_alt(&rep->rpt_wpt_alt, true, &n, &buf, &cap);
	}
	if (rep->fix_next.set) {
		serialize_pos(&rep->fix_next, true, posbuf, sizeof (posbuf));
		APPEND_SNPRINTF(n, buf, cap, "NEXT %s ", posbuf);
		if (!CPDLC_IS_NULL_TIME(rep->time_fix_next)) {
			APPEND_SNPRINTF(n, buf, cap, "AT %02d%02dZ ",
			    rep->time_fix_next.hrs, rep->time_fix_next.mins);
		}
		if (rep->dist_set) {
			APPEND_SNPRINTF(n, buf, cap, "DTG %.*f ",
			    rep->dist_nm < 99.95 ? 1 : 0, rep->dist_nm);
		}
	}
	if (rep->fix_next_p1.set) {
		serialize_pos(&rep->fix_next_p1, true, posbuf, sizeof (posbuf));
		APPEND_SNPRINTF(n, buf, cap, "NEXT+1 %s ", posbuf);
	}
	if (!CPDLC_IS_NULL_TIME(rep->time_dest)) {
		APPEND_SNPRINTF(n, buf, cap, "DEST %02d%02dZ ",
		    rep->time_dest.hrs, rep->time_dest.mins);
	}
	if (!CPDLC_IS_NULL_TIME(rep->rmng_fuel)) {
		APPEND_SNPRINTF(n, buf, cap, "FUEL %02dH%02dM ",
		    rep->rmng_fuel.hrs, rep->rmng_fuel.mins);
	}
	if (!CPDLC_IS_NULL_TEMP(rep->temp))
		APPEND_SNPRINTF(n, buf, cap, "OAT %+dC ", rep->temp);
	if (!CPDLC_IS_NULL_WIND(rep->wind)) {
		APPEND_SNPRINTF(n, buf, cap, "WIND %03d/%d ",
		    rep->wind.dir, rep->wind.spd);
	}
	if (rep->turb != CPDLC_TURB_NONE)
		APPEND_SNPRINTF(n, buf, cap, "TURB %s ", turb2str(rep->turb));
	if (rep->icing != CPDLC_ICING_NONE)
		APPEND_SNPRINTF(n, buf, cap, "ICG %s ", icing2str(rep->icing));
}

static void
serialize_posreport_machine(const cpdlc_pos_rep_t *rep, char *buf,
    unsigned cap)
{
	char wkbuf[512] = {};
	unsigned n = 0;

	CPDLC_ASSERT(rep != NULL);
	CPDLC_ASSERT(buf != NULL);
	/* [0] cur_pos */
	serialize_pos(&rep->cur_pos, false, wkbuf, sizeof (wkbuf));
	APPEND_SNPRINTF(n, buf, cap, "%s", wkbuf);
	/* [1] time_cur_pos */
	APPEND_SNPRINTF(n, buf, cap, "%02d%02d",
	    rep->time_cur_pos.hrs, rep->time_cur_pos.mins);
	/* [2] cur_alt */
	encode_alt(&rep->cur_alt, false, &n, &buf, &cap);
	APPEND_SNPRINTF(n, buf, cap, " ");
	/* [3] fix_next */
	if (rep->fix_next.set) {
		serialize_pos(&rep->fix_next, false, wkbuf, sizeof (wkbuf));
		APPEND_SNPRINTF(n, buf, cap, "%s ", wkbuf);
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [4] time_fix_next */
	if (!CPDLC_IS_NULL_TIME(rep->time_fix_next)) {
		APPEND_SNPRINTF(n, buf, cap, "%02d%02d ",
		    rep->time_fix_next.hrs, rep->time_fix_next.mins);
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [5] fix_next_p1 */
	if (rep->fix_next_p1.set) {
		serialize_pos(&rep->fix_next_p1, false, wkbuf, sizeof (wkbuf));
		APPEND_SNPRINTF(n, buf, cap, "%s ", wkbuf);
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [6] time_dest */
	if (!CPDLC_IS_NULL_TIME(rep->time_dest)) {
		APPEND_SNPRINTF(n, buf, cap, "%02d%02d ",
		    rep->time_dest.hrs, rep->time_dest.mins);
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [7] rmng_fuel */
	if (!CPDLC_IS_NULL_TIME(rep->rmng_fuel)) {
		APPEND_SNPRINTF(n, buf, cap, "%02d%02d ",
		    rep->rmng_fuel.hrs, rep->rmng_fuel.mins);
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [8] temp */
	if (!CPDLC_IS_NULL_TEMP(rep->temp))
		APPEND_SNPRINTF(n, buf, cap, "%d ", rep->temp);
	else
		APPEND_SNPRINTF(n, buf, cap, "- ");
	/* [9] wind */
	if (!CPDLC_IS_NULL_WIND(rep->wind)) {
		APPEND_SNPRINTF(n, buf, cap, "%03d/%d ",
		    rep->wind.dir, rep->wind.spd);
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [10] turb */
	APPEND_SNPRINTF(n, buf, cap, "%d ", rep->turb);
	/* [11] icing */
	APPEND_SNPRINTF(n, buf, cap, "%d ", rep->icing);
	/* [12] spd */
	if (!CPDLC_IS_NULL_SPD(rep->spd)) {
		encode_spd(&rep->spd, false, &n, &buf, &cap);
		APPEND_SNPRINTF(n, buf, cap, " ");
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [13] spd_gnd */
	if (!CPDLC_IS_NULL_SPD(rep->spd_gnd)) {
		encode_spd(&rep->spd_gnd, false, &n, &buf, &cap);
		APPEND_SNPRINTF(n, buf, cap, " ");
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [14] vvi */
	if (rep->vvi_set)
		APPEND_SNPRINTF(n, buf, cap, "%d ", rep->vvi);
	else
		APPEND_SNPRINTF(n, buf, cap, "- ");
	/* [15] trk */
	if (rep->trk != 0)
		APPEND_SNPRINTF(n, buf, cap, "%d ", rep->trk);
	else
		APPEND_SNPRINTF(n, buf, cap, "- ");
	/* [16] hdg_true */
	if (rep->hdg_true != 0)
		APPEND_SNPRINTF(n, buf, cap, "%d ", rep->hdg_true);
	else
		APPEND_SNPRINTF(n, buf, cap, "- ");
	/* [17] dist */
	if (rep->dist_set)
		APPEND_SNPRINTF(n, buf, cap, "%.1f ", rep->dist_nm);
	else
		APPEND_SNPRINTF(n, buf, cap, "- ");
	/* [18] remarks */
	if (rep->remarks[0] != '\0') {
		cpdlc_escape_percent(rep->remarks, wkbuf, sizeof (wkbuf));
		APPEND_SNPRINTF(n, buf, cap, "%s ", wkbuf);
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [19] rpt_wpt_pos */
	if (rep->rpt_wpt_pos.set) {
		serialize_pos(&rep->rpt_wpt_pos, false, wkbuf, sizeof (wkbuf));
		APPEND_SNPRINTF(n, buf, cap, "%s ", wkbuf);
	} else {
		APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [20] rpt_wpt_time */
	if (!CPDLC_IS_NULL_TIME(rep->rpt_wpt_time)) {
		APPEND_SNPRINTF(n, buf, cap, "%02d%02d ",
		    rep->rpt_wpt_time.hrs, rep->rpt_wpt_time.mins);
	} else {
			APPEND_SNPRINTF(n, buf, cap, "- ");
	}
	/* [21] rpt_wpt_alt */
	if (!CPDLC_IS_NULL_ALT(rep->rpt_wpt_alt))
		encode_alt(&rep->rpt_wpt_alt, false, &n, &buf, &cap);
	else
		APPEND_SNPRINTF(n, buf, cap, "- ");
}

static void
serialize_posreport(const cpdlc_pos_rep_t *rep, bool readable,
    char *buf, unsigned cap)
{
	CPDLC_ASSERT(rep != NULL);
	CPDLC_ASSERT(buf != NULL);
	if (readable)
		serialize_posreport_readable(rep, buf, cap);
	else
		serialize_posreport_machine(rep, buf, cap);
}

void
cpdlc_encode_msg_arg(const cpdlc_arg_type_t arg_type, const cpdlc_arg_t *arg,
    bool readable, unsigned *n_bytes_p, char **buf_p, unsigned *cap_p)
{
	char textbuf[8192] = {};

	switch (arg_type) {
	case CPDLC_ARG_ALTITUDE:
		encode_alt(&arg->alt, readable, n_bytes_p, buf_p, cap_p);
		break;
	case CPDLC_ARG_SPEED:
		encode_spd(&arg->spd, readable, n_bytes_p, buf_p, cap_p);
		break;
	case CPDLC_ARG_TIME:
		if (arg->time.hrs < 0) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%sNOW",
			    readable ? "" : " ");
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    "%s%02d%02d", readable ? "" : " ",
			    arg->time.hrs, arg->time.mins);
		}
		break;
	case CPDLC_ARG_TIME_DUR:
		if (readable) {
			if (arg->time.hrs == 0) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%d MINUTES", arg->time.mins);
			} else if (arg->time.mins == 0) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%d HOURS", arg->time.hrs);
			} else {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%d HOURS %d MINUTES", arg->time.hrs,
				    arg->time.mins);
			}
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    " %d", arg->time.hrs * 60 + arg->time.mins);
		}
		break;
	case CPDLC_ARG_POSITION: {
		char posbuf[128] = {};
		serialize_pos(&arg->pos, readable, posbuf, sizeof (posbuf));
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s",
			    posbuf);
		} else {
			cpdlc_escape_percent(posbuf, textbuf, sizeof (textbuf));
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s",
			    textbuf);
		}
		break;
	}
	case CPDLC_ARG_DIRECTION: {
		const char *name;
		switch (arg->dir) {
		case CPDLC_DIR_LEFT:
			name = (readable ? "LEFT" : " L");
			break;
		case CPDLC_DIR_RIGHT:
			name = (readable ? "RIGHT" : " R");
			break;
		case CPDLC_DIR_EITHER:
			name = (readable ? "EITHER" : " -");
			break;
		case CPDLC_DIR_NORTH:
			name = (readable ? "NORTH" : " N");
			break;
		case CPDLC_DIR_SOUTH:
			name = (readable ? "SOUTH" : " S");
			break;
		case CPDLC_DIR_EAST:
			name = (readable ? "EAST" : " E");
			break;
		case CPDLC_DIR_WEST:
			name = (readable ? "WEST" : " W");
			break;
		case CPDLC_DIR_NE:
			name = (readable ? "NORTHEAST" : " NE");
			break;
		case CPDLC_DIR_NW:
			name = (readable ? "NORTHWEST" : " NW");
			break;
		case CPDLC_DIR_SE:
			name = (readable ? "SOUTHEAST" : " SE");
			break;
		case CPDLC_DIR_SW:
			name = (readable ? "SOUTHWEST" : " SW");
			break;
		default:
			CPDLC_VERIFY_MSG(0, "Invalid direction %x", arg->dir);
		}
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s", name);
		break;
	}
	case CPDLC_ARG_DISTANCE:
		if (readable) {
			if (arg->dist - (int)arg->dist == 0) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%.0f NM", arg->dist);
			} else {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%.01f NM", arg->dist);
			}
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    " %.01f", arg->dist);
		}
		break;
	case CPDLC_ARG_VVI:
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    "%d FPM", arg->vvi);
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    " %d", arg->vvi);
		}
		break;
	case CPDLC_ARG_TOFROM:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s%s",
		    readable ? "" : " ", arg->tofrom ? "TO" : "FROM");
		break;
	case CPDLC_ARG_ROUTE:
		if (arg->route != NULL) {
			char routebuf[8192] = {};
			if (readable) {
				serialize_route(arg->route, true,
				    routebuf, sizeof (routebuf));
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%s", routebuf);
			} else {
				serialize_route(arg->route, false,
				    routebuf, sizeof (routebuf));
				cpdlc_escape_percent(routebuf, textbuf,
				    sizeof (textbuf));
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    " %s", textbuf);
			}
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%sNIL",
			    readable ? "" : " ");
		}
		break;
	case CPDLC_ARG_PROCEDURE:
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s",
			    arg->proc);
		} else {
			cpdlc_escape_percent(arg->proc, textbuf,
			    sizeof (textbuf));
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s",
			    textbuf);
		}
		break;
	case CPDLC_ARG_SQUAWK:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s%04d",
		    readable ? "" : " ", arg->squawk);
		break;
	case CPDLC_ARG_ICAONAME:
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s %s",
			    arg->icaoname.icao, arg->icaoname.name);
		} else {
			cpdlc_escape_percent(arg->icaoname.icao, textbuf,
			    sizeof (textbuf));
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s",
			    textbuf);
			cpdlc_escape_percent(arg->icaoname.name, textbuf,
			    sizeof (textbuf));
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s",
			    textbuf);
		}
		break;
	case CPDLC_ARG_FREQUENCY:
		if (readable) {
			if (arg->freq <= 21) {
				/* HF */
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%.04f MHZ", arg->freq);
			} else {
				/* VHF/UHF */
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%.03f MHZ", arg->freq);
			}
		} else {
			if (arg->freq <= 21) {
				/* HF */
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    " %.04f", arg->freq);
			} else {
				/* VHF/UHF */
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    " %.03f", arg->freq);
			}
		}
		break;
	case CPDLC_ARG_DEGREES:
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%03d%s",
			    arg->deg.deg, arg->deg.tru ? " TRUE" : "");
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %d%s",
			    arg->deg.deg, arg->deg.tru ? "T" : "");
		}
		break;
	case CPDLC_ARG_BARO:
		if (readable) {
			if (arg->baro.hpa) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%.0f HPA", arg->baro.val);
			} else {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%.02f IN", arg->baro.val);
			}
		} else {
			if (arg->baro.hpa) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    " Q%.0f", arg->baro.val);
			} else {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    " A%.02f", arg->baro.val);
			}
		}
		break;
	case CPDLC_ARG_FREETEXT:
		if (arg->freetext != NULL) {
			if (readable) {
				cpdlc_strlcpy(textbuf, arg->freetext,
				    sizeof (textbuf));
			} else {
				cpdlc_escape_percent(arg->freetext, textbuf,
				    sizeof (textbuf));
			}
		} else {
			textbuf[0] = '\0';
		}
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s%s",
		    readable ? "" : " ", textbuf);
		break;
	case CPDLC_ARG_PERSONS:
		if (arg->pob != 0) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s%d",
			    readable ? "" : " ", arg->pob);
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s-",
			    readable ? "" : " ");
		}
		break;
	case CPDLC_ARG_POSREPORT: {
		char repbuf[1024] = {};
		serialize_posreport(&arg->pos_rep, readable, repbuf,
		    sizeof (repbuf));
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s",
			    repbuf);
		} else {
			cpdlc_escape_percent(repbuf, textbuf, sizeof (textbuf));
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s",
			    textbuf);
		}
		break;
	}
	}
}

static void
encode_seg(const cpdlc_msg_seg_t *seg, unsigned *n_bytes_p, char **buf_p,
    unsigned *cap_p)
{
	const cpdlc_msg_info_t *info = seg->info;

	APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "/MSG=%s%d",
	    info->is_dl ? "DM" : "UM", info->msg_type);
	if (info->msg_subtype != 0) {
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%c",
		    info->msg_subtype);
	}

	for (unsigned i = 0; i < info->num_args; i++) {
		cpdlc_encode_msg_arg(info->args[i], &seg->args[i], false,
		    n_bytes_p, buf_p, cap_p);
	}
}

cpdlc_msg_t *
cpdlc_msg_alloc(cpdlc_pkt_t pkt_type)
{
	cpdlc_msg_t *msg = safe_calloc(1, sizeof (cpdlc_msg_t));

	CPDLC_ASSERT3U(pkt_type, <=, CPDLC_PKT_PONG);
	msg->mrn = CPDLC_INVALID_MSG_SEQ_NR;
	msg->pkt_type = pkt_type;

	return (msg);
}

static cpdlc_route_t *
duplicate_route(const cpdlc_route_t *route_in)
{
	cpdlc_route_t *route_out = safe_malloc(sizeof (*route_out));
	memcpy(route_out, route_in, sizeof (*route_out));
	return (route_out);
}

cpdlc_msg_t *
cpdlc_msg_copy(const cpdlc_msg_t *oldmsg)
{
	cpdlc_msg_t *newmsg = safe_calloc(1, sizeof (cpdlc_msg_t));

	memcpy(newmsg, oldmsg, sizeof (*newmsg));
	if (oldmsg->logon_data != NULL)
		newmsg->logon_data = strdup(oldmsg->logon_data);

	for (unsigned i = 0; i < oldmsg->num_segs; i++) {
		const cpdlc_msg_seg_t *oldseg = &oldmsg->segs[i];
		cpdlc_msg_seg_t *newseg = &newmsg->segs[i];

		if (oldseg->info == NULL)
			continue;
		for (unsigned j = 0; j < oldseg->info->num_args; j++) {
			if (oldseg->info->args[j] == CPDLC_ARG_ROUTE &&
			    oldseg->args[j].route != NULL) {
				newseg->args[j].route = duplicate_route(
				    oldseg->args[j].route);
			} else if (oldseg->info->args[j] ==
			    CPDLC_ARG_FREETEXT &&
			    oldseg->args[j].freetext != NULL) {
				newseg->args[j].freetext = strdup(
				    oldseg->args[j].freetext);
			}
		}
	}

	return (newmsg);
}

void
cpdlc_msg_free(cpdlc_msg_t *msg)
{
	CPDLC_ASSERT(msg != NULL);

	free(msg->logon_data);

	for (unsigned i = 0; i < msg->num_segs; i++) {
		cpdlc_msg_seg_t *seg = &msg->segs[i];

		if (seg->info == NULL)
			continue;
		for (unsigned j = 0; j < seg->info->num_args; j++) {
			if (seg->info->args[j] == CPDLC_ARG_ROUTE)
				free(seg->args[j].route);
			else if (seg->info->args[j] == CPDLC_ARG_FREETEXT)
				free(seg->args[j].freetext);
		}
	}
	free(msg);
}

static const char *
pkt_type2str(cpdlc_pkt_t pkt_type)
{
	switch (pkt_type) {
	case CPDLC_PKT_CPDLC:
		return ("CPDLC");
	case CPDLC_PKT_PING:
		return ("PING");
	case CPDLC_PKT_PONG:
		return ("PONG");
	default:
		CPDLC_VERIFY_MSG(0, "Invalid pkt_type %x", pkt_type);
	}
}

unsigned
cpdlc_msg_encode(const cpdlc_msg_t *msg, char *buf, unsigned cap)
{
	unsigned n_bytes = 0;

	APPEND_SNPRINTF(n_bytes, buf, cap, "PKT=%s/MIN=%d",
	    pkt_type2str(msg->pkt_type), msg->min);

	if (msg->mrn != CPDLC_INVALID_MSG_SEQ_NR)
		APPEND_SNPRINTF(n_bytes, buf, cap, "/MRN=%d", msg->mrn);
	if (msg->is_logon) {
		char textbuf[64] = {};
		CPDLC_ASSERT(msg->logon_data != NULL);
		cpdlc_escape_percent(msg->logon_data, textbuf,
		    sizeof (textbuf));
		APPEND_SNPRINTF(n_bytes, buf, cap, "/LOGON=%s", textbuf);
	}
	if (msg->is_logoff)
		APPEND_SNPRINTF(n_bytes, buf, cap, "/LOGOFF");
	if (msg->from[0] != '\0') {
		char textbuf[32] = {};
		cpdlc_escape_percent(msg->from, textbuf, sizeof (textbuf));
		APPEND_SNPRINTF(n_bytes, buf, cap, "/FROM=%s", textbuf);
	}
	if (msg->to[0] != '\0') {
		char textbuf[32] = {};
		cpdlc_escape_percent(msg->to, textbuf, sizeof (textbuf));
		APPEND_SNPRINTF(n_bytes, buf, cap, "/TO=%s", textbuf);
	}

	for (unsigned i = 0; i < msg->num_segs; i++)
		encode_seg(&msg->segs[i], &n_bytes, &buf, &cap);
	APPEND_SNPRINTF(n_bytes, buf, cap, "\n");

	return (n_bytes);
}

static void
readable_seg(const cpdlc_msg_seg_t *seg, unsigned *n_bytes_p, char **buf_p,
    unsigned *cap_p)
{
	const cpdlc_msg_info_t *info = seg->info;
	const char *start = info->text;
	const char *end = start + strlen(info->text);
	char textbuf[512] = {};

	for (unsigned arg = 0; start < end; arg++) {
		const char *open = strchr(start, '[');
		const char *close = strchr(start, ']');
		if (open == NULL) {
			open = end;
			close = end;
		}
		cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
		    (uintptr_t)(open - start) + 1));
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s", textbuf);

		if (close == end) {
			CPDLC_ASSERT3U(arg, ==, info->num_args);
			break;
		}

		cpdlc_encode_msg_arg(info->args[arg], &seg->args[arg], true,
		    n_bytes_p, buf_p, cap_p);
		/*
		 * If the argument was at the end of the text line, `start'
		 * can now be pointing past `end'. So we must NOT deref it
		 * before it gets re-validated at the top of this loop.
		 */
		start = close + 1;
		if (info->args[arg] == CPDLC_ARG_DIRECTION &&
		    seg->args[arg].dir == CPDLC_DIR_EITHER) {
			/*
			 * Skip an extra space when the turn direction is
			 * `ANY'
			 */
			start++;
		}
	}
}

unsigned
cpdlc_msg_readable(const cpdlc_msg_t *msg, char *buf, unsigned cap)
{
	unsigned n_bytes = 0;

	for (unsigned i = 0; i < msg->num_segs; i++) {
		readable_seg(&msg->segs[i], &n_bytes, &buf, &cap);
		if (i + 1 < msg->num_segs)
			APPEND_SNPRINTF(n_bytes, buf, cap, ", ");
	}

	return (n_bytes);
}

static bool
validate_logon_logoff_message(const cpdlc_msg_t *msg, char *reason,
    unsigned reason_cap)
{
	const char *msgtype;

	CPDLC_ASSERT(msg != NULL);

	msgtype = (msg->is_logon ? "LOGON" : "LOGOFF");
	if (msg->is_logon && msg->is_logoff) {
		MALFORMED_MSG("message can either be a LOGON or LOGOFF "
		    "message, but not both");
		return (false);
	}
	if (msg->num_segs != 0) {
		MALFORMED_MSG("%s messages may not contain MSG segments",
		    msgtype);
		return (false);
	}
	if (msg->from[0] == '\0') {
		MALFORMED_MSG("%s messages MUST contain a FROM header",
		    msgtype);
		return (false);
	}
	if (msg->min == CPDLC_INVALID_MSG_SEQ_NR) {
		MALFORMED_MSG("missing or invalid MIN header");
		return (false);
	}
	return (true);
}

static bool
validate_message(const cpdlc_msg_t *msg, char *reason, unsigned reason_cap)
{
	/* LOGON message format is special */
	if (msg->is_logon || msg->is_logoff)
		return (validate_logon_logoff_message(msg, reason, reason_cap));

	if (msg->min == CPDLC_INVALID_MSG_SEQ_NR) {
		MALFORMED_MSG("missing or invalid MIN header");
		return (false);
	}
	if (msg->pkt_type == CPDLC_PKT_PING ||
	    msg->pkt_type == CPDLC_PKT_PONG) {
		if (msg->num_segs != 0) {
			MALFORMED_MSG("PING/PONG messages may not contain "
			    "MSG segments");
			return (false);
		}
		if (msg->pkt_type == CPDLC_PKT_PING &&
		    msg->mrn != CPDLC_INVALID_MSG_SEQ_NR) {
			MALFORMED_MSG("PING messages may not contain "
			    "an MRN header");
			return (false);
		}
		return (true);
	}
	if (msg->num_segs == 0) {
		MALFORMED_MSG("no message segments found");
		return (false);
	}

	return (true);
}

static bool
is_hold(bool is_dl, int msg_type)
{
	return (!is_dl && msg_type ==
	    CPDLC_UM91_HOLD_AT_pos_MAINT_alt_INBD_deg_TURN_dir_LEG_TIME_time);
}

static bool
is_offset(bool is_dl, int msg_type)
{
	return ((is_dl &&
	    (msg_type == CPDLC_DM15_REQ_OFFSET_dir_dist_OF_ROUTE ||
	    msg_type == CPDLC_DM16_AT_pos_REQ_OFFSET_dir_dist_OF_ROUTE ||
	    msg_type == CPDLC_DM17_AT_time_REQ_OFFSET_dir_dist_OF_ROUTE)) ||
	    (!is_dl &&
	    (msg_type == CPDLC_UM64_OFFSET_dir_dist_OF_ROUTE ||
	    msg_type == CPDLC_UM65_AT_pos_OFFSET_dir_dist_OF_ROUTE ||
	    msg_type == CPDLC_UM66_AT_time_OFFSET_dir_dist_OF_ROUTE ||
	    msg_type == CPDLC_UM152_WHEN_CAN_YOU_ACPT_dir_dist_OFFSET)));
}

static bool
is_valid_squawk(unsigned squawk)
{
	char buf[8];
	int l;

	l = snprintf(buf, sizeof (buf), "%04d", squawk);
	return (l == 4 && buf[0] >= '0' && buf[0] <= '7' &&
	    buf[1] >= '0' && buf[1] <= '7' &&
	    buf[2] >= '0' && buf[2] <= '7' &&
	    buf[3] >= '0' && buf[3] <= '7');
}

static const char *
find_arg_end(const char *start, const char *end)
{
	while (start < end && !isspace(start[0]))
		start++;
	return (start);
}

static bool
contains_spaces(const char *str)
{
	for (int i = 0, n = strlen(str); i < n; i++) {
		if (isspace(str[i]))
			return (true);
	}
	return (false);
}

static const char *
deserialize_pb(const char *s, cpdlc_pb_t *pb)
{
	const char *slash, *bra, *end;
	CPDLC_ASSERT(s != NULL);
	CPDLC_ASSERT(pb != NULL);

	slash = strchr(s, '/');
	if (slash == NULL)
		return (NULL);
	cpdlc_strlcpy(pb->fixname, s, MIN(sizeof (pb->fixname),
	    (unsigned)(slash - s) + 1));
	if (sscanf(&slash[1], "%d", &pb->degrees) != 1 ||
	    !is_valid_crs(pb->degrees)) {
		return (NULL);
	}
	bra = strchr(&slash[1], '(');
	if (bra != NULL) {
		if (sscanf(&bra[1], "%lf,%lf", &pb->lat_lon.lat,
		    &pb->lat_lon.lon) != 2 || !is_valid_lat_lon(pb->lat_lon)) {
			return (NULL);
		}
	} else {
		pb->lat_lon = CPDLC_NULL_LAT_LON;
	}
	end = strchr(&slash[1], '/');
	if (end == NULL)
		end = s + strlen(s);

	return (end);
}

static bool
deserialize_pbd(const char *s, cpdlc_pbd_t *pbd)
{
	const char *slash, *bra;

	CPDLC_ASSERT(s != NULL);
	CPDLC_ASSERT(pbd != NULL);

	slash = strchr(s, '/');
	if (slash == NULL)
		return (NULL);
	cpdlc_strlcpy(pbd->fixname, s, MIN(sizeof (pbd->fixname),
	    (unsigned)(slash - s) + 1));
	if (sscanf(&slash[1], "%d", &pbd->degrees) != 1 ||
	    !is_valid_crs(pbd->degrees)) {
		return (NULL);
	}
	slash = strchr(&slash[1], '/');
	if (slash == NULL)
		return (NULL);
	if (sscanf(&slash[1], "%f", &pbd->dist_nm) != 1 ||
	    pbd->dist_nm < 0.1 || pbd->dist_nm > 99.9) {
		return (NULL);
	}
	bra = strchr(&slash[1], '(');
	if (bra != NULL) {
		if (sscanf(&bra[1], "%lf,%lf", &pbd->lat_lon.lat,
		    &pbd->lat_lon.lon) != 2 ||
		    !is_valid_lat_lon(pbd->lat_lon)) {
			return (NULL);
		}
	} else {
		pbd->lat_lon = CPDLC_NULL_LAT_LON;
	}
	return (true);
}

static bool
deserialize_trk_detail(const char *s, cpdlc_trk_detail_t *trk)
{
	const char *bra;

	CPDLC_ASSERT(s != NULL);
	CPDLC_ASSERT(trk != NULL);

	bra = strchr(s, '(');
	if (bra == NULL)
		return (false);
	cpdlc_strlcpy(trk->name, s, MIN(sizeof (trk->name),
	    (unsigned)(bra - s) + 1));
	s = bra + 1;
	while (trk->num_lat_lon < CPDLC_TRK_DETAIL_MAX_LAT_LON &&
	    *s != '\0' && *s != ')') {
		if (sscanf(s, "%lf,%lf", &trk->lat_lon[trk->num_lat_lon].lat,
		    &trk->lat_lon[trk->num_lat_lon].lon) != 2 ||
		    !is_valid_lat_lon(trk->lat_lon[trk->num_lat_lon])) {
			return (false);
		}
		trk->num_lat_lon++;
		s = strchr(s, ',');
		if (s == NULL)
			return (false);
		s++;
		s = strchr(s, ',');
		if (s == NULL)
			break;
		s++;
	}
	return (true);
}

static bool
parse_route_info(cpdlc_route_t *route, const char *comp,
    char *reason, unsigned reason_cap)
{
	CPDLC_ASSERT(route != NULL);
	CPDLC_ASSERT(comp != NULL);
	CPDLC_ASSERT(reason != NULL);

	if (strchr(comp, ':') == NULL) {
		cpdlc_route_info_t *info;

		if (route->num_info == CPDLC_ROUTE_MAX_INFO) {
			MALFORMED_MSG("Malformed route: too many route infos");
			return (false);
		}
		info = &route->info[route->num_info];
		info->type = CPDLC_ROUTE_UNKNOWN;
		cpdlc_strlcpy(info->str, comp, sizeof (info->str));
		route->num_info++;
		return (true);
	}
	if (strncmp(comp, "ADEP:", 5) == 0) {
		cpdlc_strlcpy(route->orig_icao, &comp[5],
		    sizeof (route->orig_icao));
	} else if (strncmp(comp, "ADES:", 5) == 0) {
		cpdlc_strlcpy(route->dest_icao, &comp[5],
		    sizeof (route->dest_icao));
	} else if (strncmp(comp, "DEPRWY:", 7) == 0) {
		cpdlc_strlcpy(route->orig_rwy, &comp[7],
		    sizeof (route->orig_rwy));
	} else if (strncmp(comp, "SID:", 4) == 0) {
		cpdlc_strlcpy(route->sid.name, &comp[4],
		    sizeof (route->sid.name));
	} else if (strncmp(comp, "SIDTR:", 6) == 0) {
		cpdlc_strlcpy(route->sid.trans, &comp[6],
		    sizeof (route->sid.trans));
	} else if (strncmp(comp, "STAR:", 5) == 0) {
		cpdlc_strlcpy(route->star.name, &comp[5],
		    sizeof (route->star.name));
	} else if (strncmp(comp, "STARTR:", 7) == 0) {
		cpdlc_strlcpy(route->star.trans, &comp[7],
		    sizeof (route->star.trans));
	} else if (strncmp(comp, "APP:", 4) == 0) {
		cpdlc_strlcpy(route->appch.name, &comp[4],
		    sizeof (route->appch.name));
	} else if (strncmp(comp, "APPTR:", 6) == 0) {
		cpdlc_strlcpy(route->appch.trans, &comp[6],
		    sizeof (route->appch.trans));
	} else if (strncmp(comp, "FIX:", 4) == 0) {
		const char *bra = strchr(&comp[4], '(');
		cpdlc_route_info_t *info;

		if (route->num_info == CPDLC_ROUTE_MAX_INFO) {
			MALFORMED_MSG("Malformed route: too many route infos");
			return (false);
		}
		info = &route->info[route->num_info++];
		info->type = CPDLC_ROUTE_PUB_IDENT;
		if (bra != NULL) {
			cpdlc_strlcpy(info->pub_ident.fixname, &comp[4],
			    MIN(sizeof (info->pub_ident.fixname),
			    (unsigned)(bra - &comp[4]) + 1));
			if (sscanf(&bra[1], "%lf,%lf",
			    &info->pub_ident.lat_lon.lat,
			    &info->pub_ident.lat_lon.lon) != 2 ||
			    !is_valid_lat_lon(info->pub_ident.lat_lon)) {
				MALFORMED_MSG("Malformed or invalid lat-lon "
				    "coordinates in route component \"%s\"",
				    comp);
				return (false);
			}
		} else {
			cpdlc_strlcpy(info->pub_ident.fixname, &comp[4],
			    sizeof (info->pub_ident.fixname));
			info->pub_ident.lat_lon = CPDLC_NULL_LAT_LON;
		}
	} else if (strncmp(comp, "LATLON:", 7) == 0) {
		cpdlc_route_info_t *info;

		if (route->num_info == CPDLC_ROUTE_MAX_INFO) {
			MALFORMED_MSG("Malformed route: too many route infos");
			return (false);
		}
		info = &route->info[route->num_info++];
		info->type = CPDLC_ROUTE_LAT_LON;
		if (sscanf(&comp[7], "%lf,%lf", &info->lat_lon.lat,
		    &info->lat_lon.lon) != 2 ||
		    !is_valid_lat_lon(info->lat_lon)) {
			MALFORMED_MSG("Malformed or invalid lat-lon "
			    "coordinates in route component \"%s\"", comp);
			return (false);
		}
	} else if (strncmp(comp, "PBPB:", 5) == 0) {
		const char *s = &comp[5];
		cpdlc_route_info_t *info;

		if (route->num_info == CPDLC_ROUTE_MAX_INFO) {
			MALFORMED_MSG("Malformed route: too many route infos");
			return (false);
		}
		info = &route->info[route->num_info++];
		info->type = CPDLC_ROUTE_PBPB;
		for (int i = 0; i < 2; i++) {
			s = deserialize_pb(s, &info->pbpb[i]);
			if (comp == NULL) {
				MALFORMED_MSG("Error deserializing "
				    "place-bearing/place-bearing \"%s\"", comp);
				return (false);
			}
			if (s[0] == '/')
				s++;
		}
	} else if (strncmp(comp, "PBD:", 4) == 0) {
		cpdlc_route_info_t *info;

		if (route->num_info == CPDLC_ROUTE_MAX_INFO) {
			MALFORMED_MSG("Malformed route: too many route infos");
			return (false);
		}
		info = &route->info[route->num_info++];
		info->type = CPDLC_ROUTE_PBD;
		if (!deserialize_pbd(&comp[4], &info->pbd)) {
			MALFORMED_MSG("Error deserializing "
			    "place-bearing-distance \"%s\"", comp);
			return (false);
		}
	} else if (strncmp(comp, "AWY:", 4) == 0) {
		cpdlc_route_info_t *info;

		if (route->num_info == CPDLC_ROUTE_MAX_INFO) {
			MALFORMED_MSG("Malformed route: too many route infos");
			return (false);
		}
		info = &route->info[route->num_info++];
		info->type = CPDLC_ROUTE_AWY;
		cpdlc_strlcpy(info->awy, &comp[4], sizeof (info->awy));
	} else if (strncmp(comp, "TRK:", 4) == 0) {
		cpdlc_route_info_t *info;

		if (route->num_info == CPDLC_ROUTE_MAX_INFO) {
			MALFORMED_MSG("Malformed route: too many route infos");
			return (false);
		}
		info = &route->info[route->num_info++];
		info->type = CPDLC_ROUTE_TRACK_DETAIL;
		if (!deserialize_trk_detail(&comp[4], &route->trk_detail)) {
			MALFORMED_MSG("Error deserializing trackdetail \"%s\"",
			    comp);
			return (false);
		}
	} else {
		MALFORMED_MSG("Route component \"%s\" has unknown type marker",
		    comp);
		return (false);
	}
	return (true);
}

static cpdlc_route_t *
parse_route(const char *buf, char *reason, unsigned reason_cap)
{
	unsigned n_comps;
	char **comps;
	cpdlc_route_t *route = safe_calloc(1, sizeof (*route));

	CPDLC_ASSERT(buf != NULL);
	CPDLC_ASSERT(reason != NULL);

	comps = cpdlc_strsplit(buf, " ", true, &n_comps);
	if (n_comps > CPDLC_ROUTE_MAX_INFO) {
		MALFORMED_MSG("Route contains too many components "
		    "(%d, max: %d)", n_comps, CPDLC_ROUTE_MAX_INFO);
		goto errout;
	}
	for (unsigned i = 0; i < n_comps; i++) {
		if (!parse_route_info(route, comps[i], reason, reason_cap))
			goto errout;
	}
	cpdlc_free_strlist(comps, n_comps);
	return (route);
errout:
	cpdlc_free_strlist(comps, n_comps);
	free(route);
	return (NULL);
}

static bool
parse_pos(cpdlc_pos_t *pos, const char *buf, char *reason, unsigned reason_cap)
{
	CPDLC_ASSERT(pos != NULL);
	CPDLC_ASSERT(buf != NULL);
	CPDLC_ASSERT(reason != NULL);

	if (strchr(buf, ':') == NULL) {
		pos->set = true;
		pos->type = CPDLC_POS_UNKNOWN;
		cpdlc_strlcpy(pos->str, buf, sizeof (pos->str));
		return (true);
	}
	if (strncmp(buf, "FIX:", 4) == 0) {
		pos->set = true;
		pos->type = CPDLC_POS_FIXNAME;
		cpdlc_strlcpy(pos->fixname, &buf[4], sizeof (pos->fixname));
	} else if (strncmp(buf, "NAV:", 4) == 0) {
		pos->set = true;
		pos->type = CPDLC_POS_NAVAID;
		cpdlc_strlcpy(pos->navaid, &buf[4], sizeof (pos->navaid));
	} else if (strncmp(buf, "ARPT:", 5) == 0) {
		pos->set = true;
		pos->type = CPDLC_POS_AIRPORT;
		cpdlc_strlcpy(pos->airport, &buf[5], sizeof (pos->airport));
	} else if (strncmp(buf, "LATLON:", 7) == 0) {
		pos->set = true;
		pos->type = CPDLC_POS_LAT_LON;
		if (sscanf(&buf[7], "%lf,%lf", &pos->lat_lon.lat,
		    &pos->lat_lon.lon) != 2 ||
		    !is_valid_lat_lon(pos->lat_lon)) {
			MALFORMED_MSG("Malformed or invalid lat-lon "
			    "coordinates in position \"%s\"", buf);
			return (false);
		}
	} else if (strncmp(buf, "PBD:", 4) == 0) {
		pos->set = true;
		pos->type = CPDLC_POS_PBD;
		if (!deserialize_pbd(&buf[4], &pos->pbd)) {
			MALFORMED_MSG("Error deserializing "
			    "place-bearing-distance \"%s\"", buf);
			return (false);
		}
	} else {
		MALFORMED_MSG("Position spec \"%s\" has unknown type", buf);
	}
	return (true);
}

static bool
parse_time(const char *buf, cpdlc_time_t *tim, char *reason,
    unsigned reason_cap)
{
	CPDLC_ASSERT(buf != NULL);
	CPDLC_ASSERT(tim != NULL);

	if (strncmp(buf, "NOW", 3) == 0) {
		tim->hrs = -1;
	} else {
		char hrs[3] = { buf[0], buf[1] }, mins[3] = { buf[2], buf[3] };

		if (sscanf(hrs, "%d", &tim->hrs) != 1 ||
		    sscanf(mins, "%d", &tim->mins) != 1 ||
		    tim->hrs < 0 || tim->hrs > 23 ||
		    tim->mins < 0 || tim->mins > 59) {
			MALFORMED_MSG("invalid time");
			return (false);
		}
	}
	return (true);
}

static bool
parse_alt(const char *start, const char *end, cpdlc_alt_t *alt,
    char *reason, unsigned reason_cap)
{
	const char *arg_end;

	CPDLC_ASSERT(start != NULL);
	CPDLC_ASSERT(end != NULL);
	CPDLC_ASSERT(alt != NULL);
	CPDLC_ASSERT(reason != NULL);

	if (start + 2 < end && start[0] == 'F' && start[1] == 'L') {
		alt->fl = true;
		if (sscanf(&start[2], "%d", &alt->alt) != 1 || alt->alt <= 0) {
			MALFORMED_MSG("invalid flight level");
			return (false);
		}
	} else {
		alt->alt = atoi(start);
		if (sscanf(start, "%d", &alt->alt) != 1 ||
		    alt->alt < -1500 || alt->alt > 100000) {
			MALFORMED_MSG("invalid altitude");
			return (false);
		}
	}
	arg_end = find_arg_end(start, end);
	if (*(arg_end - 1) == 'M')
		alt->met = true;
	/* Multiply non-metric FLs by 100 */
	if (alt->fl && !alt->met)
		alt->alt *= 100;
	/* Second round of validation for FLs */
	if (alt->fl && ((!alt->met && alt->alt > 60000) ||
	    (alt->met && alt->alt > 20000))) {
		MALFORMED_MSG("invalid flight level");
		return (false);
	}
	return (true);
}

static bool
parse_spd(const char *start, const char *end, cpdlc_spd_t *spd,
    char *reason, unsigned reason_cap)
{
	CPDLC_ASSERT(start != NULL);
	CPDLC_ASSERT(end != NULL);
	CPDLC_ASSERT(spd != NULL);
	CPDLC_ASSERT(reason != NULL);

	if (start + 1 < end && start[0] == 'M') {
		spd->mach = true;
		spd->spd = round(atof(&start[1]) * 1000);
		if (spd->spd < 100) {
			MALFORMED_MSG("invalid Mach");
			return (false);
		}
	} else {
		spd->spd = atoi(start);
	}
	return (true);
}

static bool
parse_posreport(const char *buf, cpdlc_pos_rep_t *rep, char *reason,
    unsigned reason_cap)
{
	unsigned n_comps;
	char **comps;

	CPDLC_ASSERT(buf != NULL);
	CPDLC_ASSERT(rep != NULL);
	CPDLC_ASSERT(reason != NULL);
	memset(rep, 0, sizeof (*rep));

	comps = cpdlc_strsplit(buf, " ", true, &n_comps);
	if (n_comps < 22) {
		MALFORMED_MSG("Malformed pos rep structure \"%s\" "
		    "has too few components (%d)", buf, n_comps);
		goto errout;
	}
	if (!parse_pos(&rep->cur_pos, comps[0], reason, reason_cap))
		goto errout;
	if (!parse_time(comps[1], &rep->time_cur_pos, reason, reason_cap))
		goto errout;
	if (!parse_alt(comps[2], comps[2] + strlen(comps[2]), &rep->cur_alt,
	    reason, reason_cap)) {
		goto errout;
	}
	if (strcmp(comps[3], "-") != 0 &&
	    !parse_pos(&rep->fix_next, comps[3], reason, reason_cap)) {
		goto errout;
	}
	if (strcmp(comps[4], "-") != 0) {
		if (!parse_time(comps[4], &rep->time_fix_next, reason,
		    reason_cap)) {
			goto errout;
		}
	} else {
		rep->time_fix_next = CPDLC_NULL_TIME;
	}
	if (strcmp(comps[5], "-") != 0 &&
	    !parse_pos(&rep->fix_next_p1, comps[5], reason, reason_cap)) {
		goto errout;
	}
	if (strcmp(comps[6], "-") != 0) {
		if (!parse_time(comps[6], &rep->time_dest, reason, reason_cap))
			goto errout;
	} else {
		rep->time_dest = CPDLC_NULL_TIME;
	}
	if (strcmp(comps[7], "-") != 0) {
		if (!parse_time(comps[7], &rep->rmng_fuel, reason, reason_cap))
			goto errout;
	} else {
		rep->rmng_fuel = CPDLC_NULL_TIME;
	}
	rep->temp = (strcmp(comps[8], "-") != 0 ? atoi(comps[8]) :
	    CPDLC_NULL_TEMP);
	if (strcmp(comps[9], "-") != 0) {
		if (sscanf(comps[9], "%d/%d", &rep->wind.dir,
		    &rep->wind.spd) != 2) {
			MALFORMED_MSG("Malformed wind data \"%s\"", comps[9]);
			goto errout;
		}
	}
	rep->turb = MIN(atoi(comps[10]), CPDLC_TURB_SEV);
	rep->icing = MIN(atoi(comps[11]), CPDLC_ICING_SEV);
	if (strcmp(comps[12], "-") != 0) {
		if (!parse_spd(comps[12], comps[12] + strlen(comps[12]),
		    &rep->spd, reason, reason_cap)) {
			goto errout;
		}
	} else {
		rep->spd = CPDLC_NULL_SPD;
	}
	if (strcmp(comps[13], "-") != 0) {
		if (!parse_spd(comps[13], comps[13] + strlen(comps[13]),
		    &rep->spd_gnd, reason, reason_cap)) {
			goto errout;
		}
	} else {
		rep->spd_gnd = CPDLC_NULL_SPD;
	}
	if (strcmp(comps[14], "-") != 0) {
		rep->vvi_set = true;
		rep->vvi = atoi(comps[14]);
	}
	if (strcmp(comps[15], "-") != 0)
		rep->trk = atoi(comps[15]);
	if (strcmp(comps[16], "-") != 0)
		rep->hdg_true = atoi(comps[16]);
	if (strcmp(comps[17], "-") != 0) {
		rep->dist_set =true;
		rep->dist_nm = atof(comps[17]);
	}
	if (cpdlc_unescape_percent(comps[18], rep->remarks,
	    sizeof (rep->remarks)) == -1) {
		MALFORMED_MSG("invalid URL escape in \"%s\"", comps[18]);
		goto errout;
	}
	if (strcmp(comps[19], "-") != 0 &&
	    !parse_pos(&rep->rpt_wpt_pos, comps[19], reason, reason_cap)) {
		goto errout;
	}
	if (strcmp(comps[20], "-") != 0) {
		if (!parse_time(comps[20], &rep->rpt_wpt_time,
		    reason, reason_cap)) {
			goto errout;
		}
	} else {
		rep->rpt_wpt_time = CPDLC_NULL_TIME;
	}
	if (strcmp(comps[21], "-") != 0) {
		if (!parse_alt(comps[21], comps[21] + strlen(comps[21]),
		    &rep->rpt_wpt_alt, reason, reason_cap)) {
			goto errout;
		}
	} else {
		rep->rpt_wpt_alt = CPDLC_NULL_ALT;
	}
	cpdlc_free_strlist(comps, n_comps);
	return (true);
errout:
	cpdlc_free_strlist(comps, n_comps);
	return (false);
}

static bool
msg_decode_seg(cpdlc_msg_seg_t *seg, const char *start, const char *end,
    char *reason, unsigned reason_cap)
{
	bool is_dl;
	int msg_type;
	char msg_subtype = 0;
	unsigned num_args = 0;
	const cpdlc_msg_info_t *info;
	char textbuf[8192] = {};

	CPDLC_ASSERT(seg != NULL);
	CPDLC_ASSERT(start != NULL);
	CPDLC_ASSERT(end != NULL);
	CPDLC_ASSERT(reason != NULL);

	if (strncmp(start, "DM", 2) == 0) {
		is_dl = true;
	} else if (strncmp(start, "UM", 2) == 0) {
		is_dl = false;
	} else {
		MALFORMED_MSG("invalid segment letters");
		return (false);
	}
	start += 2;
	if (start >= end) {
		MALFORMED_MSG("segment too short");
		return (false);
	}
	msg_type = atoi(start);
	if (msg_type < 0 ||
	    (is_dl && msg_type > CPDLC_DM80_DEVIATING_dir_dist_OF_ROUTE) ||
	    (!is_dl && msg_type >
	    CPDLC_UM208_FREETEXT_LOW_URG_LOW_ALERT_text)) {
		MALFORMED_MSG("invalid message type");
		return (false);
	}
	while (start < end && isdigit(start[0]))
		start++;
	if (start >= end) {
		seg->info = msg_infos_lookup(is_dl, msg_type, 0);
		if (seg->info == NULL) {
			MALFORMED_MSG("invalid message type");
			return (false);
		}
		goto end;
	}
	if (!isspace(start[0])) {
		if (!is_dl || msg_type != 67) {
			MALFORMED_MSG("only DM67 can have a subtype suffix");
			return (false);
		}
		msg_subtype = start[0];
		if (msg_subtype < CPDLC_DM67b_WE_CAN_ACPT_alt_AT_time ||
		    msg_subtype > CPDLC_DM67i_WHEN_CAN_WE_EXPCT_DES_TO_alt) {
			MALFORMED_MSG("invalid DM67 subtype (%c)", msg_subtype);
			return (false);
		}
		start++;
	}

	info = msg_infos_lookup(is_dl, msg_type, msg_subtype);
	if (info == NULL) {
		MALFORMED_MSG("invalid message type");
		return (false);
	}
	seg->info = info;

	if (!isspace(start[0])) {
		MALFORMED_MSG("expected space after message type");
		return (false);
	}
	SKIP_SPACE(start, end);

	for (num_args = 0; num_args < info->num_args && start < end;
	    num_args++) {
		cpdlc_arg_t *arg = &seg->args[num_args];
		int l;
		const char *arg_end;

		switch (info->args[num_args]) {
		case CPDLC_ARG_ALTITUDE:
			if (!parse_alt(start, end, &arg->alt, reason,
			    reason_cap)) {
				return (false);
			}
			break;
		case CPDLC_ARG_SPEED:
			if (!parse_spd(start, end, &arg->spd, reason,
			    reason_cap)) {
				return (false);
			}
			break;
		case CPDLC_ARG_TIME:
			if (!parse_time(start, &arg->time, reason, reason_cap))
				return (false);
			break;
		case CPDLC_ARG_TIME_DUR:
			arg_end = find_arg_end(start, end);
			if (sscanf(start, "%u", &arg->time.mins) != 1) {
				MALFORMED_MSG("invalid time");
				return (false);
			}
			arg->time.hrs = arg->time.mins / 60;
			arg->time.mins = arg->time.mins % 60;
			break;
		case CPDLC_ARG_POSITION: {
			char tmpbuf[128] = {};

			arg_end = find_arg_end(start, end);
			if ((unsigned)(arg_end - start) >= sizeof (tmpbuf)) {
				MALFORMED_MSG("position spec too long");
				return (false);
			}
			cpdlc_strlcpy(tmpbuf, start, MIN(sizeof (tmpbuf),
			    (uintptr_t)(arg_end - start) + 1));
			if (cpdlc_unescape_percent(tmpbuf, textbuf,
			    sizeof (textbuf)) == -1) {
				MALFORMED_MSG("invalid URL escape in \"%s\"",
				    tmpbuf);
				return (false);
			}
			if (!parse_pos(&arg->pos, textbuf, reason, reason_cap))
				return (false);
			break;
		}
		case CPDLC_ARG_DIRECTION:
			if (strncmp(start, "NE", 2) == 0) {
				if (is_offset(is_dl, msg_type)) {
					MALFORMED_MSG("this message type "
					    "cannot specify a direction "
					    "of 'NORTHEAST'");
					return (false);
				}
				arg->dir = CPDLC_DIR_NE;
			} else if (strncmp(start, "NW", 2) == 0) {
				if (is_offset(is_dl, msg_type)) {
					MALFORMED_MSG("this message type "
					    "cannot specify a direction "
					    "of 'NORTHWEST'");
					return (false);
				}
				arg->dir = CPDLC_DIR_NW;
			} else if (strncmp(start, "SE", 2) == 0) {
				if (is_offset(is_dl, msg_type)) {
					MALFORMED_MSG("this message type "
					    "cannot specify a direction "
					    "of 'SOUTHEAST'");
					return (false);
				}
				arg->dir = CPDLC_DIR_SE;
			} else if (strncmp(start, "SW", 2) == 0) {
				if (is_offset(is_dl, msg_type)) {
					MALFORMED_MSG("this message type "
					    "cannot specify a direction "
					    "of 'SOUTHWEST'");
					return (false);
				}
				arg->dir = CPDLC_DIR_SW;
			} else if (start[0] == '-') {
				if (is_hold(is_dl, msg_type) ||
				    is_offset(is_dl, msg_type)) {
					MALFORMED_MSG("this message type "
					    "cannot specify a direction "
					    "of 'ANY'");
					return (false);
				}
				arg->dir = CPDLC_DIR_EITHER;
			} else if (start[0] == 'L') {
				arg->dir = CPDLC_DIR_LEFT;
			} else if (start[0] == 'R') {
				arg->dir = CPDLC_DIR_RIGHT;
			} else if (start[0] == 'N') {
				if (is_offset(is_dl, msg_type)) {
					MALFORMED_MSG("this message type "
					    "cannot specify a direction "
					    "of 'NORTH'");
					return (false);
				}
				arg->dir = CPDLC_DIR_NORTH;
			} else if (start[0] == 'S') {
				if (is_offset(is_dl, msg_type)) {
					MALFORMED_MSG("this message type "
					    "cannot specify a direction "
					    "of 'NORTH'");
					return (false);
				}
				arg->dir = CPDLC_DIR_SOUTH;
			} else if (start[0] == 'E') {
				if (is_offset(is_dl, msg_type)) {
					MALFORMED_MSG("this message type "
					    "cannot specify a direction "
					    "of 'NORTH'");
					return (false);
				}
				arg->dir = CPDLC_DIR_EAST;
			} else if (start[0] == 'W') {
				if (is_offset(is_dl, msg_type)) {
					MALFORMED_MSG("this message type "
					    "cannot specify a direction "
					    "of 'NORTH'");
					return (false);
				}
				arg->dir = CPDLC_DIR_WEST;
			} else {
				MALFORMED_MSG("invalid turn direction");
				return (false);
			}
			break;
		case CPDLC_ARG_DISTANCE:
			arg->dist = atof(start);
			if (arg->dist < 0 || arg->dist > 20000) {
				MALFORMED_MSG("invalid distance (%.2f)",
				    arg->dist);
				return (false);
			}
			break;
		case CPDLC_ARG_VVI:
			arg->vvi = atoi(start);
			if (arg->vvi < 0 || arg->vvi > 10000) {
				MALFORMED_MSG("invalid VVI (%d)", arg->vvi);
				return (false);
			}
			break;
		case CPDLC_ARG_TOFROM:
			arg_end = find_arg_end(start, end);
			if (strncmp(start, "TO", 2) == 0 &&
			    arg_end - start == 2) {
				arg->tofrom = true;
			} else if (strncmp(start, "FROM", 4) == 0 &&
			    arg_end - start == 4) {
				arg->tofrom = false;
			} else {
				MALFORMED_MSG("invalid TO/FROM flag");
				return (false);
			}
			break;
		case CPDLC_ARG_ROUTE: {
			char *buf;

			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(end - start) + 1));
			l = cpdlc_unescape_percent(textbuf, NULL, 0);
			if (l == -1) {
				MALFORMED_MSG("invalid URL escape");
				return (false);
			}
			if (arg->route != NULL)
				free(arg->route);
			buf = safe_malloc(l + 1);
			cpdlc_unescape_percent(textbuf, buf, l + 1);
			arg->route = parse_route(buf, reason, reason_cap);
			free(buf);
			if (arg->route == NULL)
				return (false);
			start = end;
			break;
		}
		case CPDLC_ARG_PROCEDURE:
			arg_end = find_arg_end(start, end);
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(arg_end - start) + 1));
			if (cpdlc_unescape_percent(textbuf, arg->proc,
			    sizeof (arg->proc)) == -1) {
				MALFORMED_MSG("invalid URL escape");
				return (false);
			}
			break;
		case CPDLC_ARG_SQUAWK:
			if (sscanf(start, "%d", &arg->squawk) != 1 ||
			    !is_valid_squawk(arg->squawk)) {
				MALFORMED_MSG("invalid squawk code");
				return (false);
			}
			break;
		case CPDLC_ARG_ICAONAME:
			/* The ICAO identifier goes first */
			arg_end = find_arg_end(start, end);
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(arg_end - start) + 1));
			if (cpdlc_unescape_percent(textbuf, arg->icaoname.icao,
			    sizeof (arg->icaoname.icao)) == -1) {
				MALFORMED_MSG("invalid URL escape");
				return (false);
			}
			if (contains_spaces(arg->icaoname.icao)) {
				MALFORMED_MSG("icaoname cannot contain "
				    "whitespace");
				return (false);
			}
			/* Followed by whitespace */
			SKIP_NONSPACE(start, end);
			SKIP_SPACE(start, end);
			if (start == end) {
				MALFORMED_MSG("icaoname must be followed by "
				    "descriptive name");
				return (false);
			}
			/* And finally a percent-escaped descriptive name */
			arg_end = find_arg_end(start, end);
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(arg_end - start) + 1));
			if (cpdlc_unescape_percent(textbuf, arg->icaoname.name,
			    sizeof (arg->icaoname.name)) == -1) {
				MALFORMED_MSG("invalid percent escapes");
				return (false);
			}
			break;
		case CPDLC_ARG_FREQUENCY:
			arg->freq = atof(start);
			if (arg->freq <= 0) {
				MALFORMED_MSG("invalid frequency");
				return (false);
			}
			break;
		case CPDLC_ARG_DEGREES:
			arg_end = find_arg_end(start, end);
			arg->deg.deg = atoi(start);
			if (arg->deg.deg > 360) {
				MALFORMED_MSG("invalid heading/track");
				return (false);
			}
			arg->deg.tru = (arg_end[-1] == 'T');
			break;
		case CPDLC_ARG_BARO: {
			double val;

			if (start + 3 > end) {
				MALFORMED_MSG("baro too short");
				return (false);
			}
			switch(start[0]) {
			case 'A':
				arg->baro.val = atof(&start[1]);
				arg->baro.hpa = false;
				if (arg->baro.val < 28 || arg->baro.val > 32) {
					MALFORMED_MSG("invalid baro value");
					return (false);
				}
				break;
			case 'Q':
				arg->baro.val = atoi(&start[1]);
				arg->baro.hpa = true;
				if (arg->baro.val < 900 ||
				    arg->baro.val > 1100) {
					MALFORMED_MSG("invalid baro value");
					return (false);
				}
				break;
			default:
				val = atof(start);
				if (val >= 2800 && val <= 3200) {
					arg->baro.val = val / 100;
					arg->baro.hpa = false;
				} else if (val >= 28 && val <= 32) {
					arg->baro.val = val;
					arg->baro.hpa = false;
				} else if (val >= 900 && val <= 1100) {
					arg->baro.val = val;
					arg->baro.hpa = true;
				} else {
					MALFORMED_MSG("invalid baro value (%s)",
					    start);
					return (false);
				}
				break;
			}
			break;
		}
		case CPDLC_ARG_FREETEXT:
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(end - start) + 1));
			l = cpdlc_unescape_percent(textbuf, NULL, 0);
			if (l == -1) {
				MALFORMED_MSG("invalid URL escape");
				return (false);
			}
			free(arg->freetext);
			arg->freetext = malloc(l + 1);
			cpdlc_unescape_percent(textbuf, arg->freetext, l + 1);
			start = end;
			break;
		case CPDLC_ARG_PERSONS:
			arg->pob = atoi(start);
			if (arg->pob > 1024) {
				MALFORMED_MSG("invalid number of persons on "
				    "board");
				return (false);
			}
			break;
		case CPDLC_ARG_POSREPORT: {
			char tmpbuf[1024] = {};

			arg_end = find_arg_end(start, end);
			cpdlc_strlcpy(tmpbuf, start, MIN(sizeof (tmpbuf),
			    (unsigned)(arg_end - start) + 1));
			cpdlc_unescape_percent(tmpbuf, textbuf,
			    sizeof (textbuf));
			if (!parse_posreport(textbuf, &arg->pos_rep,
			    reason, reason_cap)) {
				return (false);
			}
			break;
		}
		}

		SKIP_NONSPACE(start, end);
		SKIP_SPACE(start, end);
	}

end:
	if (seg->info->num_args != num_args) {
		MALFORMED_MSG("invalid number of arguments");
		return (false);
	}
	if (start != end) {
		MALFORMED_MSG("too much data in message");
		return (false);
	}

	return (true);
}

bool
cpdlc_msg_decode(const char *in_buf, cpdlc_msg_t **msg_p, int *consumed,
    char *reason, unsigned reason_cap)
{
	const char *start, *term;
	cpdlc_msg_t *msg;
	bool pkt_type_seen = false;
	bool skipped_cr = false;

	CPDLC_ASSERT(in_buf != NULL);
	CPDLC_ASSERT(msg_p != NULL);

	term = strchr(in_buf, '\n');
	if (term != NULL) {
		if (term > in_buf && *(term - 1) == '\r') {
			term -= 1;
			skipped_cr = true;
		}
	} else {
		term = strchr(in_buf, '\r');
	}
	if (term == NULL) {
		/* No complete message in buffer */
		*msg_p = NULL;
		if (consumed != NULL)
			*consumed = 0;
		return (true);
	}

	msg = safe_calloc(1, sizeof (*msg));
	msg->min = CPDLC_INVALID_MSG_SEQ_NR;
	msg->mrn = CPDLC_INVALID_MSG_SEQ_NR;

	start = in_buf;
	while (in_buf < term) {
		const char *sep = strchr(in_buf, '/');

		if (sep == NULL || sep > term)
			sep = term;
		if (strncmp(in_buf, "PKT=", 4) == 0) {
			if (strncmp(&in_buf[4], "CPDLC/", 6) == 0) {
				msg->pkt_type = CPDLC_PKT_CPDLC;
			} else if (strncmp(&in_buf[4], "PING/", 5) == 0) {
				msg->pkt_type = CPDLC_PKT_PING;
			} else if (strncmp(&in_buf[4], "PONG/", 5) == 0) {
				msg->pkt_type = CPDLC_PKT_PONG;
			} else {
				MALFORMED_MSG("invalid PKT type");
				goto errout;
			}
			pkt_type_seen = true;
		} else if (strncmp(in_buf, "MIN=", 4) == 0) {
			if (sscanf(&in_buf[4], "%u", &msg->min) != 1) {
				MALFORMED_MSG("invalid MIN value");
				goto errout;
			}
		} else if (strncmp(in_buf, "MRN=", 4) == 0) {
			if (sscanf(&in_buf[4], "%u", &msg->mrn) != 1) {
				MALFORMED_MSG("invalid MRN value");
				goto errout;
			}
		} else if (strncmp(in_buf, "LOGON=", 6) == 0) {
			int l = (sep - &in_buf[6]);
			char textbuf[l + 1];

			free(msg->logon_data);
			cpdlc_strlcpy(textbuf, &in_buf[6], l + 1);
			msg->logon_data = safe_malloc(l + 1);
			cpdlc_unescape_percent(textbuf, msg->logon_data, l + 1);
			msg->is_logon = true;
		} else if (strncmp(in_buf, "LOGOFF", 6) == 0) {
			msg->is_logoff = true;
		} else if (strncmp(in_buf, "TO=", 3) == 0) {
			char textbuf[32];

			cpdlc_strlcpy(textbuf, &in_buf[3], MIN(sizeof (textbuf),
			    (uintptr_t)(sep - &in_buf[3]) + 1));
			cpdlc_unescape_percent(textbuf, msg->to,
			    sizeof (msg->to));
		} else if (strncmp(in_buf, "FROM=", 5) == 0) {
			char textbuf[32];

			cpdlc_strlcpy(textbuf, &in_buf[5], MIN(sizeof (textbuf),
			    (uintptr_t)(sep - &in_buf[5]) + 1));
			cpdlc_unescape_percent(textbuf, msg->from,
			    sizeof (msg->from));
		} else if (strncmp(in_buf, "MSG=", 4) == 0) {
			cpdlc_msg_seg_t *seg;

			if (msg->num_segs == CPDLC_MAX_MSG_SEGS) {
				MALFORMED_MSG("too many message segments");
				goto errout;
			}
			seg = &msg->segs[msg->num_segs];
			if (!msg_decode_seg(seg, &in_buf[4], sep, reason,
			    reason_cap))
				goto errout;
			if (msg->num_segs > 0 && msg->segs[0].info->is_dl !=
			    seg->info->is_dl) {
				MALFORMED_MSG("can't mix DM and UM message "
				    "segments");
				goto errout;
			}
			msg->num_segs++;
		} else {
			MALFORMED_MSG("unknown message header");
			goto errout;
		}

		in_buf = sep + 1;
	}

	if (!pkt_type_seen) {
		MALFORMED_MSG("missing PKT header");
		goto errout;
	}
	if (!validate_message(msg, reason, reason_cap))
		goto errout;

	*msg_p = msg;
	if (consumed != NULL)
		*consumed = ((term - start) + 1 + (skipped_cr ? 1 : 0));
	return (true);
errout:
	free(msg);
	*msg_p = NULL;
	if (consumed != NULL)
		*consumed = 0;
	return (false);
}

void
cpdlc_msg_set_to(cpdlc_msg_t *msg, const char *to)
{
	memset(msg->to, 0, sizeof (msg->to));
	cpdlc_strlcpy(msg->to, to, sizeof (msg->to));
}

const char *
cpdlc_msg_get_to(const cpdlc_msg_t *msg)
{
	return (msg->to);
}

void
cpdlc_msg_set_from(cpdlc_msg_t *msg, const char *from)
{
	memset(msg->from, 0, sizeof (msg->from));
	cpdlc_strlcpy(msg->from, from, sizeof (msg->from));
}

const char *
cpdlc_msg_get_from(const cpdlc_msg_t *msg)
{
	return (msg->from);
}

bool
cpdlc_msg_get_dl(const cpdlc_msg_t *msg)
{
	if (msg->num_segs == 0)
		return (false);
	CPDLC_ASSERT(msg->segs[0].info != NULL);
	return (msg->segs[0].info->is_dl);
}

void
cpdlc_msg_set_min(cpdlc_msg_t *msg, unsigned min)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(min != CPDLC_INVALID_MSG_SEQ_NR);
	msg->min = min;
}

unsigned
cpdlc_msg_get_min(const cpdlc_msg_t *msg)
{
	CPDLC_ASSERT(msg != NULL);
	return (msg->min);
}

void
cpdlc_msg_set_mrn(cpdlc_msg_t *msg, unsigned mrn)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(mrn != CPDLC_INVALID_MSG_SEQ_NR);
	msg->mrn = mrn;
}

unsigned
cpdlc_msg_get_mrn(const cpdlc_msg_t *msg)
{
	CPDLC_ASSERT(msg != NULL);
	return (msg->mrn);
}

const char *
cpdlc_msg_get_logon_data(const cpdlc_msg_t *msg)
{
	CPDLC_ASSERT(msg != NULL);
	return (msg->logon_data);
}

void
cpdlc_msg_set_logon_data(cpdlc_msg_t *msg, const char *logon_data)
{
	CPDLC_ASSERT(msg != NULL);
	free(msg->logon_data);
	if (logon_data != NULL) {
		msg->logon_data = strdup(logon_data);
		msg->is_logon = true;
	} else {
		msg->logon_data = NULL;
		msg->is_logon = false;
	}
}

void
cpdlc_msg_set_logoff(cpdlc_msg_t *msg, bool is_logoff)
{
	CPDLC_ASSERT(msg != NULL);
	msg->is_logoff = is_logoff;
}

bool
cpdlc_msg_get_logoff(const cpdlc_msg_t *msg)
{
	CPDLC_ASSERT(msg != NULL);
	return (msg->is_logoff);
}

unsigned
cpdlc_msg_get_num_segs(const cpdlc_msg_t *msg)
{
	CPDLC_ASSERT(msg != NULL);
	return (msg->num_segs);
}

int
cpdlc_msg_add_seg(cpdlc_msg_t *msg, bool is_dl, unsigned msg_type,
    unsigned char msg_subtype)
{
	cpdlc_msg_seg_t *seg;

	CPDLC_ASSERT(msg != NULL);
	if (!is_dl) {
		CPDLC_ASSERT3U(msg_type, <=,
		    CPDLC_UM208_FREETEXT_LOW_URG_LOW_ALERT_text);
		CPDLC_ASSERT0(msg_subtype);
	} else {
		CPDLC_ASSERT3U(msg_type, <=,
		    CPDLC_DM80_DEVIATING_dir_dist_OF_ROUTE);
		if (msg_subtype != 0) {
			CPDLC_ASSERT3U(msg_subtype, >=,
			    CPDLC_DM67b_WE_CAN_ACPT_alt_AT_time);
			CPDLC_ASSERT3U(msg_subtype, <=,
			    CPDLC_DM67i_WHEN_CAN_WE_EXPCT_DES_TO_alt);
		}
	}
	CPDLC_ASSERT_MSG(msg->num_segs == 0 || msg->segs[0].info->is_dl ==
	    is_dl, "Can't mix DM and UM message segments in a single "
	    "message %p", msg);
	if (msg->num_segs >= CPDLC_MAX_MSG_SEGS)
		return (-1);
	seg = &msg->segs[msg->num_segs];

	seg->info = msg_infos_lookup(is_dl, msg_type, msg_subtype);
	CPDLC_ASSERT(seg->info != NULL);

	return (msg->num_segs++);
}

void
cpdlc_msg_del_seg(cpdlc_msg_t *msg, unsigned seg_nr)
{
	cpdlc_msg_seg_t *seg;

	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT3U(seg_nr, <, msg->num_segs);
	/*
	 * Simply shift all the message segments after this one,
	 * forward by one step.
	 */
	seg = &msg->segs[seg_nr];
	CPDLC_ASSERT(seg->info != NULL);
	for (unsigned i = 0; i < msg->segs[seg_nr].info->num_args; i++) {
		if (seg->info->args[i] == CPDLC_ARG_ROUTE)
			free(seg->args[i].route);
		else if (seg->info->args[i] == CPDLC_ARG_FREETEXT)
			free(seg->args[i].freetext);
	}
	memmove(&msg->segs[seg_nr], &msg->segs[seg_nr + 1],
	    (msg->num_segs - seg_nr - 1) * sizeof (cpdlc_msg_seg_t));
	memset(&msg->segs[msg->num_segs], 0, sizeof (cpdlc_msg_seg_t));
	msg->num_segs--;
}

unsigned
cpdlc_msg_seg_get_num_args(const cpdlc_msg_t *msg, unsigned seg_nr)
{
	const cpdlc_msg_seg_t *seg;

	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT3U(seg_nr, <, msg->num_segs);
	seg = &msg->segs[seg_nr];
	CPDLC_ASSERT(seg->info != NULL);

	return (seg->info->num_args);
}

cpdlc_arg_type_t
cpdlc_msg_seg_get_arg_type(const cpdlc_msg_t *msg, unsigned seg_nr,
    unsigned arg_nr)
{
	const cpdlc_msg_seg_t *seg;
	const cpdlc_msg_info_t *info;

	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT3U(seg_nr, <, msg->num_segs);
	seg = &msg->segs[seg_nr];
	CPDLC_ASSERT(seg->info != NULL);
	info = seg->info;
	CPDLC_ASSERT3U(arg_nr, <, info->num_args);

	return (info->args[arg_nr]);
}

void
cpdlc_msg_seg_set_arg(cpdlc_msg_t *msg, unsigned seg_nr, unsigned arg_nr,
    const void *arg_val1, const void *arg_val2)
{
	const cpdlc_msg_info_t *info;
	cpdlc_msg_seg_t *seg;
	cpdlc_arg_t *arg;

	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT3U(seg_nr, <, msg->num_segs);
	seg = &msg->segs[seg_nr];
	CPDLC_ASSERT(seg->info != NULL);
	info = seg->info;
	CPDLC_ASSERT3U(arg_nr, <, info->num_args);
	arg = &seg->args[arg_nr];

	CPDLC_ASSERT(arg_val1 != NULL);

	switch (info->args[arg_nr]) {
	case CPDLC_ARG_ALTITUDE:
		CPDLC_ASSERT(arg_val2 != NULL);
		arg->alt.fl = *(bool *)arg_val1;
		arg->alt.alt = *(int *)arg_val2;
		break;
	case CPDLC_ARG_SPEED:
		CPDLC_ASSERT(arg_val2 != NULL);
		arg->spd.mach = *(bool *)arg_val1;
		arg->spd.spd = *(int *)arg_val2;
		break;
	case CPDLC_ARG_TIME:
	case CPDLC_ARG_TIME_DUR:
		CPDLC_ASSERT(arg_val2 != NULL);
		arg->time.hrs = *(int *)arg_val1;
		arg->time.mins = *(int *)arg_val2;
		break;
	case CPDLC_ARG_POSITION:
		arg->pos = *(cpdlc_pos_t *)arg_val1;
		break;
	case CPDLC_ARG_DIRECTION:
		arg->dir = *(cpdlc_dir_t *)arg_val1;
		break;
	case CPDLC_ARG_DISTANCE:
		arg->dist = *(double *)arg_val1;
		CPDLC_ASSERT3F(arg->dist, >=, 0);
		break;
	case CPDLC_ARG_VVI:
		arg->vvi = *(int *)arg_val1;
		break;
	case CPDLC_ARG_TOFROM:
		arg->tofrom = *(bool *)arg_val1;
		break;
	case CPDLC_ARG_ROUTE:
		if (arg->route != NULL)
			free(arg->route);
		arg->route = duplicate_route(arg_val1);
		break;
	case CPDLC_ARG_PROCEDURE:
		cpdlc_strlcpy(arg->proc, arg_val1, sizeof (arg->proc));
		break;
	case CPDLC_ARG_SQUAWK:
		arg->squawk = *(unsigned *)arg_val1;
		break;
	case CPDLC_ARG_ICAONAME:
		CPDLC_ASSERT(arg_val2 != NULL);
		cpdlc_strlcpy(arg->icaoname.icao, arg_val1,
		    sizeof (arg->icaoname.icao));
		cpdlc_strlcpy(arg->icaoname.name, arg_val2,
		    sizeof (arg->icaoname.name));
		break;
	case CPDLC_ARG_FREQUENCY:
		arg->freq = *(double *)arg_val1;
		break;
	case CPDLC_ARG_DEGREES:
		CPDLC_ASSERT(arg_val2 != NULL);
		arg->deg.deg = *(unsigned *)arg_val1;
		arg->deg.tru = *(bool *)arg_val2;
		break;
	case CPDLC_ARG_BARO:
		CPDLC_ASSERT(arg_val2 != NULL);
		arg->baro.hpa = *(bool *)arg_val1;
		arg->baro.val = *(double *)arg_val2;
		break;
	case CPDLC_ARG_FREETEXT:
		if (arg->freetext != NULL)
			free(arg->freetext);
		arg->freetext = strdup(arg_val1);
		break;
	case CPDLC_ARG_PERSONS:
		arg->pob = *(unsigned *)arg_val1;
		CPDLC_ASSERT3U(arg->pob, <=, 1024);
		break;
	case CPDLC_ARG_POSREPORT:
		arg->pos_rep = *(cpdlc_pos_rep_t *)arg_val1;
		break;
	default:
		CPDLC_VERIFY_MSG(0, "Message %p segment %d (%d/%d/%d) "
		    "contains invalid argument %d type %x", msg, seg_nr,
		    info->is_dl, info->msg_type, info->msg_subtype, arg_nr,
		    info->args[arg_nr]);
	}
}

unsigned
cpdlc_msg_seg_get_arg(const cpdlc_msg_t *msg, unsigned seg_nr, unsigned arg_nr,
    void *arg_val1, unsigned str_cap, void *arg_val2)
{
	const cpdlc_msg_info_t *info;
	const cpdlc_msg_seg_t *seg;
	const cpdlc_arg_t *arg;

	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT3U(seg_nr, <, msg->num_segs);
	seg = &msg->segs[seg_nr];
	CPDLC_ASSERT(seg->info != NULL);
	info = seg->info;
	CPDLC_ASSERT3U(arg_nr, <, info->num_args);
	arg = &seg->args[arg_nr];

	switch (info->args[arg_nr]) {
	case CPDLC_ARG_ALTITUDE:
		CPDLC_ASSERT(arg_val1 != NULL);
		CPDLC_ASSERT(arg_val2 != NULL);
		*(bool *)arg_val1 = arg->alt.fl;
		*(int *)arg_val2 = arg->alt.alt;
		return (sizeof (arg->alt));
	case CPDLC_ARG_SPEED:
		CPDLC_ASSERT(arg_val1 != NULL);
		CPDLC_ASSERT(arg_val2 != NULL);
		*(bool *)arg_val1 = arg->spd.mach;
		*(int *)arg_val2 = arg->spd.spd;
		return (sizeof (arg->spd));
	case CPDLC_ARG_TIME:
	case CPDLC_ARG_TIME_DUR:
		CPDLC_ASSERT(arg_val1 != NULL);
		CPDLC_ASSERT(arg_val2 != NULL);
		*(int *)arg_val1 = arg->time.hrs;
		*(int *)arg_val2 = arg->time.mins;
		return (sizeof (arg->time));
	case CPDLC_ARG_POSITION:
		CPDLC_ASSERT(arg_val1 != NULL || str_cap == 0);
		if (str_cap >= sizeof (arg->pos))
			memcpy(arg_val1, &arg->pos, sizeof (arg->pos));
		return (sizeof (arg->pos));
	case CPDLC_ARG_DIRECTION:
		CPDLC_ASSERT(arg_val1 != NULL);
		*(cpdlc_dir_t *)arg_val1 = arg->dir;
		return (sizeof (arg->dir));
	case CPDLC_ARG_DISTANCE:
		CPDLC_ASSERT(arg_val1 != NULL);
		*(double *)arg_val1 = arg->dist;
		return (arg->dist);
	case CPDLC_ARG_VVI:
		CPDLC_ASSERT(arg_val1 != NULL);
		*(int *)arg_val1 = arg->vvi;
		return (arg->vvi);
	case CPDLC_ARG_TOFROM:
		CPDLC_ASSERT(arg_val1 != NULL);
		*(bool *)arg_val1 = arg->tofrom;
		return (arg->tofrom);
	case CPDLC_ARG_ROUTE:
		CPDLC_ASSERT(arg_val1 != NULL || str_cap == 0);
		if (arg->route == NULL) {
			memset(arg_val1, 0, sizeof (cpdlc_route_t));
			return (0);
		}
		memcpy(arg_val1, arg->route, sizeof (cpdlc_route_t));
		return (sizeof (cpdlc_route_t));
	case CPDLC_ARG_PROCEDURE:
		CPDLC_ASSERT(arg_val1 != NULL || str_cap == 0);
		cpdlc_strlcpy(arg_val1, arg->proc, str_cap);
		return (strlen(arg->proc));
	case CPDLC_ARG_SQUAWK:
		CPDLC_ASSERT(arg_val1 != NULL);
		*(unsigned *)arg_val1 = arg->squawk;
		return (sizeof (arg->squawk));
	case CPDLC_ARG_ICAONAME:
		CPDLC_ASSERT(arg_val1 != NULL || str_cap == 0);
		cpdlc_strlcpy(arg_val1, arg->icaoname.icao, str_cap);
		cpdlc_strlcpy(arg_val2, arg->icaoname.name, str_cap);
		return (strlen(arg->icaoname.name));
	case CPDLC_ARG_FREQUENCY:
		CPDLC_ASSERT(arg_val1 != NULL);
		*(double *)arg_val1 = arg->freq;
		return (sizeof (arg->freq));
	case CPDLC_ARG_DEGREES:
		CPDLC_ASSERT(arg_val1 != NULL);
		CPDLC_ASSERT(arg_val2 != NULL);
		*(unsigned *)arg_val1 = arg->deg.deg;
		*(bool *)arg_val2 = arg->deg.tru;
		return (sizeof (arg->deg));
	case CPDLC_ARG_BARO:
		CPDLC_ASSERT(arg_val1 != NULL);
		CPDLC_ASSERT(arg_val2 != NULL);
		*(bool *)arg_val1 = arg->baro.hpa;
		*(double *)arg_val2 = arg->baro.val;
		return (sizeof (arg->baro));
	case CPDLC_ARG_FREETEXT:
		CPDLC_ASSERT(arg_val1 != NULL || str_cap == 0);
		if (arg->freetext == NULL) {
			if (str_cap > 0)
				*(char *)arg_val1 = '\0';
			return (0);
		}
		cpdlc_strlcpy(arg_val1, arg->freetext, str_cap);
		return (strlen(arg->freetext));
	case CPDLC_ARG_PERSONS:
		CPDLC_ASSERT(arg_val1 != NULL);
		*(unsigned *)arg_val1 = arg->pob;
		return (sizeof (arg->pob));
	case CPDLC_ARG_POSREPORT:
		CPDLC_ASSERT(arg_val1 != NULL);
		*(cpdlc_pos_rep_t *)arg_val1 = arg->pos_rep;
		return (sizeof (arg->pos_rep));
	}
	CPDLC_VERIFY_MSG(0, "Message %p segment %d (%d/%d/%d) contains "
	    "invalid argument %d type %x", msg, seg_nr, info->is_dl,
	    info->msg_type, info->msg_subtype, arg_nr, info->args[arg_nr]);
}
