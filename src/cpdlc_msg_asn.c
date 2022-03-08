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
#include "cpdlc_msg_asn.h"

#define	MET2FEET(x)	((x) * 3.2808398950131)	/* meters to feet */

static void
ia5strlcpy(char *out, const IA5String_t *in, unsigned cap)
{
	if (cap != 0) {
		unsigned len = MIN((unsigned)in->size, cap - 1);
		CPDLC_ASSERT(in != NULL);
		memcpy(out, in->buf, len);
		out[len] = '\0';	/* zero-terminate */
	}
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
	ll.lat = dir * (lat.latitudedegrees + mins);

	dir = (lon.longitudedirection == Longitudedirection_east ? 1 : -1);
	mins = (lon.minuteslatlon != NULL ? (*lon.minuteslatlon) / 10.0 : 0);
	ll.lon = dir * (lon.longitudedegrees + mins);

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
decode_dist_asn(const Distance_t *dist)
{
	CPDLC_ASSERT(dist != NULL);
	switch (dist->present) {
	default:
		return (0);
	case Distance_PR_distancenm:
		return (dist->choice.distancenm / 10.0);
	case Distance_PR_distancekm:
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
		return (dist_off->choice.distanceoffsetnm / 10.0);
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
		ia5strlcpy(pos_out.fixname, &pos_in->choice.fixname,
		    sizeof (pos_out.fixname));
		break;
	case Position_PR_navaid:
		pos_out.set = true;
		pos_out.type = CPDLC_POS_NAVAID;
		ia5strlcpy(pos_out.navaid, &pos_in->choice.navaid,
		    sizeof (pos_out.navaid));
		break;
	case Position_PR_airport:
		pos_out.set = true;
		pos_out.type = CPDLC_POS_AIRPORT;
		ia5strlcpy(pos_out.airport, &pos_in->choice.airport,
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
		ia5strlcpy(pos_out.pbd.fixname,
		    &pos_in->choice.placebearingdistance.fixname,
		    sizeof (pos_out.pbd.fixname));
		pos_out.pbd.lat_lon = latlon_asn2cpdlc(pos_in->
		    choice.placebearingdistance.latitudelongitude);
		pos_out.pbd.degrees = decode_deg_asn(
		    &pos_in->choice.placebearingdistance.degrees, NULL);
		pos_out.pbd.dist_nm = decode_dist_asn(
		    &pos_in->choice.placebearingdistance.distance);
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
	ia5strlcpy(proc_out->name, &proc_in->procedure,
	    sizeof (proc_out->name));
	if (proc_in->proceduretransition != NULL) {
		ia5strlcpy(proc_out->trans, proc_in->proceduretransition,
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
			code = (code << 3) | (*squawk->list.array[i]);
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
	ia5strlcpy(freetext_out, freetext_in, freetext_in->size + 1);
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
		ia5strlcpy(pbpb_out[i].fixname, &pb_in->fixname,
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

	ia5strlcpy(pbd_out->fixname, &pbd_in->fixname,
	    sizeof (pbd_out->fixname));
	pbd_out->degrees = decode_deg_asn(&pbd_in->degrees, NULL);
	pbd_out->dist_nm = decode_dist_asn(&pbd_in->distance);
	pbd_out->lat_lon = latlon_asn2cpdlc(pbd_in->latitudelongitude);
}

static void
decode_track_detail_asn(const Trackdetail_t *trk_in,
    cpdlc_trk_detail_t *trk_out)
{
	CPDLC_ASSERT(trk_in != NULL);
	CPDLC_ASSERT(trk_out != NULL);

	ia5strlcpy(trk_out->name, &trk_in->trackname, sizeof (trk_out->name));
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

	ia5strlcpy(id_out->fixname, &id_in->fixname, sizeof (id_out->fixname));
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
			ia5strlcpy(info_out->awy,
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

static cpdlc_route_t *
decode_route_asn(const Routeclearance_t *rc)
{
	cpdlc_route_t *route = safe_calloc(1, sizeof (*route));

	CPDLC_ASSERT(rc != NULL);

	if (rc->airportdeparture != NULL) {
		ia5strlcpy(route->orig_icao, rc->airportdeparture,
		    sizeof (route->orig_icao));
	}
	if (rc->airportdestination != NULL) {
		ia5strlcpy(route->dest_icao, rc->airportdestination,
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
		ia5strlcpy(route->awy_intc, rc->airwayintercept,
		    sizeof (route->awy_intc));
	}
	if (rc->routeinformation_seqOf != NULL) {
		if (!decode_route_info(rc->routeinformation_seqOf, route))
			goto errout;
	}
	if (rc->routeinformationadditional != NULL) {
		if (!decode_route_add_info(rc->routeinformationadditional,
		    &route->add_info)) {
			goto errout;
		}
	}

	return (route);
errout:
	free(route);
	return (NULL);
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
			seg->args[i].time = decode_time_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_TIME_DUR:
			return (false);
		case CPDLC_ARG_POSITION:
			seg->args[i].pos = decode_pos_asn(
			    get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_DIRECTION:
			seg->args[i].dir =
			    decode_dir_asn(get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_DISTANCE:
			seg->args[i].dist =
			    decode_dist_asn(get_asn_arg_ptr(info, i, elem));
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
			    decode_route_asn(get_asn_arg_ptr(info, i, elem));
			if (seg->args[i].route == NULL)
				return (false);
			break;
		case CPDLC_ARG_PROCEDURE:
			decode_proc_asn(get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].proc);
			break;
		case CPDLC_ARG_SQUAWK:
			seg->args[i].squawk =
			    decode_squawk_asn(get_asn_arg_ptr(info, i, elem));
			break;
		case CPDLC_ARG_ICAONAME:
			ia5strlcpy(seg->args[i].icaoname.icao,
			    get_asn_arg_ptr(info, i, elem),
			    sizeof (seg->args[i].icaoname.icao));
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
		case CPDLC_ARG_PDC:
			// TODO
			return (false);
		case CPDLC_ARG_TP4TABLE:
			seg->args[i].tp4 = decode_tp4table_asn(
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

bool
cpdlc_msg_decode_asn_impl(cpdlc_msg_t *msg, const void *struct_ptr, bool is_dl)
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
		for (int i = 1;
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
		for (int i = 1;
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
