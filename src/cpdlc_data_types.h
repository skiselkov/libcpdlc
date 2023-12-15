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

#ifndef	_LIBCPDLC_DATA_TYPES_H_
#define	_LIBCPDLC_DATA_TYPES_H_

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "cpdlc_core.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
	CPDLC_ARG_ALTITUDE,
	CPDLC_ARG_SPEED,
	CPDLC_ARG_TIME,
	CPDLC_ARG_TIME_DUR,
	CPDLC_ARG_POSITION,
	CPDLC_ARG_DIRECTION,
	CPDLC_ARG_DISTANCE,
	CPDLC_ARG_DISTANCE_OFFSET,
	CPDLC_ARG_VVI,
	CPDLC_ARG_TOFROM,
	CPDLC_ARG_ROUTE,
	CPDLC_ARG_PROCEDURE,
	CPDLC_ARG_SQUAWK,
	CPDLC_ARG_ICAO_ID,
	CPDLC_ARG_ICAO_NAME,
	CPDLC_ARG_FREQUENCY,
	CPDLC_ARG_DEGREES,
	CPDLC_ARG_BARO,
	CPDLC_ARG_FREETEXT,
	CPDLC_ARG_PERSONS,
	CPDLC_ARG_POSREPORT,
	CPDLC_ARG_PDC,
	CPDLC_ARG_TP4TABLE,
	CPDLC_ARG_ERRINFO,
	CPDLC_ARG_VERSION,
	CPDLC_ARG_ATIS_CODE,
	CPDLC_ARG_LEGTYPE
} cpdlc_arg_type_t;

typedef enum {
	CPDLC_DIR_LEFT =	0,
	CPDLC_DIR_RIGHT =	1,
	CPDLC_DIR_EITHER =	2,
	CPDLC_DIR_NORTH =	3,
	CPDLC_DIR_SOUTH =	4,
	CPDLC_DIR_EAST =	5,
	CPDLC_DIR_WEST =	6,
	CPDLC_DIR_NE =		7,
	CPDLC_DIR_NW =		8,
	CPDLC_DIR_SE =		9,
	CPDLC_DIR_SW =		10
} cpdlc_dir_t;

#define	CPDLC_NULL_SPD		((cpdlc_spd_t){false, false, false, 0})
#define	CPDLC_IS_NULL_SPD(_sp)	((_sp).spd == 0)

typedef struct {
	bool			mach;
	bool			tru;
	bool			gnd;
	unsigned		spd;	/* knots or 1/1000th of Mach */
} cpdlc_spd_t;

#define	CPDLC_NULL_ALT		((cpdlc_alt_t){false, false, -9999})
#define	CPDLC_IS_NULL_ALT(a)	((a).alt <= -2000)

typedef struct {
	bool			fl;	/* flight level? */
	bool			met;	/* metric? */
	int			alt;	/* feet */
} cpdlc_alt_t;

#define	CPDLC_NULL_TIME		((cpdlc_time_t){-1, -1})
#define	CPDLC_IS_NULL_TIME(tim)	((tim).hrs < 0 || (tim).mins < 0)

typedef struct {
	int			hrs;
	int			mins;
} cpdlc_time_t;

typedef enum {
	CPDLC_AT,
	CPDLC_AT_OR_ABV,
	CPDLC_AT_OR_BLW
} cpdlc_alt_cstr_type_t;

typedef struct {
	cpdlc_alt_cstr_type_t	toler;
	cpdlc_alt_t		alt;
} cpdlc_alt_cstr_t;

#define	CPDLC_NULL_LAT_LON		((cpdlc_lat_lon_t){NAN, NAN})
#define	CPDLC_IS_NULL_LAT_LON(ll)	(isnan((ll).lat) || isnan((ll).lon))

typedef struct {
	double		lat;		/* NAN if CPDLC_NULL_LAT_LON */
	double		lon;		/* NAN if CPDLC_NULL_LAT_LON */
} cpdlc_lat_lon_t;

typedef struct {
	char		fixname[8];	/* required */
	cpdlc_lat_lon_t	lat_lon;	/* optional, CPDLC_NULL_LAT_LON */
} cpdlc_pub_ident_t;

typedef struct {
	char		fixname[8];	/* required */
	cpdlc_lat_lon_t	lat_lon;	/* optional, CPDLC_NULL_LAT_LON */
	unsigned	degrees;	/* required */
} cpdlc_pb_t;

typedef cpdlc_pb_t cpdlc_pbpb_t[2];

typedef struct {
	char		fixname[8];	/* required */
	cpdlc_lat_lon_t	lat_lon;	/* optional, CPDLC_NULL_LAT_LON */
	unsigned	degrees;	/* required */
	float		dist_nm;	/* required */
} cpdlc_pbd_t;

typedef enum {
	CPDLC_POS_FIXNAME,
	CPDLC_POS_NAVAID,
	CPDLC_POS_AIRPORT,
	CPDLC_POS_LAT_LON,
	CPDLC_POS_PBD,
	CPDLC_POS_UNKNOWN
} cpdlc_pos_type_t;

#define	CPDLC_NULL_POS		((cpdlc_pos_t){false})
#define	CPDLC_IS_NULL_POS(pos)	(!(pos).set)

typedef struct {
	bool			set;
	cpdlc_pos_type_t	type;
	union {
		char		fixname[8];	/* CPDLC_POS_FIXNAME */
		char		navaid[8];	/* CPDLC_POS_NAVAID */
		char		airport[8];	/* CPDLC_POS_AIRPORT */
		cpdlc_lat_lon_t	lat_lon;	/* CPDLC_POS_LAT_LON */
		cpdlc_pbd_t	pbd;		/* CPDLC_POS_PBD */
		char		str[16];	/* CPDLC_POS_UNKNOWN */
	};
} cpdlc_pos_t;

typedef struct {
	cpdlc_pos_t		pos;
	float			dist_nm;
	bool			spd_cstr_present;
	cpdlc_spd_t		spd_cstr;
	unsigned		num_alt_cstr;
	cpdlc_alt_cstr_t	alt_cstr[2];
} cpdlc_atk_wpt_t;

typedef enum {
	CPDLC_INTC_FROM_PUB_IDENT,
	CPDLC_INTC_FROM_LAT_LON,
	CPDLC_INTC_FROM_PBPB,
	CPDLC_INTC_FROM_PBD
} cpdlc_intc_from_type_t;

typedef struct {
	cpdlc_intc_from_type_t		type;
	union {
		cpdlc_pub_ident_t	pub_ident;
		cpdlc_lat_lon_t		lat_lon;
		cpdlc_pbpb_t		pbpb;
		cpdlc_pbd_t		pbd;
	};
	unsigned			degrees;
} cpdlc_intc_from_t;

typedef enum {
	CPDLC_HOLD_LEG_NONE,
	CPDLC_HOLD_LEG_DIST,
	CPDLC_HOLD_LEG_TIME
} cpdlc_hold_leg_type_t;

typedef struct {
	cpdlc_hold_leg_type_t	type;
	union {
		float		dist_nm;	/* 0.1 - 99.9 NM */
		float		time_min;	/* 0.1 - 9.9 minutes */
	};
} cpdlc_hold_leg_t;

typedef struct {
	cpdlc_pos_t		pos;	/* required */
	cpdlc_spd_t		spd_low;/* optional, CPDLC_NULL_SPD */
	bool			alt_present;
	cpdlc_alt_cstr_t	alt;	/* optional */
	cpdlc_spd_t		spd_high;/* optional, CPDLC_NULL_SPD */
	cpdlc_dir_t		dir;	/* optional, CPDLC_DIR_EITHER */
	unsigned		degrees;/* optional, 0 */
	cpdlc_time_t		efc;	/* optional, CPDLC_NULL_TIME */
	cpdlc_hold_leg_t	leg;	/* optional, CPDLC_HOLD_LEG_NONE */
} cpdlc_hold_at_t;

typedef struct {
	cpdlc_pos_t		pos;
	cpdlc_spd_t		spd;	/* optional, CPDLC_NULL_SPD */
	cpdlc_alt_cstr_t	alt[2];	/* optional, CPDLC_NULL_ALT */
} cpdlc_wpt_spd_alt_t;

typedef enum {
	CPDLC_TIME_TOLER_AT =		0,
	CPDLC_TIME_TOLER_AT_OR_AFTER =	1,
	CPDLC_TIME_TOLER_AT_OR_BEFORE =	2,
} cpdlc_time_toler_t;

typedef struct {
	cpdlc_pos_t		pos;
	cpdlc_time_t		time;
	cpdlc_time_toler_t	toler;
} cpdlc_rta_t;

typedef struct {
	bool			rpt_lat;
	double			degrees;	/* lat/lon, degrees */
	unsigned		deg_incr;	/* optional, 0=unset */
} cpdlc_rpt_pts_t;

typedef struct {
	unsigned		num_atk_wpt;
	cpdlc_atk_wpt_t		atk_wpt[8];
	cpdlc_rpt_pts_t		rpt_pts;
	unsigned		num_intc_from;
	cpdlc_intc_from_t	intc_from[4];
	unsigned		num_hold_at_wpt;
	cpdlc_hold_at_t		hold_at_wpt[4];
	unsigned		num_wpt_spd_alt;
	cpdlc_wpt_spd_alt_t	wpt_spd_alt[32];
	unsigned		num_rta;
	cpdlc_rta_t		rta[32];
} cpdlc_route_add_info_t;

typedef enum {
	CPDLC_PROC_UNKNOWN = 0,
	CPDLC_PROC_ARRIVAL = 1,
	CPDLC_PROC_APPROACH = 2,
	CPDLC_PROC_DEPARTURE = 3
} cpdlc_proc_type_t;

typedef struct {
	cpdlc_proc_type_t	type;
	char			name[8];
	char			trans[8];
} cpdlc_proc_t;

#define	CPDLC_TRK_DETAIL_MAX_LAT_LON	128

typedef struct {
	char			name[8];
	unsigned		num_lat_lon;
	cpdlc_lat_lon_t		lat_lon[CPDLC_TRK_DETAIL_MAX_LAT_LON];
} cpdlc_trk_detail_t;

typedef enum {
	CPDLC_ROUTE_PUB_IDENT,		/* Published database identifier */
	CPDLC_ROUTE_LAT_LON,		/* Latitude+Longitude */
	CPDLC_ROUTE_PBPB,		/* Place-Bearing/Place-Bearing */
	CPDLC_ROUTE_PBD,		/* Place/Bearing/Distance */
	CPDLC_ROUTE_AWY,		/* Airway */
	CPDLC_ROUTE_TRACK_DETAIL,	/* Prescribed track */
	CPDLC_ROUTE_UNKNOWN		/* Unknown - fixname or airway */
} cpdlc_route_info_type_t;

typedef struct {
	cpdlc_route_info_type_t		type;
	union {
		/* CPDLC_ROUTE_PUB_IDENT */
		cpdlc_pub_ident_t	pub_ident;
		/* CPDLC_ROUTE_LAT_LON */
		cpdlc_lat_lon_t		lat_lon;
		/* CPDLC_ROUTE_PBPB */
		cpdlc_pbpb_t		pbpb;
		/* CPDLC_ROUTE_PBD */
		cpdlc_pbd_t		pbd;
		/* CPDLC_ROUTE_AWY */
		char			awy[8];
		/* CPDLC_ROUTE_UNKNOWN */
		char			str[24];
	};
} cpdlc_route_info_t;

#define	CPDLC_ROUTE_MAX_INFO	128

typedef struct {
	char			orig_icao[8];
	char			dest_icao[8];
	char			orig_rwy[8];
	char			dest_rwy[8];
	cpdlc_proc_t		sid;
	cpdlc_proc_t		star;
	cpdlc_proc_t		appch;
	char			awy_intc[8];
	unsigned		num_info;
	cpdlc_route_info_t	info[CPDLC_ROUTE_MAX_INFO];
	cpdlc_route_add_info_t	add_info;
	/* CPDLC_ROUTE_TRACK_DETAIL */
	cpdlc_trk_detail_t	trk_detail;
} cpdlc_route_t;

#define	CPDLC_NULL_WIND		((cpdlc_wind_t){0, 0})
#define	CPDLC_IS_NULL_WIND(wind) ((wind).dir == 0)

typedef struct {
	unsigned		dir;		/* Degrees, 1-360 */
	unsigned		spd;		/* Knots */
} cpdlc_wind_t;

typedef enum {
	CPDLC_TURB_NONE,
	CPDLC_TURB_LIGHT,
	CPDLC_TURB_MOD,
	CPDLC_TURB_SEV
} cpdlc_turb_t;

typedef enum {
	CPDLC_ICING_NONE,
	CPDLC_ICING_TRACE,
	CPDLC_ICING_LIGHT,
	CPDLC_ICING_MOD,
	CPDLC_ICING_SEV
} cpdlc_icing_t;

#define	CPDLC_NULL_TEMP		-100
#define	CPDLC_IS_NULL_TEMP(temp) ((temp) <= -100)

#define	CPDLC_NULL_POS_REP	\
	((cpdlc_pos_rep_t){ \
		CPDLC_NULL_POS,		/* cur_pos */ \
		CPDLC_NULL_TIME,	/* time_cur_pos */ \
		CPDLC_NULL_ALT,		/* alt */ \
		CPDLC_NULL_POS,		/* fix_next */ \
		CPDLC_NULL_TIME,	/* time_fix_next */ \
		CPDLC_NULL_POS,		/* fix_next_p1 */ \
		CPDLC_NULL_TIME,	/* time_dest */ \
		CPDLC_NULL_TIME,	/* rmng_fuel */ \
		CPDLC_NULL_TEMP,	/* temp */ \
		CPDLC_NULL_WIND,	/* wind */ \
		CPDLC_TURB_NONE,	/* turb */ \
		CPDLC_ICING_NONE,	/* icing */ \
		CPDLC_NULL_SPD,		/* spd */ \
		CPDLC_NULL_SPD,		/* spd_gnd */ \
		false,			/* vvi_set */ \
		0,			/* vvi */ \
		0,			/* trk */ \
		0,			/* hdg_true */ \
		false,			/* dist_set */ \
		0,			/* dist_nm */ \
		{},			/* remarks */ \
		CPDLC_NULL_POS,		/* rpt_wpt_pos */ \
		CPDLC_NULL_TIME,	/* rpt_wpt_time */ \
		CPDLC_NULL_ALT		/* rpt_wpt_alt */ \
	})

typedef struct {
	cpdlc_pos_t		cur_pos;	/* Required */
	cpdlc_time_t		time_cur_pos;	/* Required */
	cpdlc_alt_t		cur_alt;	/* Required */
	cpdlc_pos_t		fix_next;	/* Optional, CPDLC_NULL_POS */
	cpdlc_time_t		time_fix_next;	/* Optional, CPDLC_NULL_TIME */
	cpdlc_pos_t		fix_next_p1;	/* Optional, CPDLC_NULL_POS */
	cpdlc_time_t		time_dest;	/* Optional, CPDLC_NULL_TIME */
	cpdlc_time_t		rmng_fuel;	/* Optional, CPDLC_NULL_TIME */
	int			temp;		/* Optional, CPDLC_NULL_TEMP */
	cpdlc_wind_t		wind;		/* Optional, CPDLC_NULL_WIND */
	cpdlc_turb_t		turb;		/* Optional, CPDLC_TURB_NONE */
	cpdlc_icing_t		icing;		/* Optional, CPDLC_ICING_NONE */
	cpdlc_spd_t		spd;		/* Optional, CPDLC_NULL_SPD */
	cpdlc_spd_t		spd_gnd;	/* Optional, CPDLC_NULL_SPD */
	bool			vvi_set;
	int			vvi;		/* Optional, vvi_set=false */
	unsigned		trk;		/* Optional, trk=0 */
	unsigned		hdg_true;	/* Optional, hdg_true=0 */
	bool			dist_set;
	float			dist_nm;	/* Optional, dist_set=false */
	char			remarks[256];	/* Optional, empty */
	cpdlc_pos_t		rpt_wpt_pos;	/* Optional, CPDLC_NULL_POS */
	cpdlc_time_t		rpt_wpt_time;	/* Optional, CPDLC_NULL_TIME */
	cpdlc_alt_t		rpt_wpt_alt;	/* Optional, CPDLC_NULL_ALT */
} cpdlc_pos_rep_t;

typedef enum {
	CPDLC_COM_NAV_LORAN_A,
	CPDLC_COM_NAV_LORAN_C,
	CPDLC_COM_NAV_DME,
	CPDLC_COM_NAV_DECCA,
	CPDLC_COM_NAV_ADF,
	CPDLC_COM_NAV_GNSS,
	CPDLC_COM_NAV_HF_RTF,
	CPDLC_COM_NAV_INS,
	CPDLC_COM_NAV_ILS,
	CPDLC_COM_NAV_OMEGA,
	CPDLC_COM_NAV_VOR,
	CPDLC_COM_NAV_DOPPLER,
	CPDLC_COM_NAV_RNAV,
	CPDLC_COM_NAV_TACAN,
	CPDLC_COM_NAV_UHF_RTF,
	CPDLC_COM_NAV_VHF_RTF
} cpdlc_com_nav_eqpt_st_t;

typedef enum {
	CPDLC_SSR_EQPT_NIL,
	CPDLC_SSR_EQPT_XPDR_MODE_A,
	CPDLC_SSR_EQPT_XPDR_MODE_AC,
	CPDLC_SSR_EQPT_XPDR_MODE_S,
	CPDLC_SSR_EQPT_XPDR_MODE_SPA,
	CPDLC_SSR_EQPT_XPDR_MODE_SID,
	CPDLC_SSR_EQPT_XPDR_MODE_SPAID
} cpdlc_ssr_eqpt_t;

typedef struct {
	bool			com_nav_app_eqpt_avail;
	unsigned		num_com_nav_eqpt_st;
	cpdlc_com_nav_eqpt_st_t	com_nav_eqpt_st[16];
	cpdlc_ssr_eqpt_t	ssr_eqpt;
} cpdlc_acf_eqpt_code_t;

typedef struct {
	char			acf_id[8];	/* required */
	char			acf_type[8];	/* optional */
	cpdlc_acf_eqpt_code_t	acf_eqpt_code;	/* optional */
	cpdlc_time_t		time_dep;	/* required */
	cpdlc_route_t		route;		/* required */
	cpdlc_alt_t		alt_restr;	/* optional */
	double			freq;		/* required */
	unsigned		squawk;		/* required */
	unsigned		revision;	/* required */
} cpdlc_pdc_t;

typedef enum {
	CPDLC_TP4_LABEL_A,
	CPDLC_TP4_LABEL_B
} cpdlc_tp4table_t;

typedef enum {
	CPDLC_FAC_FUNC_CENTER,
	CPDLC_FAC_FUNC_APPROACH,
	CPDLC_FAC_FUNC_TOWER,
	CPDLC_FAC_FUNC_FINAL,
	CPDLC_FAC_FUNC_GROUND,
	CPDLC_FAC_FUNC_CLX,
	CPDLC_FAC_FUNC_DEPARTURE,
	CPDLC_FAC_FUNC_CONTROL
} cpdlc_fac_func_t;

typedef struct {
	bool			is_name;
	union {
		char		icao_id[8];
		char		name[24];
	};
	cpdlc_fac_func_t        func;
} cpdlc_icao_name_t;

typedef struct {
	bool			hpa;
	double			val;
} cpdlc_altimeter_t;

typedef enum {
	CPDLC_ERRINFO_APP_ERROR =		0,
	CPDLC_ERRINFO_DUP_MIN =			1,
	CPDLC_ERRINFO_UNRECOG_MRN =		2,
	CPDLC_ERRINFO_END_SVC_WITH_PDG_MSGS =	3,
	CPDLC_ERRINFO_END_SVC_WITH_NO_RESP =	4,
	CPDLC_ERRINFO_INSUFF_MSG_STORAGE =	5,
	CPDLC_ERRINFO_NO_AVBL_MIN =		6,
	CPDLC_ERRINFO_COMMANDED_TERM =		7,
	CPDLC_ERRINFO_INSUFF_DATA =		8,
	CPDLC_ERRINFO_UNEXPCT_DATA =		9,
	CPDLC_ERRINFO_INVAL_DATA =		10
} cpdlc_errinfo_t;

typedef struct {
	bool			is_time;
	double			param;	/* minutes, or nm */
} cpdlc_legtype_t;

typedef union {
	cpdlc_alt_t		alt;
	cpdlc_spd_t		spd;
	cpdlc_time_t		time;
	cpdlc_pos_t		pos;
	cpdlc_dir_t		dir;
	double			dist;	/* nautical miles */
	int			vvi;	/* feet per minute */
	bool			tofrom;	/* true = to, false = from */
	cpdlc_route_t		*route;
	cpdlc_proc_t		proc;
	unsigned		squawk;
	char			icao_id[8];
	cpdlc_icao_name_t	icao_name;
	double			freq;
	struct {
		unsigned	deg;
		bool		tru;
	} deg;
	cpdlc_altimeter_t	baro;
	char			*freetext;
	unsigned		pob;
	cpdlc_pos_rep_t		pos_rep;
	cpdlc_pdc_t		*pdc;
	cpdlc_tp4table_t	tp4;
	cpdlc_errinfo_t		errinfo;
	unsigned		version;
	char			atis_code;
	cpdlc_legtype_t		legtype;
} cpdlc_arg_t;

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_DATA_TYPES_H_ */
