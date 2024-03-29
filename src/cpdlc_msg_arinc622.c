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

#include "asn1/ATCuplinkmessage.h"
#include "asn1/ATCdownlinkmessage.h"

#include "cpdlc_alloc.h"
#include "cpdlc_assert.h"
#include "cpdlc_crc.h"
#include "cpdlc_hexcode.h"
#include "cpdlc_msg_arinc622.h"
#include "cpdlc_msg_impl.h"
#include "cpdlc_string.h"

#define	MET2FEET(x)	((x) * 3.2808398950131)	/* meters to feet */
#define	FEET2MET(x)	((x) / 3.2808398950131)	/* meters to feet */
#define	FAH2C(f)	(((f) - 32) * 0.555555)

#define	SEQ_ADD(seq_name, elem) \
	do { \
		if ((seq_name) == NULL) \
			(seq_name) = safe_calloc(1, sizeof (*(seq_name))); \
		ASN_SEQUENCE_ADD((seq_name), (elem)); \
	} while (0)

static void
ia5strlcpy_in(char *out, const IA5String_t *in, unsigned cap)
{
	if (cap != 0) {
		unsigned len = MIN((unsigned)in->size, cap - 1);
		CPDLC_ASSERT(in != NULL);
		memcpy(out, in->buf, len);
		out[len] = '\0';	/* zero-terminate */
	}
}

static void
ia5strlcpy_out(IA5String_t *out, const char *in)
{
	CPDLC_ASSERT(out != NULL);
	CPDLC_ASSERT(in != NULL);
	out->size = strlen(in);
	out->buf = safe_malloc(out->size);
	memcpy(out->buf, in, out->size);
}

static const void *
get_asn_arg_ptr(const cpdlc_msg_info_t *info, unsigned nr, const void *elem)
{
	CPDLC_ASSERT(info != NULL);
	CPDLC_ASSERT(nr < info->num_args);
	CPDLC_ASSERT(elem != NULL);

	if (info->asn_arg_info[nr].is_seq) {
		const A_SEQUENCE_OF(void) *seq =
		    elem + info->asn_arg_info[nr].offset;
		return (seq->array[info->asn_arg_info[nr].seq_idx]);
	} else {
		return (elem + info->asn_arg_info[nr].offset);
	}
}

static unsigned
arg_type2asn_sz(cpdlc_arg_type_t type)
{
	switch (type) {
	case CPDLC_ARG_ALTITUDE:
		return (sizeof (Altitude_t));
	case CPDLC_ARG_SPEED:
		return (sizeof (Speed_t));
	case CPDLC_ARG_TIME:
	case CPDLC_ARG_TIME_DUR:
		return (sizeof (Time_t));
	case CPDLC_ARG_POSITION:
		return (sizeof (Position_t));
	case CPDLC_ARG_DIRECTION:
		return (sizeof (Direction_t));
	case CPDLC_ARG_DISTANCE:
		return (sizeof (Distance_t));
	case CPDLC_ARG_DISTANCE_OFFSET:
		return (sizeof (Distanceoffset_t));
	case CPDLC_ARG_VVI:
		return (sizeof (Verticalrate_t));
	case CPDLC_ARG_TOFROM:
		return (sizeof (Tofrom_t));
	case CPDLC_ARG_ROUTE:
		return (sizeof (Routeclearance_t));
	case CPDLC_ARG_PROCEDURE:
		return (sizeof (Procedurename_t));
	case CPDLC_ARG_SQUAWK:
		return (sizeof (Beaconcode_t));
	case CPDLC_ARG_ICAO_ID:
		return (sizeof (ICAOfacilitydesignation_t));
	case CPDLC_ARG_ICAO_NAME:
		return (sizeof (ICAOunitname_t));
	case CPDLC_ARG_FREQUENCY:
		return (sizeof (Frequency_t));
	case CPDLC_ARG_DEGREES:
		return (sizeof (Degrees_t));
	case CPDLC_ARG_BARO:
		return (sizeof (Altimeter_t));
	case CPDLC_ARG_FREETEXT:
		return (sizeof (Freetext_t));
	case CPDLC_ARG_PERSONS:
		return (sizeof (Remainingsouls_t));
	case CPDLC_ARG_POSREPORT:
		return (sizeof (Positionreport_t));
	case CPDLC_ARG_PDC:
		return (sizeof (Predepartureclearance_t));
	case CPDLC_ARG_TP4TABLE:
		return (sizeof (Tp4table_t));
	case CPDLC_ARG_ERRINFO:
		return (sizeof (Errorinformation_t));
	case CPDLC_ARG_VERSION:
		return (sizeof (Versionnumber_t));
	case CPDLC_ARG_ATIS_CODE:
		return (sizeof (Atiscode_t));
	case CPDLC_ARG_LEGTYPE:
		return (sizeof (Legtype_t));
	}
	CPDLC_VERIFY(0);
}

static void *
get_asn_arg_ptr_wr(const cpdlc_msg_info_t *info, unsigned nr, void *elem)
{
	CPDLC_ASSERT(info != NULL);
	CPDLC_ASSERT(nr < info->num_args);
	CPDLC_ASSERT(elem != NULL);

	if (info->asn_arg_info[nr].is_seq) {
		A_SEQUENCE_OF(void) *seq =
		    elem + info->asn_arg_info[nr].offset;
		void *data = (seq->array != NULL ?
		    seq->array[info->asn_arg_info[nr].seq_idx] : NULL);
		if (data == NULL) {
			data = safe_calloc(1, arg_type2asn_sz(info->args[nr]));
			ASN_SEQUENCE_ADD(seq, data);
		}
		return (data);
	} else {
		return (elem + info->asn_arg_info[nr].offset);
	}
}

static void
msg_encode_hdr(const cpdlc_msg_t *msg, ATCmessageheader_t *hdr)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(hdr != NULL);

	hdr->msgidentificationnumber = msg->min % 64;
	if (msg->mrn != CPDLC_INVALID_MSG_SEQ_NR) {
		hdr->msgreferencenumber = safe_calloc(1,
		    sizeof (*hdr->msgreferencenumber));
		*hdr->msgreferencenumber = msg->mrn % 64;
	}
	hdr->timestamp = safe_calloc(1, sizeof (*hdr->timestamp));
	hdr->timestamp->timehours = msg->ts.hrs;
	hdr->timestamp->timeminutes = msg->ts.mins;
	hdr->timestamp->timeseconds = msg->ts.secs;
}

static void
latlon_cpdlc2asn(const cpdlc_lat_lon_t *ll_in, LatitudeLongitude_t *ll_out)
{
	CPDLC_ASSERT(ll_in != NULL);
	CPDLC_ASSERT(!CPDLC_IS_NULL_LAT_LON(*ll_in));
	CPDLC_ASSERT(ll_out != NULL);

	ll_out->latitude.latitudedirection = (ll_in->lat >= 0 ?
	    Latitudedirection_north : Latitudedirection_south);
	ll_out->latitude.latitudedegrees = floor(fabs(ll_in->lat));
	CPDLC_ASSERT(ll_out->latitude.minuteslatlon == NULL);
	ll_out->latitude.minuteslatlon = safe_calloc(1,
	    sizeof (*ll_out->latitude.minuteslatlon));
	*ll_out->latitude.minuteslatlon = round(fmod(fabs(ll_in->lat), 1) *
	    600);

	ll_out->longitude.longitudedirection = (ll_in->lon >= 0 ?
	    Longitudedirection_east : Longitudedirection_west);
	ll_out->longitude.longitudedegrees = floor(fabs(ll_in->lon));
	CPDLC_ASSERT(ll_out->longitude.minuteslatlon == NULL);
	ll_out->longitude.minuteslatlon = safe_calloc(1,
	    sizeof (*ll_out->longitude.minuteslatlon));
	*ll_out->longitude.minuteslatlon = round(fmod(fabs(ll_in->lon), 1) *
	    600);
}

static void
encode_alt_asn(const cpdlc_alt_t *alt_in, Altitude_t *alt_out)
{
	CPDLC_ASSERT(alt_in != NULL);
	CPDLC_ASSERT(alt_out != NULL);

	if (CPDLC_IS_NULL_ALT(*alt_in)) {
		alt_out->present = Altitude_PR_NOTHING;
		return;
	}
	if (alt_in->fl) {
		if (alt_in->met) {
			alt_out->present =
			    Altitude_PR_altitudeflightlevelmetric;
			alt_out->choice.altitudeflightlevelmetric =
			    round(FEET2MET(alt_in->alt) / 10.0);
		} else {
			alt_out->present = Altitude_PR_altitudeflightlevel;
			alt_out->choice.altitudeflightlevel =
			    round(alt_in->alt / 100.0);
		}
	} else {
		if (alt_in->met) {
			alt_out->present = Altitude_PR_altitudeqnhmeters;
			alt_out->choice.altitudeqnhmeters =
			    round(FEET2MET(alt_in->alt));
		} else {
			alt_out->present = Altitude_PR_altitudeqnh;
			alt_out->choice.altitudeqnh = round(alt_in->alt / 10.0);
		}
	}
}

static void
encode_spd_asn(const cpdlc_spd_t *spd_in, Speed_t *spd_out)
{
	CPDLC_ASSERT(spd_in != NULL);
	CPDLC_ASSERT(spd_out != NULL);

	if (CPDLC_IS_NULL_SPD(*spd_in)) {
		spd_out->present = Speed_PR_NOTHING;
		return;
	}
	if (spd_in->mach) {
		if (spd_in->spd >= 930) {
			spd_out->present = Speed_PR_speedmachlarge;
			spd_out->choice.speedmachlarge = spd_in->spd / 10;
		} else {
			spd_out->present = Speed_PR_speedmach;
			spd_out->choice.speedmach = spd_in->spd / 10;
		}
	} else if (spd_in->tru) {
		spd_out->present = Speed_PR_speedtrue;
		spd_out->choice.speedtrue = spd_in->spd / 10;
	} else if (spd_in->gnd) {
		spd_out->present = Speed_PR_speedground;
		spd_out->choice.speedground = spd_in->spd / 10;
	} else {
		spd_out->present = Speed_PR_speedindicated;
		spd_out->choice.speedindicated = spd_in->spd / 10;
	}
}

static void
encode_time_asn(const cpdlc_time_t *time_in, Time_t *time_out)
{
	CPDLC_ASSERT(time_in != NULL);
	CPDLC_ASSERT(time_out != NULL);

	if (CPDLC_IS_NULL_TIME(*time_in))
		return;
	time_out->timehours = time_in->hrs;
	time_out->timeminutes = time_in->mins;
}

static void
encode_degrees_asn(double degrees, bool tru, Degrees_t *deg_out)
{
	CPDLC_ASSERT(deg_out != NULL);
	if (tru) {
		deg_out->present = Degrees_PR_degreestrue;
		deg_out->choice.degreestrue = degrees;
	} else {
		deg_out->present = Degrees_PR_degreesmagnetic;
		deg_out->choice.degreesmagnetic = degrees;
	}
}

static void
encode_dist_asn(double dist_nm, Distance_t *dist_out)
{
	CPDLC_ASSERT(dist_out != NULL);
	dist_out->present = Distance_PR_distancenm;
	dist_out->choice.distancenm = round(dist_nm * 10);
}

static void
encode_dist_off_asn(double dist_nm, Distanceoffset_t *dist_out)
{
	CPDLC_ASSERT(dist_out != NULL);
	dist_out->present = Distanceoffset_PR_distanceoffsetnm;
	dist_out->choice.distanceoffsetnm = round(dist_nm);
}

static void
encode_pbd_asn(const cpdlc_pbd_t *pbd_in, Placebearingdistance_t *pbd_out)
{
	CPDLC_ASSERT(pbd_in != NULL);
	CPDLC_ASSERT(pbd_out != NULL);

	ia5strlcpy_out(&pbd_out->fixname, pbd_in->fixname);
	if (!CPDLC_IS_NULL_LAT_LON(pbd_in->lat_lon)) {
		pbd_out->latitudelongitude = safe_calloc(1,
		    sizeof (*pbd_out->latitudelongitude));
		latlon_cpdlc2asn(&pbd_in->lat_lon, pbd_out->latitudelongitude);
	}
	encode_degrees_asn(pbd_in->degrees, false, &pbd_out->degrees);
	encode_dist_asn(pbd_in->dist_nm, &pbd_out->distance);
}

static void
encode_pos_asn(const cpdlc_pos_t *pos_in, Position_t *pos_out)
{
	CPDLC_ASSERT(pos_in != NULL);
	CPDLC_ASSERT(pos_out != NULL);

	if (!pos_in->set) {
		pos_out->present = Position_PR_NOTHING;
		return;
	}
	switch (pos_in->type) {
	case CPDLC_POS_FIXNAME:
		pos_out->present = Position_PR_fixname;
		ia5strlcpy_out(&pos_out->choice.fixname, pos_in->fixname);
		break;
	case CPDLC_POS_NAVAID:
		pos_out->present = Position_PR_navaid;
		ia5strlcpy_out(&pos_out->choice.navaid, pos_in->navaid);
		break;
	case CPDLC_POS_AIRPORT:
		pos_out->present = Position_PR_airport;
		ia5strlcpy_out(&pos_out->choice.airport, pos_in->navaid);
		break;
	case CPDLC_POS_LAT_LON:
		pos_out->present = Position_PR_latitudeLongitude;
		latlon_cpdlc2asn(&pos_in->lat_lon,
		    &pos_out->choice.latitudeLongitude);
		break;
	case CPDLC_POS_PBD:
		pos_out->present = Position_PR_placebearingdistance;
		encode_pbd_asn(&pos_in->pbd,
		    &pos_out->choice.placebearingdistance);
		break;
	case CPDLC_POS_UNKNOWN:
		pos_out->present = Position_PR_fixname;
		ia5strlcpy_out(&pos_out->choice.fixname, pos_in->str);
		break;
	}
}

static void
encode_dir_asn(cpdlc_dir_t dir_in, Direction_t *dir_out)
{
	CPDLC_ASSERT(dir_out != NULL);
	switch (dir_in) {
	case CPDLC_DIR_LEFT:
		*dir_out = Direction_left;
		break;
	case CPDLC_DIR_RIGHT:
		*dir_out = Direction_right;
		break;
	case CPDLC_DIR_EITHER:
		*dir_out = Direction_eitherSide;
		break;
	case CPDLC_DIR_NORTH:
		*dir_out = Direction_north;
		break;
	case CPDLC_DIR_SOUTH:
		*dir_out = Direction_south;
		break;
	case CPDLC_DIR_EAST:
		*dir_out = Direction_east;
		break;
	case CPDLC_DIR_WEST:
		*dir_out = Direction_west;
		break;
	case CPDLC_DIR_NE:
		*dir_out = Direction_northEast;
		break;
	case CPDLC_DIR_NW:
		*dir_out = Direction_northWest;
		break;
	case CPDLC_DIR_SE:
		*dir_out = Direction_southEast;
		break;
	case CPDLC_DIR_SW:
		*dir_out = Direction_southWest;
		break;
	}
}

static void
encode_vvi_asn(int vvi, Verticalrate_t *vvi_out)
{
	CPDLC_ASSERT(vvi_out != NULL);
	vvi_out->present = Verticalrate_PR_verticalrateenglish;
	vvi_out->choice.verticalrateenglish = round(fabs(vvi / 100.0));
}

static void
encode_tofrom_asn(bool tofrom, Tofrom_t *tofrom_out)
{
	CPDLC_ASSERT(tofrom_out != NULL);
	*tofrom_out = (tofrom ? Tofrom_to : Tofrom_from);
}

static void
encode_proc_asn(const cpdlc_proc_t *proc_in, Procedurename_t *proc_out)
{
	CPDLC_ASSERT(proc_in != NULL);
	CPDLC_ASSERT(proc_out != NULL);
	switch (proc_in->type) {
	case CPDLC_PROC_UNKNOWN:
		return;
	case CPDLC_PROC_ARRIVAL:
		proc_out->proceduretype = Proceduretype_arrival;
		break;
	case CPDLC_PROC_APPROACH:
		proc_out->proceduretype = Proceduretype_approach;
		break;
	case CPDLC_PROC_DEPARTURE:
		proc_out->proceduretype = Proceduretype_departure;
		break;
	}
	ia5strlcpy_out(&proc_out->procedure, proc_in->name);
	if (proc_in->trans[0] != '\0') {
		proc_out->proceduretransition =
		    safe_calloc(1, sizeof (*proc_out->proceduretransition));
		ia5strlcpy_out(proc_out->proceduretransition, proc_in->trans);
	}
}

static void
encode_squawk_asn(unsigned bcn, Beaconcode_t *bcn_out)
{
	Beaconcodeoctaldigit_t *digits[4] = {
	    safe_calloc(1, sizeof (**digits)),
	    safe_calloc(1, sizeof (**digits)),
	    safe_calloc(1, sizeof (**digits)),
	    safe_calloc(1, sizeof (**digits))
	};

	CPDLC_ASSERT(bcn_out != NULL);

	*(digits[0]) = (bcn / 1000) % 10;
	*(digits[1]) = (bcn / 100) % 10;
	*(digits[2]) = (bcn / 10) % 10;
	*(digits[3]) = (bcn / 1) % 10;
	for (int i = 0; i < 4; i++)
		ASN_SEQUENCE_ADD(&bcn_out->list, digits[i]);
}

static void
encode_icao_id_asn(const char *icao_in, ICAOfacilitydesignation_t *icao_out)
{
	CPDLC_ASSERT(icao_in != NULL);
	CPDLC_ASSERT(icao_out != NULL);
	ia5strlcpy_out(icao_out, icao_in);
}

static void
encode_icao_name_asn(const cpdlc_icao_name_t *icao_in, ICAOunitname_t *icao_out)
{
	CPDLC_ASSERT(icao_in != NULL);
	CPDLC_ASSERT(icao_out != NULL);
	if (icao_in->is_name) {
		char shortened[19];

		icao_out->iCAOfacilityidentification.present =
		    ICAOfacilityidentification_PR_iCAOfacilityname;
		cpdlc_strlcpy(shortened, icao_in->name, sizeof (shortened));
		ia5strlcpy_out(&icao_out->iCAOfacilityidentification.choice.
		    iCAOfacilityname, shortened);
	} else {
		icao_out->iCAOfacilityidentification.present =
		    ICAOfacilityidentification_PR_iCAOfacilitydesignation;
		ia5strlcpy_out(&icao_out->iCAOfacilityidentification.choice.
		    iCAOfacilitydesignation, icao_in->icao_id);
	}
}

static void
encode_freq_asn(double freq, Frequency_t *freq_out)
{
	CPDLC_ASSERT(freq_out != NULL);

	if (freq <= 28) {
		freq_out->present = Frequency_PR_frequencyhf;
		freq_out->choice.frequencyhf = round(freq * 1000);
	} else if (freq <= 155) {
		freq_out->present = Frequency_PR_frequencyvhf;
		freq_out->choice.frequencyvhf = round(freq * 1000);
	} else {
		freq_out->present = Frequency_PR_frequencyuhf;
		freq_out->choice.frequencyuhf = round(freq * 1000);
	}
}

static void
encode_altimeter_asn(const cpdlc_altimeter_t *baro_in, Altimeter_t *baro_out)
{
	CPDLC_ASSERT(baro_in != NULL);
	CPDLC_ASSERT(baro_out != NULL);
	if (baro_in->hpa) {
		baro_out->present = Altimeter_PR_altimetermetric;
		baro_out->choice.altimetermetric = round(baro_in->val * 10);
	} else {
		baro_out->present = Altimeter_PR_altimeterenglish;
		baro_out->choice.altimeterenglish = round(baro_in->val * 100);
	}
}

static void
encode_persons_asn(unsigned pob, Remainingsouls_t *pob_out)
{
	CPDLC_ASSERT(pob_out != NULL);
	*pob_out = pob;
}

static void
encode_tp4table_asn(unsigned tp4, Tp4table_t *tp4_out)
{
	CPDLC_ASSERT(tp4_out != NULL);
	*tp4_out = tp4;
}

static void
encode_rwy_asn(const char *rwy_in, Runway_t *rwy_out)
{
	CPDLC_ASSERT(rwy_in != NULL);
	CPDLC_ASSERT(rwy_out != NULL);
	rwy_out->runwaydirection = atoi(rwy_in);
	switch (rwy_in[3]) {
	case 'L':
		rwy_out->runwayconfiguration = Runwayconfiguration_left;
		break;
	case 'R':
		rwy_out->runwayconfiguration = Runwayconfiguration_right;
		break;
	case 'C':
		rwy_out->runwayconfiguration = Runwayconfiguration_center;
		break;
	default:
		rwy_out->runwayconfiguration = Runwayconfiguration_none;
		break;
	}
}

static void
encode_pub_ident_asn(const cpdlc_pub_ident_t *id_in,
    Publishedidentifier_t *id_out)
{
	CPDLC_ASSERT(id_in != NULL);
	CPDLC_ASSERT(id_out != NULL);

	ia5strlcpy_out(&id_out->fixname, id_in->fixname);
	if (!CPDLC_IS_NULL_LAT_LON(id_in->lat_lon)) {
		id_out->latitudeLongitude =
		    safe_calloc(1, sizeof (*id_out->latitudeLongitude));
		latlon_cpdlc2asn(&id_in->lat_lon, id_out->latitudeLongitude);
	}
}

static void
encode_pb_asn(const cpdlc_pb_t *pb_in, Placebearing_t *pb_out)
{
	CPDLC_ASSERT(pb_in != NULL);
	CPDLC_ASSERT(pb_out != NULL);

	ia5strlcpy_out(&pb_out->fixname, pb_in->fixname);
	if (!CPDLC_IS_NULL_LAT_LON(pb_in->lat_lon)) {
		pb_out->latitudeLongitude =
		    safe_calloc(1, sizeof (*pb_out->latitudeLongitude));
		latlon_cpdlc2asn(&pb_in->lat_lon, pb_out->latitudeLongitude);
	}
	encode_degrees_asn(pb_in->degrees, false, &pb_out->degrees);
}

static void
encode_trk_detail_asn(const cpdlc_trk_detail_t *trk_in,
    Trackdetail_t *trk_out)
{
	CPDLC_ASSERT(trk_in != NULL);
	CPDLC_ASSERT(trk_out != NULL);

	ia5strlcpy_out(&trk_out->trackname, trk_in->name);
	for (unsigned i = 0; i < trk_in->num_lat_lon; i++) {
		LatitudeLongitude_t *ll_out = safe_calloc(1, sizeof (*ll_out));

		ASN_SEQUENCE_ADD(&trk_out->latitudeLongitude_seqOf, ll_out);
		latlon_cpdlc2asn(&trk_in->lat_lon[i], ll_out);
	}
}

static void
encode_route_info(const cpdlc_route_t *route,
    const cpdlc_route_info_t *info_in, Routeinformation_t *info_out)
{
	CPDLC_ASSERT(route != NULL);
	CPDLC_ASSERT(info_in != NULL);
	CPDLC_ASSERT(info_out != NULL);

	switch (info_in->type) {
	case CPDLC_ROUTE_PUB_IDENT:
		info_out->present = Routeinformation_PR_publishedidentifier;
		encode_pub_ident_asn(&info_in->pub_ident,
		    &info_out->choice.publishedidentifier);
		break;
	case CPDLC_ROUTE_LAT_LON:
		info_out->present = Routeinformation_PR_latitudeLongitude;
		latlon_cpdlc2asn(&info_in->lat_lon,
		    &info_out->choice.latitudeLongitude);
		break;
	case CPDLC_ROUTE_PBPB:
		info_out->present =
		    Routeinformation_PR_placebearingplacebearing;
		for (int i = 0; i < 2; i++) {
			Placebearing_t *pb = safe_calloc(1, sizeof (*pb));
			ASN_SEQUENCE_ADD(
			    &info_out->choice.placebearingplacebearing.list,
			    pb);
			encode_pb_asn(&info_in->pbpb[i], pb);
		}
		break;
	case CPDLC_ROUTE_PBD:
		info_out->present = Routeinformation_PR_placebearingdistance;
		encode_pbd_asn(&info_in->pbd,
		    &info_out->choice.placebearingdistance);
		break;
	case CPDLC_ROUTE_AWY:
		info_out->present = Routeinformation_PR_airwayidentifier;
		ia5strlcpy_out(&info_out->choice.airwayidentifier,
		    info_in->awy);
		break;
	case CPDLC_ROUTE_TRACK_DETAIL:
		info_out->present = Routeinformation_PR_trackdetail;
		encode_trk_detail_asn(&route->trk_detail,
		    &info_out->choice.trackdetail);
		break;
	case CPDLC_ROUTE_UNKNOWN:
		info_out->present = Routeinformation_PR_publishedidentifier;
		ia5strlcpy_out(&info_out->choice.publishedidentifier.fixname,
		    info_in->str);
		break;
	}
}

static void
encode_route_asn(const cpdlc_route_t *route_in, Routeclearance_t *route_out)
{
	CPDLC_ASSERT(route_in != NULL);
	CPDLC_ASSERT(route_out != NULL);

	if (route_in->orig_icao[0] != '\0') {
		route_out->airportdeparture =
		    safe_calloc(1, sizeof (*route_out->airportdeparture));
		ia5strlcpy_out(route_out->airportdeparture,
		    route_in->orig_icao);
	}
	if (route_in->dest_icao[0] != '\0') {
		route_out->airportdestination =
		    safe_calloc(1, sizeof (*route_out->airportdestination));
		ia5strlcpy_out(route_out->airportdestination,
		    route_in->dest_icao);
	}
	if (route_in->orig_rwy[0] != '\0') {
		route_out->runwaydeparture =
		    safe_calloc(1, sizeof (*route_out->runwaydeparture));
		encode_rwy_asn(route_in->orig_rwy, route_out->runwaydeparture);
	}
	if (route_in->dest_rwy[0] != '\0') {
		route_out->runwayarrival =
		    safe_calloc(1, sizeof (*route_out->runwayarrival));
		encode_rwy_asn(route_in->dest_rwy, route_out->runwayarrival);
	}
	if (route_in->sid.name[0] != '\0') {
		route_out->proceduredeparture =
		    safe_calloc(1, sizeof (*route_out->proceduredeparture));
		encode_proc_asn(&route_in->sid, route_out->proceduredeparture);
	}
	if (route_in->star.name[0] != '\0') {
		route_out->procedurearrival =
		    safe_calloc(1, sizeof (*route_out->procedurearrival));
		encode_proc_asn(&route_in->star, route_out->procedurearrival);
	}
	if (route_in->appch.name[0] != '\0') {
		route_out->procedureapproach =
		    safe_calloc(1, sizeof (*route_out->procedureapproach));
		encode_proc_asn(&route_in->appch, route_out->procedureapproach);
	}
	if (route_in->awy_intc[0] != '\0') {
		route_out->airwayintercept =
		    safe_calloc(1, sizeof (*route_out->airwayintercept));
		ia5strlcpy_out(route_out->airwayintercept, route_in->awy_intc);
	}
	if (route_in->num_info != 0) {
		route_out->routeinformation_seqOf =
		    safe_calloc(1, sizeof (*route_out->routeinformation_seqOf));
		for (unsigned i = 0; i < route_in->num_info; i++) {
			Routeinformation_t *info_out =
			    safe_calloc(1, sizeof (*info_out));

			ASN_SEQUENCE_ADD(route_out->routeinformation_seqOf,
			    info_out);
			encode_route_info(route_in, &route_in->info[i],
			    info_out);
		}
	}
}

static void
encode_temp_asn(int temp_in, Temperature_t *temp_out)
{
	CPDLC_ASSERT(temp_out != NULL);
	temp_out->present = Temperature_PR_temperaturec;
	temp_out->choice.temperaturec = temp_in;
}

static void
encode_winds_asn(const cpdlc_wind_t *wind_in, Winds_t *wind_out)
{
	CPDLC_ASSERT(wind_in != NULL);
	CPDLC_ASSERT(wind_out != NULL);
	wind_out->winddirection = wind_in->dir;
	wind_out->windspeed.present = Windspeed_PR_windspeedenglish;
	wind_out->windspeed.choice.windspeedenglish = wind_in->spd;
}

static void
encode_posreport_asn(const cpdlc_pos_rep_t *pos_rep_in,
    Positionreport_t *pos_rep_out)
{
	CPDLC_ASSERT(pos_rep_in != NULL);
	CPDLC_ASSERT(pos_rep_out != NULL);

	encode_pos_asn(&pos_rep_in->cur_pos, &pos_rep_out->positioncurrent);
	encode_time_asn(&pos_rep_in->time_cur_pos,
	    &pos_rep_out->timeatpositioncurrent);
	encode_alt_asn(&pos_rep_in->cur_alt, &pos_rep_out->altitude);
	if (!CPDLC_IS_NULL_POS(pos_rep_in->fix_next)) {
		pos_rep_out->fixnext =
		    safe_calloc(1, sizeof (*pos_rep_out->fixnext));
		encode_pos_asn(&pos_rep_in->fix_next, pos_rep_out->fixnext);
	}
	if (!CPDLC_IS_NULL_TIME(pos_rep_in->time_fix_next)) {
		pos_rep_out->timeetaatfixnext =
		    safe_calloc(1, sizeof (*pos_rep_out->timeetaatfixnext));
		encode_time_asn(&pos_rep_in->time_fix_next,
		    pos_rep_out->timeetaatfixnext);
	}
	if (!CPDLC_IS_NULL_POS(pos_rep_in->fix_next_p1)) {
		pos_rep_out->fixnextplusone =
		    safe_calloc(1, sizeof (*pos_rep_out->fixnextplusone));
		encode_pos_asn(&pos_rep_in->fix_next_p1,
		    pos_rep_out->fixnextplusone);
	}
	if (!CPDLC_IS_NULL_TIME(pos_rep_in->time_dest)) {
		pos_rep_out->timeetadestination =
		    safe_calloc(1, sizeof (*pos_rep_out->timeetadestination));
		encode_time_asn(&pos_rep_in->time_dest,
		    pos_rep_out->timeetadestination);
	}
	if (!CPDLC_IS_NULL_TIME(pos_rep_in->rmng_fuel)) {
		pos_rep_out->remainingfuel =
		    safe_calloc(1, sizeof (*pos_rep_out->remainingfuel));
		pos_rep_out->remainingfuel->timehours =
		    pos_rep_in->rmng_fuel.hrs;
		pos_rep_out->remainingfuel->timeminutes =
		    pos_rep_in->rmng_fuel.mins;
	}
	if (!CPDLC_IS_NULL_TEMP(pos_rep_in->temp)) {
		pos_rep_out->temperature =
		    safe_calloc(1, sizeof (*pos_rep_out->temperature));
		encode_temp_asn(pos_rep_in->temp, pos_rep_out->temperature);
	}
	if (!CPDLC_IS_NULL_WIND(pos_rep_in->wind)) {
		pos_rep_out->winds =
		    safe_calloc(1, sizeof (*pos_rep_out->winds));
		encode_winds_asn(&pos_rep_in->wind, pos_rep_out->winds);
	}
	if (pos_rep_in->turb != CPDLC_TURB_NONE) {
		pos_rep_out->turbulence =
		    safe_calloc(1, sizeof (*pos_rep_out->turbulence));
		*pos_rep_out->turbulence = pos_rep_in->turb - 1;
	}
	if (pos_rep_in->icing != CPDLC_ICING_NONE) {
		pos_rep_out->icing =
		    safe_calloc(1, sizeof (*pos_rep_out->icing));
		*pos_rep_out->icing = pos_rep_in->icing - 1;
	}
	if (!CPDLC_IS_NULL_SPD(pos_rep_in->spd)) {
		pos_rep_out->speed =
		    safe_calloc(1, sizeof (*pos_rep_out->speed));
		encode_spd_asn(&pos_rep_in->spd, pos_rep_out->speed);
	}
	if (!CPDLC_IS_NULL_SPD(pos_rep_in->spd_gnd) &&
	    pos_rep_in->spd_gnd.gnd) {
		pos_rep_out->speedground =
		    safe_calloc(1, sizeof (*pos_rep_out->speedground));
		*pos_rep_out->speedground =
		    round(pos_rep_in->spd_gnd.spd / 10.0);
	}
	if (pos_rep_in->vvi_set) {
		pos_rep_out->verticalchange =
		    safe_calloc(1, sizeof (*pos_rep_out->verticalchange));
		pos_rep_out->verticalchange->verticaldirection =
		    (pos_rep_in->vvi >= 0 ? Verticaldirection_up :
		    Verticaldirection_down);
		encode_vvi_asn(pos_rep_in->vvi,
		    &pos_rep_out->verticalchange->verticalrate);
	}
	if (pos_rep_in->trk != 0) {
		pos_rep_out->trackangle =
		    safe_calloc(1, sizeof (*pos_rep_out->trackangle));
		encode_degrees_asn(pos_rep_in->trk, false,
		    pos_rep_out->trackangle);
	}
	if (pos_rep_in->hdg_true != 0) {
		pos_rep_out->trueheading =
		    safe_calloc(1, sizeof (*pos_rep_out->trueheading));
		encode_degrees_asn(pos_rep_in->hdg_true, true,
		    pos_rep_out->trueheading);
	}
	if (pos_rep_in->dist_set) {
		pos_rep_out->distance =
		    safe_calloc(1, sizeof (*pos_rep_out->distance));
		encode_dist_asn(pos_rep_in->dist_nm, pos_rep_out->distance);
	}
	if (pos_rep_in->remarks[0] != '\0') {
		pos_rep_out->supplementaryinformation = safe_calloc(1,
		    sizeof (*pos_rep_out->supplementaryinformation));
		ia5strlcpy_out(pos_rep_out->supplementaryinformation,
		    pos_rep_in->remarks);
	}
	if (!CPDLC_IS_NULL_POS(pos_rep_in->rpt_wpt_pos)) {
		pos_rep_out->reportedwaypointposition = safe_calloc(1,
		    sizeof (*pos_rep_out->reportedwaypointposition));
		encode_pos_asn(&pos_rep_in->rpt_wpt_pos,
		    pos_rep_out->reportedwaypointposition);
	}
	if (!CPDLC_IS_NULL_TIME(pos_rep_in->rpt_wpt_time)) {
		pos_rep_out->reportedwaypointtime =
		    safe_calloc(1, sizeof (*pos_rep_out->reportedwaypointtime));
		encode_time_asn(&pos_rep_in->rpt_wpt_time,
		    pos_rep_out->reportedwaypointtime);
	}
	if (!CPDLC_IS_NULL_ALT(pos_rep_in->rpt_wpt_alt)) {
		pos_rep_out->reportedwaypointaltitude = safe_calloc(1,
		    sizeof (*pos_rep_out->reportedwaypointaltitude));
		encode_alt_asn(&pos_rep_in->rpt_wpt_alt,
		    pos_rep_out->reportedwaypointaltitude);
	}
}

static void
encode_acf_eqpt_code(const cpdlc_acf_eqpt_code_t *eqpt_in,
    Aircraftequipmentcode_t *eqpt_out)
{
	CPDLC_ASSERT(eqpt_in != NULL);
	CPDLC_ASSERT(eqpt_out != NULL);

	eqpt_out->cOMNAVapproachequipmentavailable =
	    eqpt_in->com_nav_app_eqpt_avail;
	for (unsigned i = 0; i < eqpt_in->num_com_nav_eqpt_st; i++) {
		COMNAVequipmentstatus_t *st_p = safe_calloc(1, sizeof (*st_p));
		*st_p = eqpt_in->com_nav_eqpt_st[i];
		SEQ_ADD(eqpt_out->cOMNAVequipmentstatus_seqOf, st_p);
	}
	eqpt_out->sSRequipmentavailable = eqpt_in->ssr_eqpt;
}

static void
encode_pdc_asn(const cpdlc_pdc_t *pdc_in, Predepartureclearance_t *pdc_out)
{
	CPDLC_ASSERT(pdc_in != NULL);
	CPDLC_ASSERT(pdc_out != NULL);

	ia5strlcpy_out(&pdc_out->aircraftflightidentification, pdc_in->acf_id);
	if (pdc_in->acf_type[0] != '\0') {
		pdc_out->aircrafttype =
		    safe_calloc(1, sizeof (*pdc_out->aircrafttype));
		ia5strlcpy_out(pdc_out->aircrafttype, pdc_in->acf_type);
	}
	if (pdc_in->acf_eqpt_code.com_nav_app_eqpt_avail ||
	    pdc_in->acf_eqpt_code.num_com_nav_eqpt_st != 0 ||
	    pdc_in->acf_eqpt_code.ssr_eqpt != CPDLC_SSR_EQPT_NIL) {
		pdc_out->aircraftequipmentcode =
		    safe_calloc(1, sizeof (*pdc_out->aircraftequipmentcode));
		encode_acf_eqpt_code(&pdc_in->acf_eqpt_code,
		    pdc_out->aircraftequipmentcode);
	}
	encode_time_asn(&pdc_in->time_dep, &pdc_out->timedepartureedct);
	encode_route_asn(&pdc_in->route, &pdc_out->routeclearance);
	if (!CPDLC_IS_NULL_ALT(pdc_in->alt_restr)) {
		pdc_out->altituderestriction =
		    safe_calloc(1, sizeof (*pdc_out->altituderestriction));
		encode_alt_asn(&pdc_in->alt_restr,
		    pdc_out->altituderestriction);
	}
	pdc_out->frequencydeparture = round(pdc_in->freq * 1000);
	encode_squawk_asn(pdc_in->squawk, &pdc_out->beaconcode);
	pdc_out->pDCrevision = pdc_in->revision;
}

static void
encode_errinfo_asn(cpdlc_errinfo_t errinfo_in, Errorinformation_t *errinfo_out)
{
	CPDLC_ASSERT(errinfo_out != NULL);
	*errinfo_out = errinfo_in;
}

static void
encode_version_asn(unsigned ver_in, Versionnumber_t *ver_out)
{
	CPDLC_ASSERT(ver_out != NULL);
	*ver_out = ver_in;
}

static void
encode_atis_code_asn(char code_in, Atiscode_t *code_out)
{
	char str[2] = { code_in };
	CPDLC_ASSERT(code_out != NULL);
	ia5strlcpy_out(code_out, str);
}

static void
encode_legtype_asn(cpdlc_legtype_t leg_in, Legtype_t *leg_out)
{
	CPDLC_ASSERT(leg_out != NULL);
	if (leg_in.is_time) {
		leg_out->present = Legtype_PR_legtime;
		leg_out->choice.legtime = round(leg_in.param * 10);
	} else {
		leg_out->present = Legtype_PR_legdistance;
		leg_out->choice.legdistance.present =
		    Legdistance_PR_legdistanceenglish;
		leg_out->choice.legdistance.choice.legdistanceenglish =
		    round(leg_in.param * 10);
	}
}

static void
msg_encode_seg_common(const cpdlc_msg_seg_t *seg, void *el)
{
	CPDLC_ASSERT(seg != NULL);
	CPDLC_ASSERT(seg->info != NULL);
	CPDLC_ASSERT(el != NULL);

	for (unsigned i = 0; i < seg->info->num_args; i++) {
		switch (seg->info->args[i]) {
		case CPDLC_ARG_ALTITUDE:
			encode_alt_asn(&seg->args[i].alt,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_SPEED:
			encode_spd_asn(&seg->args[i].spd,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_TIME:
		case CPDLC_ARG_TIME_DUR:
			encode_time_asn(&seg->args[i].time,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_POSITION:
			encode_pos_asn(&seg->args[i].pos,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_DIRECTION:
			encode_dir_asn(seg->args[i].dir,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_DISTANCE:
			encode_dist_asn(seg->args[i].dist,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_DISTANCE_OFFSET:
			encode_dist_off_asn(seg->args[i].dist,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_VVI:
			encode_vvi_asn(seg->args[i].vvi,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_TOFROM:
			encode_tofrom_asn(seg->args[i].tofrom,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_ROUTE:
			CPDLC_ASSERT(seg->args[i].route != NULL);
			encode_route_asn(seg->args[i].route,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_PROCEDURE:
			encode_proc_asn(&seg->args[i].proc,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_SQUAWK:
			encode_squawk_asn(seg->args[i].squawk,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_ICAO_ID:
			encode_icao_id_asn(seg->args[i].icao_id,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_ICAO_NAME:
			encode_icao_name_asn(&seg->args[i].icao_name,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_FREQUENCY:
			encode_freq_asn(seg->args[i].freq,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_DEGREES:
			encode_degrees_asn(seg->args[i].deg.deg,
			    seg->args[i].deg.tru,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_BARO:
			encode_altimeter_asn(&seg->args[i].baro,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_FREETEXT:
			CPDLC_ASSERT(seg->args[i].freetext != NULL);
			ia5strlcpy_out(get_asn_arg_ptr_wr(seg->info, i, el),
			    seg->args[i].freetext);
			break;
		case CPDLC_ARG_PERSONS:
			encode_persons_asn(seg->args[i].pob,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_POSREPORT:
			encode_posreport_asn(&seg->args[i].pos_rep,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_PDC:
			CPDLC_ASSERT(seg->args[i].pdc != NULL);
			encode_pdc_asn(seg->args[i].pdc,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_TP4TABLE:
			encode_tp4table_asn(seg->args[i].tp4,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_ERRINFO:
			encode_errinfo_asn(seg->args[i].errinfo,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_VERSION:
			encode_version_asn(seg->args[i].version,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_ATIS_CODE:
			encode_atis_code_asn(seg->args[i].atis_code,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		case CPDLC_ARG_LEGTYPE:
			encode_legtype_asn(seg->args[i].legtype,
			    get_asn_arg_ptr_wr(seg->info, i, el));
			break;
		}
	}
}

static void
msg_encode_seg_dl(const cpdlc_msg_seg_t *seg, ATCdownlinkmsgelementid_t *el)
{
	CPDLC_ASSERT(seg != NULL);
	CPDLC_ASSERT(el != NULL);
	el->present = seg->info->asn_elem_id;
	msg_encode_seg_common(seg, el);
}

static void
msg_encode_seg_ul(const cpdlc_msg_seg_t *seg, ATCuplinkmsgelementid_t *el)
{
	CPDLC_ASSERT(seg != NULL);
	CPDLC_ASSERT(el != NULL);
	el->present = seg->info->asn_elem_id;
	msg_encode_seg_common(seg, el);
}

static void
msg_encode_asn_dl(const cpdlc_msg_t *msg_in, ATCdownlinkmessage_t *msg_out)
{
	CPDLC_ASSERT(msg_in != NULL);
	CPDLC_ASSERT(msg_out != NULL);

	msg_encode_hdr(msg_in, &msg_out->aTCmessageheader);

	msg_encode_seg_dl(&msg_in->segs[0], &msg_out->aTCdownlinkmsgelementid);
	for (unsigned i = 1; i < msg_in->num_segs; i++) {
		ATCdownlinkmsgelementid_t *el = safe_calloc(1, sizeof (*el));
		msg_encode_seg_dl(&msg_in->segs[i], el);
		SEQ_ADD(msg_out->aTCdownlinkmsgelementid_seqOf, el);
	}
}

static void
msg_encode_asn_ul(const cpdlc_msg_t *msg_in, ATCuplinkmessage_t *msg_out)
{
	CPDLC_ASSERT(msg_in != NULL);
	CPDLC_ASSERT(msg_out != NULL);

	msg_encode_hdr(msg_in, &msg_out->aTCmessageheader);

	msg_encode_seg_ul(&msg_in->segs[0], &msg_out->aTCuplinkmsgelementid);
	for (unsigned i = 1; i < msg_in->num_segs; i++) {
		ATCuplinkmsgelementid_t *el = safe_calloc(1, sizeof (*el));
		msg_encode_seg_ul(&msg_in->segs[i], el);
		SEQ_ADD(msg_out->aTCuplinkmsgelementid_seqOf, el);
	}
}

static const char *
imi2str(const cpdlc_imi_t imi)
{
	switch (imi) {
	case CPDLC_IMI_DATA:
		return ("AT1");
	case CPDLC_IMI_CONN_REQUEST:
		return ("CR1");
	case CPDLC_IMI_CONN_CONFIRM:
		return ("CC1");
	case CPDLC_IMI_DISC_REQUEST:
		return ("DR1");
	}
	CPDLC_VERIFY(0);
}

bool
cpdlc_msg_encode_arinc622(const cpdlc_msg_t *msg, unsigned *n_bytes_p,
    char **buf_p, unsigned *cap_p)
{
	asn_TYPE_descriptor_t *td;
	void *struct_ptr, *rawbuf;
	uint8_t *crc_inbuf;
	char *hexbuf;
	ssize_t sz;
	const char *cs;
	char cs_padd[8];
	uint16_t crc;

	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(n_bytes_p != NULL);
	CPDLC_ASSERT(buf_p != NULL);
	CPDLC_ASSERT(cap_p != NULL);

	if (msg->num_segs == 0)
		return (false);
	if (msg->segs[0].info->is_dl) {
		td = &asn_DEF_ATCdownlinkmessage;
		struct_ptr = safe_calloc(1, sizeof (ATCdownlinkmessage_t));
		msg_encode_asn_dl(msg, struct_ptr);
	} else {
		td = &asn_DEF_ATCuplinkmessage;
		struct_ptr = safe_calloc(1, sizeof (ATCuplinkmessage_t));
		msg_encode_asn_ul(msg, struct_ptr);
	}
	sz = uper_encode_to_new_buffer(td, NULL, struct_ptr, &rawbuf);
	if (sz < 0)
		return (false);
	crc_inbuf = safe_malloc(CPDLC_DATA_OFF + sz);
	cs = (msg->segs[0].info->is_dl ? msg->from : msg->to);
	cpdlc_padd_callsign(cs, cs_padd);
	snprintf((char *)crc_inbuf, CPDLC_DATA_OFF + sz, "%s%s",
	    imi2str(msg->arinc622.imi), cs_padd);
	memcpy(&crc_inbuf[CPDLC_DATA_OFF], rawbuf, sz);
	crc = cpdlc_crc16(crc_inbuf, CPDLC_DATA_OFF + sz);
	free(crc_inbuf);

	hexbuf = safe_calloc(1, 2 * sz + 1);
	cpdlc_hex_enc(rawbuf, sz, hexbuf, 2 * sz + 1);
	APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "/ARINC622=%s%s%s%04x",
	    imi2str(msg->arinc622.imi), cs_padd, hexbuf, crc);
	free(hexbuf);
	free(rawbuf);
	ASN_STRUCT_FREE(*td, struct_ptr);

	return (true);
}

static cpdlc_alt_t
decode_alt_asn(const Altitude_t *alt_in)
{
	if (alt_in == NULL)
		return (CPDLC_NULL_ALT);
	switch (alt_in->present) {
	default:
		return (CPDLC_NULL_ALT);
	case Altitude_PR_altitudeqnh:
		return ((cpdlc_alt_t){.alt = alt_in->choice.altitudeqnh * 10});
	case Altitude_PR_altitudeqnhmeters:
		return ((cpdlc_alt_t){
		    .met = true,
		    .alt = round(MET2FEET(alt_in->choice.altitudeqnhmeters))
		});
	case Altitude_PR_altitudeqfe:
	case Altitude_PR_altitudeqfemeters:
		return (CPDLC_NULL_ALT);
	case Altitude_PR_altitudegnssfeet:
		return ((cpdlc_alt_t){.alt = alt_in->choice.altitudegnssfeet});
	case Altitude_PR_altitudegnssmeters:
		return ((cpdlc_alt_t){
		    .met = true,
		    .alt = round(MET2FEET(alt_in->choice.altitudegnssmeters))
		});
	case Altitude_PR_altitudeflightlevel:
		return ((cpdlc_alt_t){
		    .fl = true,
		    .alt = alt_in->choice.altitudeflightlevel * 100
		});
	case Altitude_PR_altitudeflightlevelmetric:
		return ((cpdlc_alt_t){
		    .fl = true,
		    .met = true,
		    .alt = round(MET2FEET(
		    alt_in->choice.altitudeflightlevelmetric * 10))
		});
	}
}

static cpdlc_spd_t
decode_spd_asn(const Speed_t *spd_in)
{
	if (spd_in == NULL)
		return (CPDLC_NULL_SPD);
	switch (spd_in->present) {
	default:
		return (CPDLC_NULL_SPD);
	case Speed_PR_speedindicated:
		return ((cpdlc_spd_t){
		    .spd = spd_in->choice.speedindicated * 10
		});
	case Speed_PR_speedindicatedmetric:
		return ((cpdlc_spd_t){
		    .spd = round((spd_in->choice.speedindicatedmetric *
		    10) / 1.852)
		});
	case Speed_PR_speedtrue:
		return ((cpdlc_spd_t){
		    .tru = true,
		    .spd = spd_in->choice.speedtrue * 10
		});
	case Speed_PR_speedtruemetric:
		return ((cpdlc_spd_t){
		    .tru = true,
		    .spd = round((spd_in->choice.speedtruemetric *
		    10) / 1.852)
		});
	case Speed_PR_speedground:
		return ((cpdlc_spd_t){
		    .gnd = true,
		    .spd = spd_in->choice.speedground * 10
		});
	case Speed_PR_speedgroundmetric:
		return ((cpdlc_spd_t){
		    .gnd = true,
		    .spd = round((spd_in->choice.speedgroundmetric *
		    10) / 1.852)
		});
	case Speed_PR_speedmach:
	case Speed_PR_speedmachlarge:
		return ((cpdlc_spd_t){
		    .mach = true,
		    .spd = spd_in->choice.speedmach * 10
		});
	}
}

static cpdlc_time_t
decode_time_asn(const Time_t *time_in)
{
	if (time_in != NULL) {
		return ((cpdlc_time_t){ time_in->timehours,
		    time_in->timeminutes });
	} else {
		return (CPDLC_NULL_TIME);
	}
}

static cpdlc_time_t
decode_rmng_fuel_asn(const Remainingfuel_t *fuel_in)
{
	if (fuel_in != NULL) {
		return ((cpdlc_time_t){ fuel_in->timehours,
		    fuel_in->timeminutes });
	} else {
		return (CPDLC_NULL_TIME);
	}
}

static int
decode_temp_asn(const Temperature_t *temp_in)
{
	if (temp_in != NULL) {
		switch (temp_in->present) {
		default:
			return (CPDLC_NULL_TEMP);
		case Temperature_PR_temperaturec:
			return (temp_in->choice.temperaturec);
		case Temperature_PR_temperaturef:
			return (FAH2C(temp_in->choice.temperaturef));
		}
	} else {
		return (CPDLC_NULL_TEMP);
	}
}

static cpdlc_wind_t
decode_winds_asn(const Winds_t *wind_in)
{
	if (wind_in == NULL)
		return (CPDLC_NULL_WIND);
	switch (wind_in->windspeed.present) {
	default:
		return (CPDLC_NULL_WIND);
	case Windspeed_PR_windspeedenglish:
		return ((cpdlc_wind_t){ wind_in->winddirection,
		    wind_in->windspeed.choice.windspeedenglish });
	case Windspeed_PR_windspeedmetric:
		return ((cpdlc_wind_t){ wind_in->winddirection,
		    round(wind_in->windspeed.choice.windspeedmetric * 1.852) });
	}
}

static double
decode_vvi_asn(const Verticalrate_t *vvi)
{
	CPDLC_ASSERT(vvi != NULL);
	switch (vvi->present) {
	default:
		return (0);
	case Verticalrate_PR_verticalrateenglish:
		return (vvi->choice.verticalrateenglish * 100);
	case Verticalrate_PR_verticalratemetric:
		return (MET2FEET(vvi->choice.verticalratemetric * 10));
	}
}

static int
decode_vvi_chg_asn(const Verticalchange_t *chg, bool *vvi_set)
{
	CPDLC_ASSERT(vvi_set != NULL);
	if (chg == NULL) {
		*vvi_set = false;
		return (0);
	}
	*vvi_set = true;
	return ((chg->verticaldirection == Verticaldirection_up ? 1 : -1) *
	    decode_vvi_asn(&chg->verticalrate));
}

static cpdlc_lat_lon_t
latlon_asn2cpdlc(const LatitudeLongitude_t *latlon)
{
	Latitude_t lat;
	Longitude_t lon;
	cpdlc_lat_lon_t ll;
	int dir;
	double mins;

	if (latlon == NULL)
		return (CPDLC_NULL_LAT_LON);
	lat = latlon->latitude;
	lon = latlon->longitude;

	dir = (lat.latitudedirection == Latitudedirection_north ? 1 : -1);
	mins = (lat.minuteslatlon != NULL ? (*lat.minuteslatlon) / 10.0 : 0);
	ll.lat = dir * (lat.latitudedegrees + mins / 60.0);

	dir = (lon.longitudedirection == Longitudedirection_east ? 1 : -1);
	mins = (lon.minuteslatlon != NULL ? (*lon.minuteslatlon) / 10.0 : 0);
	ll.lon = dir * (lon.longitudedegrees + mins / 60.0);

	return (ll);
}

static unsigned
decode_deg_asn(const Degrees_t *deg, bool *tru)
{
	if (deg == NULL)
		return (0);
	switch (deg->present) {
	default:
		return (0);
	case Degrees_PR_degreesmagnetic:
		if (tru != NULL)
			*tru = false;
		return (deg->choice.degreesmagnetic);
	case Degrees_PR_degreestrue:
		if (tru != NULL)
			*tru = true;
		return (deg->choice.degreestrue);
	}
}

static double
decode_dist_asn(const Distance_t *dist, bool *set)
{
	if (dist == NULL) {
		if (set != NULL)
			*set = false;
		return (0);
	}
	switch (dist->present) {
	default:
		if (set != NULL)
			*set = false;
		return (0);
	case Distance_PR_distancenm:
		if (set != NULL)
			*set = true;
		return (dist->choice.distancenm / 10.0);
	case Distance_PR_distancekm:
		if (set != NULL)
			*set = true;
		return (dist->choice.distancekm / 1.852);
	}
}

static double
decode_dist_off_asn(const Distanceoffset_t *dist_off)
{
	CPDLC_ASSERT(dist_off != NULL);
	switch (dist_off->present) {
	default:
		return (0);
	case Distanceoffset_PR_distanceoffsetnm:
		return (dist_off->choice.distanceoffsetnm);
	case Distanceoffset_PR_distanceoffsetkm:
		return (dist_off->choice.distanceoffsetkm / 1.852);
	}
}

static cpdlc_pos_t
decode_pos_asn(const Position_t *pos_in)
{
	cpdlc_pos_t pos_out = {};
	if (pos_in == NULL)
		return (CPDLC_NULL_POS);
	switch (pos_in->present) {
	default:
		pos_out = CPDLC_NULL_POS;
		break;
	case Position_PR_fixname:
		pos_out.set = true;
		pos_out.type = CPDLC_POS_FIXNAME;
		ia5strlcpy_in(pos_out.fixname, &pos_in->choice.fixname,
		    sizeof (pos_out.fixname));
		break;
	case Position_PR_navaid:
		pos_out.set = true;
		pos_out.type = CPDLC_POS_NAVAID;
		ia5strlcpy_in(pos_out.navaid, &pos_in->choice.navaid,
		    sizeof (pos_out.navaid));
		break;
	case Position_PR_airport:
		pos_out.set = true;
		pos_out.type = CPDLC_POS_AIRPORT;
		ia5strlcpy_in(pos_out.airport, &pos_in->choice.airport,
		    sizeof (pos_out.airport));
		break;
	case Position_PR_latitudeLongitude:
		pos_out.set = true;
		pos_out.type = CPDLC_POS_LAT_LON;
		pos_out.lat_lon = latlon_asn2cpdlc(
		    &pos_in->choice.latitudeLongitude);
		break;
	case Position_PR_placebearingdistance:
		pos_out.set = true;
		pos_out.type = CPDLC_POS_PBD;
		ia5strlcpy_in(pos_out.pbd.fixname,
		    &pos_in->choice.placebearingdistance.fixname,
		    sizeof (pos_out.pbd.fixname));
		pos_out.pbd.lat_lon = latlon_asn2cpdlc(pos_in->
		    choice.placebearingdistance.latitudelongitude);
		pos_out.pbd.degrees = decode_deg_asn(
		    &pos_in->choice.placebearingdistance.degrees, NULL);
		pos_out.pbd.dist_nm = decode_dist_asn(
		    &pos_in->choice.placebearingdistance.distance, NULL);
		break;
	}
	return (pos_out);
}

static cpdlc_dir_t
decode_dir_asn(const Direction_t *dir_in)
{
	if (dir_in == NULL)
		return (CPDLC_DIR_EITHER);
	switch (*dir_in) {
	case Direction_left:
		return (CPDLC_DIR_LEFT);
	case Direction_right:
		return (CPDLC_DIR_RIGHT);
	case Direction_eitherSide:
		return (CPDLC_DIR_EITHER);
	case Direction_north:
		return (CPDLC_DIR_NORTH);
	case Direction_south:
		return (CPDLC_DIR_SOUTH);
	case Direction_east:
		return (CPDLC_DIR_EAST);
	case Direction_west:
		return (CPDLC_DIR_WEST);
	case Direction_northEast:
		return (CPDLC_DIR_NE);
	case Direction_northWest:
		return (CPDLC_DIR_NW);
	case Direction_southEast:
		return (CPDLC_DIR_SE);
	case Direction_southWest:
		return (CPDLC_DIR_SW);
	}
	return (CPDLC_DIR_EITHER);
}

static double
decode_freq_asn(const Frequency_t *freq)
{
	CPDLC_ASSERT(freq != NULL);
	switch (freq->present) {
	default:
		return (0);
	case Frequency_PR_frequencyhf:
		return (freq->choice.frequencyhf / 1000.0);
	case Frequency_PR_frequencyvhf:
		return (freq->choice.frequencyvhf / 1000.0);
	case Frequency_PR_frequencyuhf:
		return (freq->choice.frequencyuhf / 1000.0);
	}
}

static bool
decode_tofrom_asn(const Tofrom_t *tofrom)
{
	CPDLC_ASSERT(tofrom != NULL);
	return (*tofrom == Tofrom_to);
}

static void
decode_proc_asn(const Procedurename_t *proc_in, cpdlc_proc_t *proc_out)
{
	CPDLC_ASSERT(proc_in != NULL);
	CPDLC_ASSERT(proc_out != NULL);
	/* maps Proceduretype_t to our cpdlc_proc_type_t */
	proc_out->type = proc_in->proceduretype + 1;
	ia5strlcpy_in(proc_out->name, &proc_in->procedure,
	    sizeof (proc_out->name));
	if (proc_in->proceduretransition != NULL) {
		ia5strlcpy_in(proc_out->trans, proc_in->proceduretransition,
		    sizeof (proc_out->trans));
	}
}

static unsigned
decode_squawk_asn(const Beaconcode_t *squawk)
{
	unsigned code = 0;
	CPDLC_ASSERT(squawk != NULL);
	for (int i = 0; i < squawk->list.count; i++) {
		if (squawk->list.array[i] != NULL)
			code = (code * 10) + (*squawk->list.array[i]);
	}
	return (code);
}

static void
decode_baro_asn(const Altimeter_t *baro, double *val_out, bool *hpa_out)
{
	CPDLC_ASSERT(baro != NULL);
	CPDLC_ASSERT(val_out != NULL);
	CPDLC_ASSERT(hpa_out != NULL);
	switch (baro->present) {
	default:
		break;
	case Altimeter_PR_altimeterenglish:
		*val_out = baro->choice.altimeterenglish / 100.0;
		break;
	case Altimeter_PR_altimetermetric:
		*hpa_out = true;
		*val_out = baro->choice.altimeterenglish / 10.0;
		break;
	}
}

static char *
decode_freetext_asn(const Freetext_t *freetext_in)
{
	char *freetext_out;
	CPDLC_ASSERT(freetext_in != NULL);
	if (freetext_in->size > 256)
		return (NULL);
	freetext_out = safe_malloc(freetext_in->size + 1);
	ia5strlcpy_in(freetext_out, freetext_in, freetext_in->size + 1);
	return (freetext_out);
}

static unsigned
decode_persons_asn(const Remainingsouls_t *pob)
{
	CPDLC_ASSERT(pob != NULL);
	return (*pob);
}

static cpdlc_tp4table_t
decode_tp4table_asn(const Tp4table_t *tp4_in)
{
	CPDLC_ASSERT(tp4_in != NULL);
	return (*tp4_in);
}

static bool
decode_pbpb_asn(const Placebearingplacebearing_t *pbpb_in,
    cpdlc_pb_t pbpb_out[2])
{
	CPDLC_ASSERT(pbpb_in != NULL);
	CPDLC_ASSERT(pbpb_out != NULL);

	for (int i = 0; i < 2; i++) {
		const Placebearing_t *pb_in;
		if (i >= pbpb_in->list.count || pbpb_in->list.array[i] == NULL)
			return (false);
		pb_in = pbpb_in->list.array[i];
		ia5strlcpy_in(pbpb_out[i].fixname, &pb_in->fixname,
		    sizeof (pbpb_out[i].fixname));
		pbpb_out[i].degrees = decode_deg_asn(&pb_in->degrees, NULL);
		pbpb_out[i].lat_lon =
		    latlon_asn2cpdlc(pb_in->latitudeLongitude);
	}
	return (true);
}

static void
decode_pbd_asn(const Placebearingdistance_t *pbd_in, cpdlc_pbd_t *pbd_out)
{
	CPDLC_ASSERT(pbd_in != NULL);
	CPDLC_ASSERT(pbd_out != NULL);

	ia5strlcpy_in(pbd_out->fixname, &pbd_in->fixname,
	    sizeof (pbd_out->fixname));
	pbd_out->degrees = decode_deg_asn(&pbd_in->degrees, NULL);
	pbd_out->dist_nm = decode_dist_asn(&pbd_in->distance, NULL);
	pbd_out->lat_lon = latlon_asn2cpdlc(pbd_in->latitudelongitude);
}

static void
decode_track_detail_asn(const Trackdetail_t *trk_in,
    cpdlc_trk_detail_t *trk_out)
{
	CPDLC_ASSERT(trk_in != NULL);
	CPDLC_ASSERT(trk_out != NULL);

	ia5strlcpy_in(trk_out->name, &trk_in->trackname, sizeof (trk_out->name));
	for (int i = 0; i < trk_in->latitudeLongitude_seqOf.list.count; i++) {
		if (trk_in->latitudeLongitude_seqOf.list.array[i] == NULL ||
		    trk_out->num_lat_lon >= CPDLC_TRK_DETAIL_MAX_LAT_LON) {
			continue;
		}
		trk_out->lat_lon[trk_out->num_lat_lon] = latlon_asn2cpdlc(
		    trk_in->latitudeLongitude_seqOf.list.array[i]);
		trk_out->num_lat_lon++;
	}
}

static void
decode_pub_ident_asn(const Publishedidentifier_t *id_in,
    cpdlc_pub_ident_t *id_out)
{
	CPDLC_ASSERT(id_in != NULL);
	CPDLC_ASSERT(id_out != NULL);

	ia5strlcpy_in(id_out->fixname, &id_in->fixname, sizeof (id_out->fixname));
	id_out->lat_lon = latlon_asn2cpdlc(id_in->latitudeLongitude);
}

static bool
decode_route_info(const struct Routeclearance__routeinformation_seqOf *seq,
    cpdlc_route_t *route)
{
	CPDLC_ASSERT(route != NULL);
	CPDLC_ASSERT(seq != NULL);
	CPDLC_ASSERT(route != NULL);

	for (int i = 0; i < seq->list.count; i++) {
		const Routeinformation_t *info_in = seq->list.array[i];
		cpdlc_route_info_t *info_out =
		    &route->info[route->num_info];

		if (info_in == NULL || route->num_info >= CPDLC_ROUTE_MAX_INFO)
			continue;
		switch (info_in->present) {
		default:
			continue;
		case Routeinformation_PR_publishedidentifier:
			info_out->type = CPDLC_ROUTE_PUB_IDENT;
			decode_pub_ident_asn(
			    &info_in->choice.publishedidentifier,
			    &info_out->pub_ident);
			break;
		case Routeinformation_PR_latitudeLongitude:
			info_out->type = CPDLC_ROUTE_LAT_LON;
			info_out->lat_lon = latlon_asn2cpdlc(
			    &info_in->choice.latitudeLongitude);
			break;
		case Routeinformation_PR_placebearingplacebearing:
			info_out->type = CPDLC_ROUTE_PBPB;
			if (!decode_pbpb_asn(
			    &info_in->choice.placebearingplacebearing,
			    info_out->pbpb)) {
				return (false);
			}
			break;
		case Routeinformation_PR_placebearingdistance:
			info_out->type = CPDLC_ROUTE_PBD;
			decode_pbd_asn(&info_in->choice.placebearingdistance,
			    &info_out->pbd);
			break;
		case Routeinformation_PR_airwayidentifier:
			info_out->type = CPDLC_ROUTE_AWY;
			ia5strlcpy_in(info_out->awy,
			    &info_in->choice.airwayidentifier,
			    sizeof (info_out->awy));
			break;
		case Routeinformation_PR_trackdetail:
			decode_track_detail_asn(&info_in->choice.trackdetail,
			    &route->trk_detail);
			break;
		}
		route->num_info++;
	}
	return (true);
}

static cpdlc_alt_cstr_t
decode_alt_cstr(const ATWaltitude_t *cstr_in)
{
	if (cstr_in != NULL)
		return ((cpdlc_alt_cstr_t){ .alt = CPDLC_NULL_ALT });
	switch (cstr_in->aTWaltitudetolerance) {
	default:
		return ((cpdlc_alt_cstr_t){
		    .toler = CPDLC_AT,
		    .alt = decode_alt_asn(&cstr_in->altitude)
		});
	case ATWaltitudetolerance_atorabove:
		return ((cpdlc_alt_cstr_t){
		    .toler = CPDLC_AT_OR_ABV,
		    .alt = decode_alt_asn(&cstr_in->altitude)
		});
	case ATWaltitudetolerance_atorbelow:
		return ((cpdlc_alt_cstr_t){
		    .toler = CPDLC_AT_OR_BLW,
		    .alt = decode_alt_asn(&cstr_in->altitude)
		});
	}
}

static void
decode_atw_seq_asn(const ATWalongtrackwaypointsequence_t *seq,
    cpdlc_route_add_info_t *add_info_out)
{
	CPDLC_ASSERT(seq != NULL);
	CPDLC_ASSERT(add_info_out != NULL);

	for (int i = 0; i < seq->list.count && add_info_out->num_atk_wpt < 8;
	    i++) {
		const ATWalongtrackwaypoint_t *atk_in = seq->list.array[i];
		cpdlc_atk_wpt_t *atk_out =
		    &add_info_out->atk_wpt[add_info_out->num_atk_wpt];

		if (atk_in == NULL)
			continue;
		atk_out->pos = decode_pos_asn(&atk_in->position);
		if (atk_in->speed != NULL) {
			atk_out->spd_cstr_present = true;
			atk_out->spd_cstr = decode_spd_asn(atk_in->speed);
		}
		for (int i = 0; atk_in->aTWaltitudesequence != NULL &&
		    i < atk_in->aTWaltitudesequence->list.count &&
		    atk_out->num_alt_cstr < 2; i++) {
			if (atk_in->aTWaltitudesequence->list.array[i] == NULL)
				continue;
			atk_out->alt_cstr[atk_out->num_alt_cstr] =
			    decode_alt_cstr(
			    atk_in->aTWaltitudesequence->list.array[i]);
			atk_out->num_alt_cstr++;
		}
		add_info_out->num_atk_wpt++;
	}
}

static void
decode_rpt_pts_asn(const Reportingpoints_t *rpt_pts_in,
    cpdlc_rpt_pts_t *rpt_pts_out)
{
	const Latitudereportingpoints_t *lat;
	const Longitudereportingpoints_t *lon;

	CPDLC_ASSERT(rpt_pts_in != NULL);
	CPDLC_ASSERT(rpt_pts_out != NULL);

	switch (rpt_pts_in->latlonreportingpoints.present) {
	default:
		break;
	case Latlonreportingpoints_PR_latitudereportingpoints:
		rpt_pts_out->rpt_lat = true;
		lat = &rpt_pts_in->latlonreportingpoints.choice.
		    latitudereportingpoints;
		rpt_pts_out->degrees = (lat->latitudedirection ==
		    Latitudedirection_north ? 1 : -1) * lat->latitudedegrees;
		break;
	case Latlonreportingpoints_PR_longitudereportingpoints:
		rpt_pts_out->rpt_lat = false;
		lon = &rpt_pts_in->latlonreportingpoints.choice.
		    longitudereportingpoints;
		rpt_pts_out->degrees = (lon->longitudedirection ==
		    Longitudedirection_east ? 1 : -1) * lon->longitudedegrees;
		break;
	}
}

static bool
decode_intc_from_asn(const Interceptcoursefromsequence_t *seq,
    cpdlc_route_add_info_t *add_info)
{
	CPDLC_ASSERT(seq != NULL);
	CPDLC_ASSERT(add_info != NULL);

	for (int i = 0; i < seq->list.count && add_info->num_intc_from < 4;
	    i++) {
		const Interceptcoursefromselection_t *sel =
		    &seq->list.array[i]->interceptcoursefromselection;
		cpdlc_intc_from_t *intc_from_out =
		    &add_info->intc_from[add_info->num_intc_from];

		if (sel == NULL)
			continue;
		switch (sel->present) {
		default:
			continue;
		case Interceptcoursefromselection_PR_publishedidentifier:
			intc_from_out->type = CPDLC_INTC_FROM_PUB_IDENT;
			decode_pub_ident_asn(
			    &sel->choice.publishedidentifier,
			    &intc_from_out->pub_ident);
			break;
		case Interceptcoursefromselection_PR_latitudeLongitude:
			intc_from_out->type = CPDLC_INTC_FROM_LAT_LON;
			intc_from_out->lat_lon = latlon_asn2cpdlc(
			    &sel->choice.latitudeLongitude);
			break;
		case Interceptcoursefromselection_PR_placebearingplacebearing:
			intc_from_out->type = CPDLC_INTC_FROM_PBPB;
			if (!decode_pbpb_asn(
			    &sel->choice.placebearingplacebearing,
			    intc_from_out->pbpb)) {
				return (false);
			}
			break;
		case Interceptcoursefromselection_PR_placebearingdistance:
			intc_from_out->type = CPDLC_INTC_FROM_PBD;
			decode_pbd_asn(&sel->choice.placebearingdistance,
			    &intc_from_out->pbd);
			break;
		}
		add_info->num_intc_from++;
	}
	return (true);
}

static cpdlc_hold_leg_t
decode_leg_asn(const Legtype_t *leg)
{
	if (leg == NULL)
		return ((cpdlc_hold_leg_t){});
	switch (leg->present) {
	default:
		return ((cpdlc_hold_leg_t){});
	case Legtype_PR_legdistance:
		switch (leg->choice.legdistance.present) {
		default:
			return ((cpdlc_hold_leg_t){});
		case Legdistance_PR_legdistanceenglish:
			return ((cpdlc_hold_leg_t){
			    .type = CPDLC_HOLD_LEG_DIST,
			    .dist_nm = leg->choice.legdistance.
			    choice.legdistanceenglish / 10.0
			});
		case Legdistance_PR_legdistancemetric:
			return ((cpdlc_hold_leg_t){
			    .type = CPDLC_HOLD_LEG_DIST,
			    .dist_nm = leg->choice.legdistance.
			    choice.legdistancemetric / 1.852
			});
		}
	case Legtype_PR_legtime:
		return ((cpdlc_hold_leg_t){
		    .type = CPDLC_HOLD_LEG_TIME,
		    .time_min = leg->choice.legtime / 10.0
		});
	}
}

static bool
decode_hold_at_asn(const Holdatwaypointsequence_t *seq,
    cpdlc_route_add_info_t *add_info)
{
	CPDLC_ASSERT(seq != NULL);
	CPDLC_ASSERT(add_info != NULL);

	for (int i = 0; i < seq->list.count && add_info->num_hold_at_wpt < 4;
	    i++) {
		const Holdatwaypoint_t *hold_in = seq->list.array[i];
		cpdlc_hold_at_t *hold_out =
		    &add_info->hold_at_wpt[add_info->num_hold_at_wpt];

		if (hold_in == NULL)
			continue;
		hold_out->pos = decode_pos_asn(&hold_in->position);
		hold_out->spd_low =
		    decode_spd_asn(hold_in->holdatwaypointspeedlow);
		if (hold_in->aTWaltitude != NULL) {
			hold_out->alt_present = true;
			hold_out->alt =
			    decode_alt_cstr(hold_in->aTWaltitude);
		}
		hold_out->spd_high =
		    decode_spd_asn(hold_in->holdatwaypointspeedhigh);
		hold_out->dir = decode_dir_asn(hold_in->direction);
		hold_out->degrees = decode_deg_asn(hold_in->degrees, NULL);
		hold_out->efc = decode_time_asn(hold_in->eFCtime);
		hold_out->leg = decode_leg_asn(hold_in->legtype);
	}
	return (true);
}

static void
decode_wpt_spd_alt_asn(const Waypointspeedaltitudesequence_t *seq,
    cpdlc_route_add_info_t *add_info)
{
	CPDLC_ASSERT(seq != NULL);
	CPDLC_ASSERT(add_info != NULL);

	for (int i = 0; i < seq->list.count && add_info->num_wpt_spd_alt < 32;
	    i++) {
		const Waypointspeedaltitude_t *wsa_in = seq->list.array[i];
		cpdlc_wpt_spd_alt_t *wsa_out =
		    &add_info->wpt_spd_alt[add_info->num_wpt_spd_alt];

		if (wsa_in == NULL)
			continue;
		wsa_out->pos = decode_pos_asn(&wsa_in->position);
		wsa_out->spd = decode_spd_asn(wsa_in->speed);
		for (int j = 0; wsa_in->aTWaltitudesequence != NULL &&
		    j < wsa_in->aTWaltitudesequence->list.count && j < 2; j++) {
			wsa_out->alt[j] = decode_alt_cstr(
			    wsa_in->aTWaltitudesequence->list.array[j]);
		}
		add_info->num_wpt_spd_alt++;
	}
}

static void
decode_rta_seq_asn(const RTArequiredtimearrivalsequence_t *seq,
    cpdlc_route_add_info_t *add_info)
{
	CPDLC_ASSERT(seq != NULL);
	CPDLC_ASSERT(add_info != NULL);

	for (int i = 0; i < seq->list.count && add_info->num_rta < 32; i++) {
		const RTArequiredtimearrival_t *rta_in = seq->list.array[i];
		cpdlc_rta_t *rta_out = &add_info->rta[add_info->num_rta];

		if (rta_in == NULL)
			continue;
		rta_out->pos = decode_pos_asn(&rta_in->position);
		rta_out->time = decode_time_asn(&rta_in->rTAtime.time);
		switch (rta_in->rTAtime.timetolerance) {
		case Timetolerance_at:
			rta_out->toler = CPDLC_TIME_TOLER_AT;
			break;
		case Timetolerance_atorafter:
			rta_out->toler = CPDLC_TIME_TOLER_AT_OR_AFTER;
			break;
		case Timetolerance_atorbefore:
			rta_out->toler = CPDLC_TIME_TOLER_AT_OR_BEFORE;
			break;
		}
	}
}

static bool
decode_route_add_info(const Routeinformationadditional_t *add_info_in,
    cpdlc_route_add_info_t *add_info_out)
{
	CPDLC_ASSERT(add_info_in != NULL);
	CPDLC_ASSERT(add_info_out != NULL);

	if (add_info_in->aTWalongtrackwaypointsequence != NULL) {
		decode_atw_seq_asn(add_info_in->aTWalongtrackwaypointsequence,
		    add_info_out);
	}
	if (add_info_in->reportingpoints != NULL) {
		decode_rpt_pts_asn(add_info_in->reportingpoints,
		    &add_info_out->rpt_pts);
	}
	if (add_info_in->interceptcoursefromsequence != NULL) {
		if (!decode_intc_from_asn(
		    add_info_in->interceptcoursefromsequence, add_info_out)) {
			return (false);
		}
	}
	if (add_info_in->holdatwaypointsequence != NULL) {
		if (!decode_hold_at_asn(
		    add_info_in->holdatwaypointsequence, add_info_out)) {
			return (false);
		}
	}
	if (add_info_in->waypointspeedaltitudesequence != NULL) {
		decode_wpt_spd_alt_asn(
		    add_info_in->waypointspeedaltitudesequence, add_info_out);
	}
	if (add_info_in->rTArequiredtimearrivalsequenc != NULL) {
		decode_rta_seq_asn(
		    add_info_in->rTArequiredtimearrivalsequenc, add_info_out);
	}
	return (true);
}

static bool
decode_route_asn(const Routeclearance_t *rc, cpdlc_route_t *route)
{
	CPDLC_ASSERT(rc != NULL);
	CPDLC_ASSERT(route != NULL);

	if (rc->airportdeparture != NULL) {
		ia5strlcpy_in(route->orig_icao, rc->airportdeparture,
		    sizeof (route->orig_icao));
	}
	if (rc->airportdestination != NULL) {
		ia5strlcpy_in(route->dest_icao, rc->airportdestination,
		    sizeof (route->dest_icao));
	}
	if (rc->runwaydeparture != NULL) {
		const Runway_t *rwy = rc->runwaydeparture;
		char *side;

		switch (rwy->runwayconfiguration) {
		case Runwayconfiguration_left:
			side = "L";
			break;
		case Runwayconfiguration_right:
			side = "R";
			break;
		case Runwayconfiguration_center:
			side = "C";
			break;
		default:
			side = "";
			break;
		}
		snprintf(route->orig_rwy, sizeof (route->orig_rwy),
		    "%02d%s", (int)rwy->runwaydirection, side);
	}
	if (rc->proceduredeparture != NULL)
		decode_proc_asn(rc->proceduredeparture, &route->sid);
	if (rc->procedurearrival != NULL)
		decode_proc_asn(rc->procedurearrival, &route->star);
	if (rc->procedureapproach != NULL)
		decode_proc_asn(rc->procedureapproach, &route->appch);
	if (rc->airwayintercept != NULL) {
		ia5strlcpy_in(route->awy_intc, rc->airwayintercept,
		    sizeof (route->awy_intc));
	}
	if (rc->routeinformation_seqOf != NULL) {
		if (!decode_route_info(rc->routeinformation_seqOf, route))
			return (false);
	}
	if (rc->routeinformationadditional != NULL) {
		if (!decode_route_add_info(rc->routeinformationadditional,
		    &route->add_info)) {
			return (false);
		}
	}

	return (route);
}

static void
decode_icao_name(const ICAOunitname_t *icaoname_in,
    cpdlc_icao_name_t *icaoname_out)
{
	CPDLC_ASSERT(icaoname_in != NULL);
	CPDLC_ASSERT(icaoname_out != NULL);

	switch (icaoname_in->iCAOfacilityidentification.present) {
	default:
		break;
	case ICAOfacilityidentification_PR_iCAOfacilitydesignation:
		ia5strlcpy_in(icaoname_out->icao_id,
		    &icaoname_in->iCAOfacilityidentification.choice.
		    iCAOfacilitydesignation, sizeof (icaoname_out->icao_id));
		break;
	case ICAOfacilityidentification_PR_iCAOfacilityname:
		icaoname_out->is_name = true;
		ia5strlcpy_in(icaoname_out->name,
		    &icaoname_in->iCAOfacilityidentification.choice.
		    iCAOfacilityname, sizeof (icaoname_out->name));
		break;
	}
	icaoname_out->func = icaoname_in->iCAOfacilityfunction;
}

static void
decode_pos_rep_asn(const Positionreport_t *pos_rep_in,
    cpdlc_pos_rep_t *pos_rep_out)
{
	CPDLC_ASSERT(pos_rep_in != NULL);
	CPDLC_ASSERT(pos_rep_out != NULL);

	pos_rep_out->cur_pos = decode_pos_asn(&pos_rep_in->positioncurrent);
	pos_rep_out->time_cur_pos =
	    decode_time_asn(&pos_rep_in->timeatpositioncurrent);
	pos_rep_out->cur_alt = decode_alt_asn(&pos_rep_in->altitude);
	pos_rep_out->fix_next = decode_pos_asn(pos_rep_in->fixnext);
	pos_rep_out->time_fix_next =
	    decode_time_asn(pos_rep_in->timeetaatfixnext);
	pos_rep_out->fix_next_p1 = decode_pos_asn(pos_rep_in->fixnextplusone);
	pos_rep_out->time_dest =
	    decode_time_asn(pos_rep_in->timeetadestination);
	pos_rep_out->rmng_fuel =
	    decode_rmng_fuel_asn(pos_rep_in->remainingfuel);
	pos_rep_out->temp = decode_temp_asn(pos_rep_in->temperature);
	pos_rep_out->wind = decode_winds_asn(pos_rep_in->winds);
	if (pos_rep_in->turbulence != NULL)
		pos_rep_out->turb = (*pos_rep_in->turbulence) + 1;
	else
		pos_rep_out->turb = CPDLC_TURB_NONE;
	if (pos_rep_in->icing != NULL)
		pos_rep_out->icing = (*pos_rep_in->icing) + 1;
	else
		pos_rep_out->icing = CPDLC_ICING_NONE;
	pos_rep_out->spd = decode_spd_asn(pos_rep_in->speed);
	if (pos_rep_in->speedground != NULL) {
		pos_rep_out->spd_gnd = ((cpdlc_spd_t){
		    .gnd = true,
		    .spd = *pos_rep_in->speedground
		});
	} else {
		pos_rep_out->spd_gnd = CPDLC_NULL_SPD;
	}
	pos_rep_out->vvi = decode_vvi_chg_asn(pos_rep_in->verticalchange,
	    &pos_rep_out->vvi_set);
	pos_rep_out->trk = decode_deg_asn(pos_rep_in->trackangle, NULL);
	pos_rep_out->hdg_true = decode_deg_asn(pos_rep_in->trueheading, NULL);
	pos_rep_out->dist_nm = decode_dist_asn(pos_rep_in->distance,
	    &pos_rep_out->dist_set);
	if (pos_rep_in->supplementaryinformation != NULL) {
		ia5strlcpy_in(pos_rep_out->remarks,
		    pos_rep_in->supplementaryinformation,
		    sizeof (pos_rep_out->remarks));
	} else {
		pos_rep_out->remarks[0] = '\0';
	}
	pos_rep_out->rpt_wpt_pos =
	    decode_pos_asn(pos_rep_in->reportedwaypointposition);
	pos_rep_out->rpt_wpt_time =
	    decode_time_asn(pos_rep_in->reportedwaypointtime);
	pos_rep_out->rpt_wpt_alt =
	    decode_alt_asn(pos_rep_in->reportedwaypointaltitude);
}

static void
decode_acf_eqpt_code_asn(const Aircraftequipmentcode_t *eqpt_in,
    cpdlc_acf_eqpt_code_t *eqpt_out)
{
	CPDLC_ASSERT(eqpt_out != NULL);
	if (eqpt_in == NULL) {
		memset(eqpt_out, 0, sizeof (*eqpt_out));
		return;
	}
	eqpt_out->com_nav_app_eqpt_avail =
	    (eqpt_in->cOMNAVapproachequipmentavailable != 0);
	if (eqpt_in->cOMNAVequipmentstatus_seqOf != NULL) {
		for (int i = 0; eqpt_out->num_com_nav_eqpt_st < 16 &&
		    i < eqpt_in->cOMNAVequipmentstatus_seqOf->list.count; i++) {
			const COMNAVequipmentstatus_t *st_p =
			    eqpt_in->cOMNAVequipmentstatus_seqOf->list.array[i];
			if (st_p != NULL) {
				eqpt_out->com_nav_eqpt_st[
				    eqpt_out->num_com_nav_eqpt_st] = *st_p;
			}
		}
	}
	eqpt_out->ssr_eqpt = eqpt_in->sSRequipmentavailable;
}

static bool
decode_pdc_asn(const Predepartureclearance_t *pdc_in, cpdlc_pdc_t *pdc_out)
{
	CPDLC_ASSERT(pdc_in != NULL);
	CPDLC_ASSERT(pdc_out != NULL);

	ia5strlcpy_in(pdc_out->acf_id, &pdc_in->aircraftflightidentification,
	    sizeof (pdc_out->acf_id));
	if (pdc_in->aircrafttype != NULL) {
		ia5strlcpy_in(pdc_out->acf_type, pdc_in->aircrafttype,
		    sizeof (pdc_out->acf_type));
	}
	decode_acf_eqpt_code_asn(pdc_in->aircraftequipmentcode,
	    &pdc_out->acf_eqpt_code);
	pdc_out->time_dep = decode_time_asn(&pdc_in->timedepartureedct);
	if (!decode_route_asn(&pdc_in->routeclearance, &pdc_out->route))
		return (false);
	pdc_out->alt_restr = decode_alt_asn(pdc_in->altituderestriction);
	pdc_out->freq = pdc_in->frequencydeparture / 1000.0;
	pdc_out->squawk = decode_squawk_asn(&pdc_in->beaconcode);
	pdc_out->revision = pdc_in->pDCrevision;

	return (true);
}

static cpdlc_errinfo_t
decode_errinfo_asn(const Errorinformation_t *errinfo)
{
	CPDLC_ASSERT(errinfo != NULL);
	return (*errinfo);
}

static unsigned
decode_version_asn(const Versionnumber_t *ver)
{
	CPDLC_ASSERT(ver != NULL);
	return (*ver);
}

static char
decode_atis_code_asn(const Atiscode_t *code)
{
	char str[2];
	CPDLC_ASSERT(code != NULL);
	ia5strlcpy_in(str, code, sizeof (str));
	return (MIN(MAX(str[0], 'A'), 'Z'));
}

static cpdlc_legtype_t
decode_legtype_asn(const Legtype_t *leg)
{
	CPDLC_ASSERT(leg != NULL);
	switch (leg->present) {
	default:
		return ((cpdlc_legtype_t){});
	case Legtype_PR_legdistance:
		return ((cpdlc_legtype_t){
		    false,
		    leg->choice.legdistance.choice.legdistanceenglish / 10.0
		});
	case Legtype_PR_legtime:
		return ((cpdlc_legtype_t){ true, leg->choice.legtime / 10.0 });
	}
}

static bool
decode_msg_elem(cpdlc_msg_seg_t *seg, const cpdlc_msg_info_t *info,
    const void *elem)
{
	CPDLC_ASSERT(seg != NULL);
	CPDLC_ASSERT(info != NULL);
	CPDLC_ASSERT(elem != NULL);
	for (unsigned i = 0; i < info->num_args; i++) {
		switch (info->args[i]) {
		case CPDLC_ARG_ALTITUDE:
			seg->args[i].alt = decode_alt_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_SPEED:
			seg->args[i].spd = decode_spd_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_TIME:
		case CPDLC_ARG_TIME_DUR:
			seg->args[i].time = decode_time_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_POSITION:
			seg->args[i].pos = decode_pos_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_DIRECTION:
			seg->args[i].dir =
			    decode_dir_asn(get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_DISTANCE:
			seg->args[i].dist = decode_dist_asn(
			    get_asn_arg_ptr(info, i, elem), NULL);
			break;
		case CPDLC_ARG_DISTANCE_OFFSET:
			seg->args[i].dist =
			    decode_dist_off_asn(get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_VVI:
			seg->args[i].vvi =
			    decode_vvi_asn(get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_TOFROM:
			seg->args[i].tofrom =
			    decode_tofrom_asn(get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_ROUTE:
			seg->args[i].route =
			    safe_calloc(1, sizeof (*seg->args[i].route));
			if (!decode_route_asn(get_asn_arg_ptr(info, i, elem),
			    seg->args[i].route)) {
				return (false);
			}
			break;
		case CPDLC_ARG_PROCEDURE:
			decode_proc_asn(get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].proc);
			break;
		case CPDLC_ARG_SQUAWK:
			seg->args[i].squawk =
			    decode_squawk_asn(get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_ICAO_ID:
			ia5strlcpy_in(seg->args[i].icao_id,
			    get_asn_arg_ptr(info, i, elem),
			    sizeof (seg->args[i].icao_id));
			break;
		case CPDLC_ARG_ICAO_NAME:
			decode_icao_name(get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].icao_name);
			break;
		case CPDLC_ARG_FREQUENCY:
			seg->args[i].freq = decode_freq_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_DEGREES:
			seg->args[i].deg.deg = decode_deg_asn(
			    get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].deg.tru);
			break;
		case CPDLC_ARG_BARO:
			decode_baro_asn(get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].baro.val,
			    &seg->args[i].baro.hpa);
			break;
		case CPDLC_ARG_FREETEXT:
			seg->args[i].freetext = decode_freetext_asn(
			    get_asn_arg_ptr(info, i, elem));
			if (seg->args[i].freetext == NULL)
				return (false);
			break;
		case CPDLC_ARG_PERSONS:
			seg->args[i].pob = decode_persons_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_POSREPORT:
			decode_pos_rep_asn(get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].pos_rep);
			break;
		case CPDLC_ARG_PDC:
			CPDLC_ASSERT(seg->args[i].pdc == NULL);
			seg->args[i].pdc =
			    safe_calloc(1, sizeof (*seg->args[i].pdc));
			if (!decode_pdc_asn(get_asn_arg_ptr(info, i, elem),
			    seg->args[i].pdc)) {
				return (false);
			}
			break;
		case CPDLC_ARG_TP4TABLE:
			seg->args[i].tp4 = decode_tp4table_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_ERRINFO:
			seg->args[i].errinfo = decode_errinfo_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_VERSION:
			seg->args[i].version = decode_version_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_ATIS_CODE:
			seg->args[i].atis_code = decode_atis_code_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_LEGTYPE:
			seg->args[i].legtype = decode_legtype_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		}
	}
	return (true);
}

static bool
dl_msg_decode_asn_seg(cpdlc_msg_t *msg, const ATCdownlinkmsgelementid_t *elem)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(elem != NULL);

	if (elem->present == ATCdownlinkmsgelementid_PR_NOTHING)
		return (true);
	for (const cpdlc_msg_info_t *info = cpdlc_dl_infos;
	    info->msg_type != -1; info++) {
		if ((unsigned)elem->present == info->asn_elem_id) {
			int nr = cpdlc_msg_add_seg(msg, true, info->msg_type,
			    info->msg_subtype);
			cpdlc_msg_seg_t *seg;

			CPDLC_ASSERT(nr >= 0);
			seg = &msg->segs[nr];
			if (!decode_msg_elem(seg, info, elem))
				return (false);
			break;
		}
	}
	return (true);
}

static bool
ul_msg_decode_asn_seg(cpdlc_msg_t *msg, const ATCuplinkmsgelementid_t *elem)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(elem != NULL);

	if (elem->present == ATCuplinkmsgelementid_PR_NOTHING)
		return (true);
	for (const cpdlc_msg_info_t *info = cpdlc_ul_infos;
	    info->msg_type != -1; info++) {
		if ((unsigned)elem->present == info->asn_elem_id) {
			int nr = cpdlc_msg_add_seg(msg, false, info->msg_type,
			    info->msg_subtype);
			cpdlc_msg_seg_t *seg;

			CPDLC_ASSERT(nr >= 0);
			seg = &msg->segs[nr];
			if (!decode_msg_elem(seg, info, elem))
				return (false);
			break;
		}
	}
	return (true);
}

static void
decode_msg_header_asn(cpdlc_msg_t *msg, const ATCmessageheader_t *hdr)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(hdr != NULL);

	msg->min = hdr->msgidentificationnumber;
	if (hdr->msgreferencenumber != NULL)
		msg->mrn = *hdr->msgreferencenumber;
	if (hdr->timestamp != NULL) {
		msg->ts.set = true;
		msg->ts.hrs = hdr->timestamp->timehours;
		msg->ts.mins = hdr->timestamp->timeminutes;
		msg->ts.secs = hdr->timestamp->timeseconds;
	}
}

static bool
msg_decode_asn(cpdlc_msg_t *msg, const void *struct_ptr, bool is_dl)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(struct_ptr != NULL);

	if (is_dl) {
		const ATCdownlinkmessage_t *dl = struct_ptr;

		decode_msg_header_asn(msg, &dl->aTCmessageheader);
		if (!dl_msg_decode_asn_seg(msg,
		    &dl->aTCdownlinkmsgelementid)) {
			return (false);
		}
		for (int i = 0;
		    dl->aTCdownlinkmsgelementid_seqOf != NULL &&
		    i < dl->aTCdownlinkmsgelementid_seqOf->list.count; i++) {
			if (dl->aTCdownlinkmsgelementid_seqOf->list.array[i] !=
			    NULL &&
			    !dl_msg_decode_asn_seg(msg,
			    dl->aTCdownlinkmsgelementid_seqOf->list.array[i])) {
				return (false);
			}
		}
	} else {
		const ATCuplinkmessage_t *ul = struct_ptr;

		decode_msg_header_asn(msg, &ul->aTCmessageheader);
		if (!ul_msg_decode_asn_seg(msg,
		    &ul->aTCuplinkmsgelementid)) {
			return (false);
		}
		for (int i = 0;
		    ul->aTCuplinkmsgelementid_seqOf != NULL &&
		    i < ul->aTCuplinkmsgelementid_seqOf->list.count; i++) {
			if (ul->aTCuplinkmsgelementid_seqOf->list.array[i] !=
			    NULL &&
			    !ul_msg_decode_asn_seg(msg,
			    ul->aTCuplinkmsgelementid_seqOf->list.array[i])) {
				return (false);
			}
		}
	}
	return (true);
}

bool
cpdlc_msg_decode_arinc622(cpdlc_msg_t *msg, const char *start,
    const char *end, bool is_dl, char *reason, unsigned reason_cap)
{
	uint8_t *rawbuf;
	unsigned binsz, rawsz;
	asn_TYPE_descriptor_t *td;
	asn_dec_rval_t rval;
	void *struct_ptr = NULL;
	char imi[4];
	uint16_t crc;

	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(start != NULL);
	CPDLC_ASSERT(end != NULL);
	CPDLC_ASSERT(reason != NULL || reason_cap == 0);

	if (end - start <= CPDLC_DATA_OFF + CPDLC_CRC_LEN) {
		MALFORMED_MSG("malformed ARINC 622 data: message too short");
		return (false);
	}
	cpdlc_strlcpy(imi, start, sizeof (imi));
	if (strcmp(imi, "CR1") == 0) {
		msg->arinc622.imi = CPDLC_IMI_CONN_REQUEST;
	} else if (strcmp(imi, "CC1") == 0) {
		msg->arinc622.imi = CPDLC_IMI_CONN_CONFIRM;
	} else if (strcmp(imi, "AT1") == 0) {
		msg->arinc622.imi = CPDLC_IMI_DATA;
	} else if (strcmp(imi, "DR1") == 0) {
		msg->arinc622.imi = CPDLC_IMI_DISC_REQUEST;
		msg->is_logoff = true;
	} else {
		MALFORMED_MSG("malformed ARINC 622 data: unknown IMI \"%s\"",
		    imi);
		return (false);
	}
	cpdlc_strlcpy(msg->arinc622.acf_id, &start[CPDLC_IMI_LEN],
	    sizeof (msg->arinc622.acf_id));
	start += CPDLC_DATA_OFF;
	if ((end - start) % 2 != 0) {
		MALFORMED_MSG("malformed ARINC 622 data: expected even number "
		    "of hex chars");
		return (false);
	}
	binsz = (end - start) / 2;
	rawsz = CPDLC_IMI_LEN + CPDLC_CS_LEN + binsz;
	rawbuf = safe_malloc(rawsz);
	memcpy(rawbuf, imi, CPDLC_IMI_LEN);
	memcpy(&rawbuf[CPDLC_IMI_LEN], msg->arinc622.acf_id, CPDLC_CS_LEN);
	if (!cpdlc_hex_dec(start, end - start, &rawbuf[CPDLC_DATA_OFF],
	    binsz)) {
		MALFORMED_MSG("malformed ARINC 622 data: invalid hex chars "
		    "found");
		goto errout;
	}
	crc = cpdlc_crc16(rawbuf, rawsz - CPDLC_CRC_LEN);
	if (((crc >> 8) & 0xff) != rawbuf[rawsz - 2] ||
	    (crc & 0xff) != rawbuf[rawsz - 1]) {
		MALFORMED_MSG("malformed ARINC 622 data: CRC mismatch");
		goto errout;
	}
	td = (is_dl ? &asn_DEF_ATCdownlinkmessage : &asn_DEF_ATCuplinkmessage);
	rval = uper_decode_complete(0, td, &struct_ptr, &rawbuf[CPDLC_DATA_OFF],
	    binsz - CPDLC_CRC_LEN);
	if (rval.code != RC_OK) {
		MALFORMED_MSG("error decoding ARINC 622 ASN.1 data: error %d",
		    rval.code);
		goto errout;
	}
	if (rval.consumed < binsz - CPDLC_CRC_LEN) {
		MALFORMED_MSG("error decoding ARINC 622 ASN.1 data: "
		    "extraneous data at end of ASN.1 input");
		goto errout;
	}
	if (!msg_decode_asn(msg, struct_ptr, is_dl)) {
		MALFORMED_MSG("error decoding ARINC 622 ASN.1 data: "
		    "error interpreting parsed data structures");
		ASN_STRUCT_FREE(*td, struct_ptr);
		goto errout;
	}
	td->free_struct(td, struct_ptr, 0);
	free(rawbuf);
	return (true);
errout:
	free(rawbuf);
	return (false);
}
