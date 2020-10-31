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

#include <time.h>

#include <acfutils/helpers.h>
#include <acfutils/log.h>
#include <acfutils/thread.h>

#include <cpdlc_net_util.h>
#include <cpdlc_string.h>

#include "xpintf.h"

#define	DEFAULT_XP_PORT		49000
#define	RREF_FREQ		2
#define	DATA_REQ_INTVAL		500000	/* usec */
#define	RREF_DATA_TIMEOUT	3000000	/* usec */
#define	CHECK_FOURCC(buf, str) \
	(((buf)[0] == (str)[0]) && ((buf)[1] == (str)[1]) && \
	((buf)[2] == (str)[2]) && ((buf)[3] == (str)[3]))

typedef struct {
	char	fourcc[5];	/* "RPOS" */
	double	lon;		/* longitude, degrees */
	double	lat;		/* latitude, degrees */
	double	elev_msl;	/* elevation MSL, meters */
	float	elev_agl;	/* elevation AGL, meters */
	float	veh_theta_loc;	/* pitch, local coords, degrees */
	float	veh_psi_loc;	/* true heading, local coords, degrees */
	float	veh_phi_loc;	/* roll, local coords, degrees */
	float	vx_wrl;		/* EAST velocity, world coords, m/s */
	float	vy_wrl;		/* UP velocity, world coords, m/s */
	float	vz_wrl;		/* SOUTH velocity, world coords, m/s */
	float	P;		/* roll rate, rad/sec */
	float	Q;		/* pitch rate, rad/sec */
	float	R;		/* yaw rate, rad/sec */
} __attribute__((packed)) rpos_data_t;

CTASSERT(sizeof (float) == sizeof (int32_t));
typedef struct {
	int32_t	freq;
	int32_t	idx;
	char	name[400];
} __attribute__((packed)) dref_req_t;

static bool inited = false;
static uint64_t now = 0;
static mutex_t lock;
static cpdlc_socktype_t sock = CPDLC_INVALID_SOCKET;
static struct addrinfo *ai = NULL;
static uint64_t last_req_t = 0;
/*
 * RPOS data
 */
static time_t rpos_data_recv_t = 0;
static rpos_data_t rpos_data = {};
/*
 * RREF data
 */
enum {
    RREF_SAT,
    RREF_IAS,
    RREF_MACH,
    RREF_IS_MACH,
    RREF_ALT,
    RREF_VVI,
    RREF_ALT_SEL,
    RREF_GPS_HDEF_DOT,
    RREF_GPS_NM_PER_DOT,
    NUM_RREFS
};
static const char *rref_names[NUM_RREFS] = {
    [RREF_SAT] = "sim/cockpit2/temperature/outside_air_LE_temp_degc",
    [RREF_IAS] = "sim/cockpit2/gauges/indicators/airspeed_kts_pilot",
    [RREF_MACH] = "sim/cockpit2/gauges/indicators/mach_pilot",
    [RREF_IS_MACH] = "sim/cockpit2/autopilot/airspeed_is_mach",
    [RREF_ALT] = "sim/cockpit2/gauges/indicators/altitude_ft_pilot",
    [RREF_VVI] = "sim/cockpit2/gauges/indicators/vvi_fpm_pilot",
    [RREF_ALT_SEL] = "sim/cockpit2/autopilot/altitude_dial_ft",
    [RREF_GPS_HDEF_DOT] = "sim/cockpit/radios/gps_hdef_dot",
    [RREF_GPS_NM_PER_DOT] = "sim/cockpit/radios/gps_hdef_nm_per_dot"
};
static uint64_t rref_data_recv_t = 0;
static uint8_t rref_data[4 * NUM_RREFS] = {};

static void send_data_cmd(const void *extra_buf, size_t extra_sz, ...)
    SENTINEL_ATTR;

static bool rref_read_fp32(unsigned index, float *outval) UNUSED_ATTR;
static bool
rref_read_fp32(unsigned index, float *outval)
{
	ASSERT3U(index, <, NUM_RREFS);
	ASSERT(outval != NULL);
	mutex_enter(&lock);
	if (now > rref_data_recv_t + RREF_DATA_TIMEOUT) {
		mutex_exit(&lock);
		return (false);
	}
	*outval = *(float *)(&rref_data[index * 4]);
	mutex_exit(&lock);
	return (true);
}

static bool rref_read_int32(unsigned index, int32_t *outval) UNUSED_ATTR;
static bool
rref_read_int32(unsigned index, int32_t *outval)
{
	ASSERT3U(index, <, NUM_RREFS);
	ASSERT(outval != NULL);
	mutex_enter(&lock);
	if (now > rref_data_recv_t + RREF_DATA_TIMEOUT) {
		mutex_exit(&lock);
		return (false);
	}
	*outval = *(int32_t *)(&rref_data[index * 4]);
	mutex_exit(&lock);
	return (true);
}

static void
send_data_cmd(const void *extra_buf, size_t extra_sz, ...)
{
	int l = 0;
	va_list ap;
	char *outbuf_start, *outbuf_ptr;
	const char *inbuf;

	ASSERT(extra_sz == 0 || extra_buf != NULL);

	va_start(ap, extra_sz);
	while ((inbuf = va_arg(ap, const char *)) != NULL)
		l += strlen(inbuf) + 1;
	va_end(ap);
	l += extra_sz;

	outbuf_ptr = outbuf_start = malloc(l);
	va_start(ap, extra_sz);
	while ((inbuf = va_arg(ap, const char *)) != NULL) {
		memcpy(outbuf_ptr, inbuf, strlen(inbuf) + 1);
		outbuf_ptr += strlen(inbuf) + 1;
	}
	va_end(ap);

	if (extra_sz != 0)
		memcpy(outbuf_ptr, extra_buf, extra_sz);

	sendto(sock, outbuf_start, l, MSG_NOSIGNAL,
	    ai->ai_addr, ai->ai_addrlen);

	free(outbuf_start);
}

static void
send_dref_req(const char *name, int idx, int freq)
{
	dref_req_t req = { .freq = freq, .idx = idx };

	ASSERT(name != NULL);
	cpdlc_strlcpy(req.name, name, sizeof (req.name));
	send_data_cmd(&req, sizeof (req), "RREF", NULL);
}

static void
enable_data(void)
{
	send_data_cmd(NULL, 0, "RPOS", "2", NULL);
	for (int i = 0; i < NUM_RREFS; i++) {
		ASSERT(rref_names[i] != NULL);
		send_dref_req(rref_names[i], i, RREF_FREQ);
	}
}

static void
disable_data(void)
{
	send_data_cmd(NULL, 0, "RPOS", "0", NULL);
	for (int i = 0; i < NUM_RREFS; i++) {
		ASSERT(rref_names[i] != NULL);
		send_dref_req(rref_names[i], i, 0);
	}
}

bool
xpintf_init(const char *host, int port)
{
	struct addrinfo hints = {
	    .ai_family = AF_UNSPEC,
	    .ai_socktype = SOCK_DGRAM,
	    .ai_protocol = IPPROTO_UDP
	};
	int result;
	char hostbuf[PATH_MAX], portbuf[8];

	ASSERT(!inited);
	inited = true;
	mutex_init(&lock);

	if (host != NULL)
		cpdlc_strlcpy(hostbuf, host, sizeof (hostbuf));
	else
		cpdlc_strlcpy(hostbuf, "localhost", sizeof (hostbuf));
	snprintf(portbuf, sizeof (portbuf), "%d",
	    port != 0 ? port : DEFAULT_XP_PORT);

	result = getaddrinfo(host, portbuf, &hints, &ai);
	if (result != 0) {
		const char *error = CPDLC_GAI_STRERROR(result);
		logMsg("Can't resolve %s: %s\n", hostbuf, error);
		goto errout;
	}
	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (!CPDLC_SOCKET_IS_VALID(sock)) {
		logMsg("Error creating socket: %s",
		    cpdlc_get_last_socket_error());
		goto errout;
	}
	if (!cpdlc_set_fd_nonblock(sock)) {
		logMsg("Cannot set socket as non-blocking: %s",
		    cpdlc_get_last_socket_error());
		goto errout;
	}

	return (true);
errout:
	xpintf_fini();
	return (false);
}

void
xpintf_fini(void)
{
	if (!inited)
		return;
	inited = false;

	if (CPDLC_SOCKET_IS_VALID(sock) && ai != NULL)
		disable_data();
	if (CPDLC_SOCKET_IS_VALID(sock)) {
		close(sock);
		sock = CPDLC_INVALID_SOCKET;
	}
	if (ai != NULL) {
		freeaddrinfo(ai);
		ai = NULL;
	}
	mutex_destroy(&lock);
}

static void
process_rpos_data(const rpos_data_t *buf)
{
	ASSERT(buf != NULL);

	mutex_enter(&lock);
	rpos_data_recv_t = time(NULL);
	rpos_data = *buf;
	mutex_exit(&lock);
}

static void
process_rref_data(const void *inbuf, size_t sz)
{
	ASSERT(inbuf != NULL);
	ASSERT0(sz % 8);

	rref_data_recv_t = now;
	for (unsigned off = 0; off + 8 <= sz; off += 8) {
		int32_t idx = *(int32_t *)(inbuf + off);

		if (idx >= NUM_RREFS) {
			logMsg("Invalid RREF index %d at offset %d", idx, off);
			continue;
		}
		memcpy(&rref_data[idx * sizeof (int32_t)],
		    inbuf + off + sizeof (int32_t), sizeof (int32_t));
	}
}

static void
drain_input(void)
{
	uint8_t buf[4096];
	ssize_t sz;

	ASSERT(CPDLC_SOCKET_IS_VALID(sock));
	sz = recv(sock, buf, sizeof (buf), 0);
	if (sz == -1) {
		logMsg("recv() error: %s", cpdlc_get_last_socket_error());
	} else if (sz == sizeof (rpos_data_t) && CHECK_FOURCC(buf, "RPOS")) {
		process_rpos_data((const rpos_data_t *)buf);
	} else if (sz >= 5 && (sz - 5) % 8 == 0 && CHECK_FOURCC(buf, "RREF")) {
		process_rref_data(&buf[5], sz - 5);
	}
}

void
xpintf_update(void)
{
	int res;

	if (!inited)
		return;

	ASSERT(CPDLC_SOCKET_IS_VALID(sock));

	now = microclock();
	if (now > last_req_t + DATA_REQ_INTVAL) {
		enable_data();
		last_req_t = now;
	}
	do {
		struct timeval tv = {};
		fd_set rfds;

		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		res = select(sock + 1, &rfds, NULL, NULL, &tv);
		if (res == 1) {
			drain_input();
		} else if (res == -1) {
			logMsg("Error calling select() on UDP socket: %s",
			    cpdlc_get_last_socket_error());
		}
	} while (res == 1);
}

bool
xpintf_get_time(unsigned *hours, unsigned *mins)
{
	ASSERT(hours != NULL);
	ASSERT(mins != NULL);

	mutex_enter(&lock);
	mutex_exit(&lock);

	return (false);
}

bool
xpintf_get_sat(int *temp_C)
{
	float temp;
	ASSERT(temp_C != NULL);
	if (!rref_read_fp32(RREF_SAT, &temp))
		return (false);
	*temp_C = round(temp);
	return (true);
}

bool
xpintf_get_cur_spd(bool *mach, unsigned *spd)
{
	int32_t is_mach;
	float spd_f32;

	ASSERT(mach != NULL);
	ASSERT(spd != NULL);

	if (rref_read_int32(RREF_IS_MACH, &is_mach)) {
		*mach = is_mach;
		if (is_mach && rref_read_fp32(RREF_MACH, &spd_f32)) {
			*spd = round(spd_f32 * 100);
			return (true);
		} else if (!is_mach && rref_read_fp32(RREF_IAS, &spd_f32)) {
			*spd = round(spd_f32);
			return (true);
		}
	}
	return (false);
}

float
xpintf_get_cur_alt(void)
{
	float alt_ft;

	if (rref_read_fp32(RREF_ALT, &alt_ft))
		return (round(alt_ft / 10.0) * 10.0);
	return (NAN);
}

float
xpintf_get_cur_vvi(void)
{
	float vvi_fpm;
	if (rref_read_fp32(RREF_VVI, &vvi_fpm))
		return (round(vvi_fpm / 50.0) * 50.0);
	return (NAN);
}

float
xpintf_get_sel_alt(void)
{
	float alt_ft;
	if (rref_read_fp32(RREF_ALT_SEL, &alt_ft))
		return (round(alt_ft / 100.0) * 100.0);
	return (NAN);
}

float
xpintf_get_offset(void)
{
	float hdef_dots, nm_per_dot;
	if (rref_read_fp32(RREF_GPS_HDEF_DOT, &hdef_dots) &&
	    rref_read_fp32(RREF_GPS_NM_PER_DOT, &nm_per_dot)) {
		return (-hdef_dots * nm_per_dot);
	}
	return (NAN);
}

bool
xpintf_get_wind(unsigned *deg_true, unsigned *knots)
{
	ASSERT(deg_true != NULL);
	ASSERT(knots != NULL);
	return (false);
}
