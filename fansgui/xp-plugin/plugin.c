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

#include <stdbool.h>
#include <string.h>

#include <zmq.h>

#include <XPLMNavigation.h>
#include <XPLMProcessing.h>
#include <XPLMPlugin.h>
#include <XPLMUtilities.h>

#include <acfutils/crc64.h>
#include <acfutils/dr_cmd_reg.h>
#include <acfutils/geom.h>
#include <acfutils/helpers.h>
#include <acfutils/log.h>
#include <acfutils/math.h>
#include <acfutils/perf.h>
#include <acfutils/time.h>

#include "../../fans/fans.h"

#include "plugin_intf.h"

#define	PLUGIN_NAME		"FANS Client Interface"
#define	PLUGIN_SIG		"skiselkov.fans.plugin"
#define	PLUGIN_DESCRIPTION	\
	"Interface plugin for the standalone FANS CPDLC client"

#define	PUB_DATA_FLOOP_INTVAL		1.0	/* seconds */

static void *ctx = NULL;
static void *sock = NULL;
static xp_plugin_data_t data = {};
static fms_wpt_info_t prev_wpt_data = {};

static struct {
	dr_t	ias;
	dr_t	mach;
	dr_t	spd_is_mach;
	dr_t	gs;
	dr_t	alt;
	dr_t	gps_dme_time;
	dr_t	lat, lon;
} drs = {};

static void
log_dbg_string(const char *str)
{
	XPLMDebugString(str);
}

static void
find_drs(void)
{
	fdr_find(&drs.ias, "sim/cockpit2/gauges/indicators/airspeed_kts_pilot");
	fdr_find(&drs.mach, "sim/cockpit2/gauges/indicators/mach_pilot");
	fdr_find(&drs.spd_is_mach, "sim/cockpit2/autopilot/airspeed_is_mach");
	fdr_find(&drs.gs, "sim/flightmodel2/position/groundspeed");
	fdr_find(&drs.alt, "sim/cockpit2/gauges/indicators/altitude_ft_pilot");
	fdr_find(&drs.gps_dme_time, "sim/cockpit/radios/gps_dme_time_secs");
	fdr_find(&drs.lat, "sim/flightmodel/position/latitude");
	fdr_find(&drs.lon, "sim/flightmodel/position/longitude");
}

static void
get_wpt_info(int index, fms_wpt_info_t *info)
{
	char ID[256] = {};
	XPLMNavType type;
	int alt_ft;
	float lat, lon;

	ASSERT(info != NULL);

	XPLMGetFMSEntryInfo(index, &type, ID, NULL, &alt_ft, &lat, &lon);
	if ((type & (xplm_Nav_Airport | xplm_Nav_NDB | xplm_Nav_VOR |
	    xplm_Nav_ILS | xplm_Nav_Localizer | xplm_Nav_GlideSlope |
	    xplm_Nav_Fix | xplm_Nav_DME)) != 0) {
		strlcpy(info->wpt_name, ID, sizeof (info->wpt_name));
		if (alt_ft != 0) {
			info->alt_ft = alt_ft;
			info->alt_set = true;
		}
		info->lat = lat;
		info->lon = lon;
	}
}

static void
find_dest_data(int cur, int n_fms)
{
	const double MAX_DIST_DES = NM2MET(120);
	const double MAX_DIST_APP = NM2MET(12);
	const double MIN_SPD = KT2MPS(100);
	const double MAX_SPD_DES = KT2MPS(250);
	const double MAX_SPD_APP = KT2MPS(140);
	int dest_idx = -1;
	double dist = 0, gs = dr_getf(&drs.gs);
	double v_crz, v_des_end, v_des_avg, v_app_end, v_app_avg, v_avg;
	double d_crz, d_des, d_app;
	geo_pos2_t cur_pos;

	for (int i = n_fms - 1; i >= cur && i >= 0; i--) {
		XPLMNavType type;

		XPLMGetFMSEntryInfo(i, &type, NULL, NULL, NULL, NULL, NULL);
		if (type == xplm_Nav_Airport) {
			dest_idx = i;
			break;
		}
	}
	if (dest_idx == -1)
		return;

	get_wpt_info(dest_idx, &data.dest_wpt);
	cur_pos = GEO_POS2(dr_getf(&drs.lat), dr_getf(&drs.lon));
	for (int i = cur; i <= dest_idx; i++) {
		float lat, lon;

		XPLMGetFMSEntryInfo(i, NULL, NULL, NULL, NULL, &lat, &lon);
		if (is_valid_lat(lat) && is_valid_lon(lon)) {
			dist += gc_distance(cur_pos, GEO_POS2(lat, lon));
			cur_pos = GEO_POS2(lat, lon);
		}
	}
	data.dest_dist = dist;

	if (dist < 100)
		return;
	/*
	 * For the bulk of the trip distance we will estimate that we
	 * will be flying at a constant groundspeed. Then in the last
	 * 120NM, we estimate a linear deceleration to 250 knots 12NM
	 * from the destination. In the last 10NM, we approximate a
	 * linear deceleration from 250 knots to 140 knots.
	 */
	gs = MAX(gs, MIN_SPD);
	v_crz = gs;
	v_des_end = MIN(gs, MAX_SPD_DES);
	v_app_end = MIN(gs, MAX_SPD_APP);
	d_crz = MAX(0, dist - MAX_DIST_DES);
	d_des = clamp(dist, MAX_DIST_APP, MAX_DIST_DES) - MAX_DIST_APP;
	d_app = clamp(dist, 0, MAX_DIST_APP);

	v_des_avg = wavg(AVG(v_crz, v_des_end), v_des_end,
	    iter_fract(dist, MAX_DIST_DES, MAX_DIST_APP, true));
	v_app_avg = wavg(AVG(v_des_end, v_app_end), v_app_end,
	    iter_fract(dist, MAX_DIST_APP, 0, true));

	v_avg = v_crz * (d_crz / dist) + v_des_avg * (d_des / dist) +
	    v_app_avg * (d_app / dist);

	data.dest_flt_time = dist / v_avg;
}

static void
update_wpt_passage_data(fms_wpt_info_t *info)
{
	time_t now = time(NULL);
	struct tm t = *gmtime(&now);

	ASSERT(info != NULL);
	info->time_set = true;
	info->hrs = t.tm_hour;
	info->mins = t.tm_min;

	info->alt_set = true;
	info->alt_fl = false;
	info->alt_ft = round(dr_getf(&drs.alt) / 100.0) * 100.0;

	info->spd_set = true;
	if (dr_geti(&drs.spd_is_mach) != 0) {
		info->spd_mach = true;
		info->spd = round(dr_getf(&drs.mach) * 1000);
	} else {
		info->spd_mach = false;
		info->spd = round(dr_getf(&drs.ias));
	}
}

static float
pub_data_floop_cb(float elapsed1, float elapsed2, int counter, void *refcon)
{
	int n_fms = XPLMCountFMSEntries();
	int n_dest = XPLMGetDestinationFMSEntry();

	UNUSED(elapsed1);
	UNUSED(elapsed2);
	UNUSED(counter);
	UNUSED(refcon);

	memset(&data, 0, sizeof (data));

	find_dest_data(n_dest, n_fms);

	if (n_dest > 0 && n_fms > 0) {
		get_wpt_info(n_dest - 1, &data.prev_wpt);
		if (strcmp(data.prev_wpt.wpt_name, prev_wpt_data.wpt_name) !=
		    0) {
			prev_wpt_data = data.prev_wpt;
			update_wpt_passage_data(&prev_wpt_data);
		} else {
			data.prev_wpt = prev_wpt_data;
		}
	}
	if (n_dest < n_fms) {
		float dme_mins = dr_getf(&drs.gps_dme_time);

		get_wpt_info(n_dest, &data.nxt_wpt);
		if (dme_mins > 0 && dme_mins < 24 * 60) {
			time_t then = time(NULL) + dme_mins * 60;
			struct tm t = *gmtime(&then);

			data.nxt_wpt.time_set = true;
			data.nxt_wpt.hrs = t.tm_hour;
			data.nxt_wpt.mins = t.tm_min;
		}
	}
	if (n_dest + 1 < n_fms)
		get_wpt_info(n_dest + 1, &data.nxt_p1_wpt);

	ASSERT(sock != NULL);
	if (zmq_send(sock, &data, sizeof (data), ZMQ_DONTWAIT) == -1)
		logMsg("Error publishing interface data: %s", strerror(errno));

	return (PUB_DATA_FLOOP_INTVAL);
}

static bool
create_zmq(void)
{
	ASSERT3P(ctx, ==, NULL);
	ctx = zmq_ctx_new();
	ASSERT(ctx != NULL);

	ASSERT3P(sock, ==, NULL);
	sock = zmq_socket(ctx, ZMQ_PUB);
	ASSERT(sock != NULL);
	if (zmq_bind(sock, XP_PLUGIN_SOCK_ADDR) == -1) {
		logMsg("Can't bind publish socket: %s", strerror(errno));
		logMsg("Perhaps there is another X-Plane instance running. "
		    "Please shut it down (or kill it).");
		return (false);
	}

	return (true);
}

static void
destroy_zmq(void)
{
	if (sock != NULL) {
		zmq_close(sock);
		sock = NULL;
	}
	if (ctx != NULL) {
		zmq_ctx_destroy(ctx);
		ctx = NULL;
	}
}

PLUGIN_API int
XPluginStart(char *name, char *sig, char *desc)
{
	log_init(log_dbg_string, "FANS");
	logMsg("This is the FANS plugin (" PLUGIN_VERSION ") libacfutils-%s",
	    libacfutils_version);
	crc64_init();
	crc64_srand(microclock() + clock());

	strcpy(name, PLUGIN_NAME);
	strcpy(sig, PLUGIN_SIG);
	strcpy(desc, PLUGIN_DESCRIPTION);

	return (true);
}

PLUGIN_API void
XPluginStop(void)
{
	log_fini();
}

PLUGIN_API int
XPluginEnable(void)
{
	dcr_init();

	find_drs();
	if (!create_zmq())
		return (false);
	XPLMRegisterFlightLoopCallback(pub_data_floop_cb,
	    PUB_DATA_FLOOP_INTVAL, NULL);

	return (true);
}

PLUGIN_API void
XPluginDisable(void)
{
	XPLMUnregisterFlightLoopCallback(pub_data_floop_cb, NULL);
	destroy_zmq();

	dcr_fini();
}

PLUGIN_API void
XPluginReceiveMessage(XPLMPluginID from, int msg, void *param)
{
	UNUSED(from);
	UNUSED(msg);
	UNUSED(param);
}
