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

static void
decode_alt_asn(const Altitude_t *alt_in, cpdlc_alt_t *alt_out)
{
	CPDLC_ASSERT(alt_in != NULL);
	CPDLC_ASSERT(alt_out != NULL);
	switch (alt_in->present) {
	default:
		*alt_out = CPDLC_NULL_ALT;
		break;
	case Altitude_PR_altitudeqnh:
		alt_out->alt = alt_in->choice.altitudeqnh * 10;
		break;
	case Altitude_PR_altitudeqnhmeters:
		alt_out->met = true;
		alt_out->alt =
		    round(MET2FEET(alt_in->choice.altitudeqnhmeters));
		break;
	case Altitude_PR_altitudeqfe:
	case Altitude_PR_altitudeqfemeters:
		*alt_out = CPDLC_NULL_ALT;
		break;
	case Altitude_PR_altitudegnssfeet:
		alt_out->alt = alt_in->choice.altitudegnssfeet;
		break;
	case Altitude_PR_altitudegnssmeters:
		alt_out->met = true;
		alt_out->alt =
		    round(MET2FEET(alt_in->choice.altitudegnssmeters));
		break;
	case Altitude_PR_altitudeflightlevel:
		alt_out->fl = true;
		alt_out->alt = alt_in->choice.altitudeflightlevel * 100;
		break;
	case Altitude_PR_altitudeflightlevelmetric:
		alt_out->fl = true;
		alt_out->met = true;
		alt_out->alt = round(MET2FEET(
		    alt_in->choice.altitudeflightlevelmetric * 10));
		break;
	}
}

static void
decode_spd_asn(const Speed_t *spd_in, cpdlc_spd_t *spd_out)
{
	CPDLC_ASSERT(spd_in != NULL);
	CPDLC_ASSERT(spd_out != NULL);

	switch (spd_in->present) {
	default:
		*spd_out = CPDLC_NULL_SPD;
		break;
	case Speed_PR_speedindicated:
		spd_out->spd = spd_in->choice.speedindicated * 10;
		break;
	case Speed_PR_speedindicatedmetric:
		spd_out->spd = round((spd_in->choice.speedindicatedmetric *
		    10) / 1.852);
		break;
	case Speed_PR_speedtrue:
		spd_out->tru = true;
		spd_out->spd = spd_in->choice.speedtrue * 10;
		break;
	case Speed_PR_speedtruemetric:
		spd_out->tru = true;
		spd_out->spd = round((spd_in->choice.speedtruemetric *
		    10) / 1.852);
		break;
	case Speed_PR_speedground:
		spd_out->gnd = true;
		spd_out->spd = spd_in->choice.speedground * 10;
		break;
	case Speed_PR_speedgroundmetric:
		spd_out->gnd = true;
		spd_out->spd = round((spd_in->choice.speedgroundmetric *
		    10) / 1.852);
		break;
	case Speed_PR_speedmach:
	case Speed_PR_speedmachlarge:
		spd_out->mach = true;
		spd_out->spd = spd_in->choice.speedmach * 10;
		break;
	}
}

static void
decode_time_asn(const Time_t *time_in, cpdlc_time_t *time_out)
{
	CPDLC_ASSERT(time_in != NULL);
	CPDLC_ASSERT(time_out != NULL);
	time_out->hrs = time_in->timehours;
	time_out->mins = time_in->timeminutes;
}

static cpdlc_lat_lon_t
latlon_asn2cpdlc(LatitudeLongitude_t latlon)
{
	Latitude_t lat = latlon.latitude;
	Longitude_t lon = latlon.longitude;
	cpdlc_lat_lon_t ll;
	int dir;
	double mins;

	dir = (lat.latitudedirection == Latitudedirection_north ? 1 : -1);
	mins = (lat.minuteslatlon != NULL ? (*lat.minuteslatlon) / 10.0 : 0);
	ll.lat = dir * (lat.latitudedegrees + mins);

	dir = (lon.longitudedirection == Longitudedirection_east ? 1 : -1);
	mins = (lon.minuteslatlon != NULL ? (*lon.minuteslatlon) / 10.0 : 0);
	ll.lon = dir * (lon.longitudedegrees + mins);

	return (ll);
}

static unsigned
get_deg(Degrees_t deg)
{
	switch (deg.present) {
	default:
		return (0);
	case Degrees_PR_degreesmagnetic:
		return (deg.choice.degreesmagnetic);
	case Degrees_PR_degreestrue:
		return (deg.choice.degreestrue);
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

static void
decode_pos_asn(const Position_t *pos_in, cpdlc_pos_t *pos_out)
{
	CPDLC_ASSERT(pos_in != NULL);
	CPDLC_ASSERT(pos_out != NULL);
	switch (pos_in->present) {
	default:
		*pos_out = CPDLC_NULL_POS;
		break;
	case Position_PR_fixname:
		pos_out->set = true;
		pos_out->type = CPDLC_POS_FIXNAME;
		ia5strlcpy(pos_out->fixname, &pos_in->choice.fixname,
		    sizeof (pos_out->fixname));
		break;
	case Position_PR_navaid:
		pos_out->set = true;
		pos_out->type = CPDLC_POS_NAVAID;
		ia5strlcpy(pos_out->navaid, &pos_in->choice.navaid,
		    sizeof (pos_out->navaid));
		break;
	case Position_PR_airport:
		pos_out->set = true;
		pos_out->type = CPDLC_POS_AIRPORT;
		ia5strlcpy(pos_out->airport, &pos_in->choice.airport,
		    sizeof (pos_out->airport));
		break;
	case Position_PR_latitudeLongitude:
		pos_out->set = true;
		pos_out->type = CPDLC_POS_LAT_LON;
		pos_out->lat_lon = latlon_asn2cpdlc(
		    pos_in->choice.latitudeLongitude);
		break;
	case Position_PR_placebearingdistance:
		pos_out->set = true;
		pos_out->type = CPDLC_POS_PBD;
		ia5strlcpy(pos_out->pbd.fixname,
		    &pos_in->choice.placebearingdistance.fixname,
		    sizeof (pos_out->pbd.fixname));
		if (pos_in->choice.placebearingdistance.latitudelongitude !=
		    NULL) {
			pos_out->pbd.lat_lon = latlon_asn2cpdlc(*pos_in->
			    choice.placebearingdistance.latitudelongitude);
		} else {
			pos_out->pbd.lat_lon = CPDLC_NULL_LAT_LON;
		}
		pos_out->pbd.degrees = get_deg(
		    pos_in->choice.placebearingdistance.degrees);
		pos_out->pbd.dist_nm = decode_dist_asn(
		    &pos_in->choice.placebearingdistance.distance);
		break;
	}
}

static cpdlc_dir_t
decode_dir_asn(const Direction_t *dir_in)
{
	CPDLC_ASSERT(dir_in != NULL);
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
			decode_alt_asn(get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].alt);
			break;
		case CPDLC_ARG_SPEED:
			decode_spd_asn(get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].spd);
			break;
		case CPDLC_ARG_TIME:
			decode_time_asn(get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].time);
			break;
		case CPDLC_ARG_TIME_DUR:
			return (false);
		case CPDLC_ARG_POSITION:
			decode_pos_asn(get_asn_arg_ptr(info, i, elem),
			    &seg->args[i].pos);
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
		case CPDLC_ARG_PROCEDURE:
			return (false);
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
		case CPDLC_ARG_DEGREES:
		case CPDLC_ARG_BARO:
		case CPDLC_ARG_FREETEXT:
		case CPDLC_ARG_PERSONS:
		case CPDLC_ARG_POSREPORT:
		case CPDLC_ARG_PDC:
			return (false);
		}
	}
	return (true);
}

static bool
dl_msg_decode_asn_seg(cpdlc_msg_t *msg,
    const ATCdownlinkmsgelementid_t *elem)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(elem != NULL);

	if (elem->present == ATCdownlinkmsgelementid_PR_NOTHING)
		return (true);
	for (const cpdlc_msg_info_t *info = cpdlc_dl_infos;
	    info->msg_type != -1; info++) {
		if ((unsigned)elem->present == info->asn_elem_id) {
			cpdlc_msg_seg_t *seg = &msg->segs[msg->num_segs];

			cpdlc_msg_add_seg(msg, true, info->msg_type,
			    info->msg_subtype);
			if (!decode_msg_elem(seg, info, elem))
				return (false);
			break;
		}
	}
	return (true);
}

static bool
ul_msg_decode_asn_seg(cpdlc_msg_t *msg,
    const ATCuplinkmsgelementid_t *elem)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(elem != NULL);

	if (elem->present == ATCuplinkmsgelementid_PR_NOTHING)
		return (true);
	for (const cpdlc_msg_info_t *info = cpdlc_ul_infos;
	    info->msg_type != -1; info++) {
		if ((unsigned)elem->present == info->asn_elem_id) {
			cpdlc_msg_seg_t *seg = &msg->segs[msg->num_segs];

			cpdlc_msg_add_seg(msg, true, info->msg_type,
			    info->msg_subtype);
			if (!decode_msg_elem(seg, info, elem))
				return (false);
			break;
		}
	}
	return (true);
}

bool
cpdlc_msg_decode_asn_impl(cpdlc_msg_t *msg, const void *struct_ptr, bool is_dl)
{
	CPDLC_ASSERT(msg != NULL);
	CPDLC_ASSERT(struct_ptr != NULL);

	if (is_dl) {
		const ATCdownlinkmessage_t *dl = struct_ptr;

		if (!dl_msg_decode_asn_seg(msg,
		    &dl->aTCdownlinkmsgelementid)) {
			return (false);
		}

		for (int i = 1;
		    dl->aTCdownlinkmsgelementid_seqOf != NULL &&
		    i < dl->aTCdownlinkmsgelementid_seqOf->list.size; i++) {
			if (dl->aTCdownlinkmsgelementid_seqOf->list.array[i] !=
			    NULL &&
			    !dl_msg_decode_asn_seg(msg,
			    dl->aTCdownlinkmsgelementid_seqOf->list.array[i])) {
				return (false);
			}
		}
	} else {
		const ATCuplinkmessage_t *ul = struct_ptr;

		if (!ul_msg_decode_asn_seg(msg,
		    &ul->aTCuplinkmsgelementid)) {
			return (false);
		}

		for (int i = 1;
		    ul->aTCuplinkmsgelementid_seqOf != NULL &&
		    i < ul->aTCuplinkmsgelementid_seqOf->list.size; i++) {
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
