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
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "cpdlc_alloc.h"
#include "cpdlc_assert.h"
#include "cpdlc_string.h"
#include "cpdlc.h"

#define	APPEND_SNPRINTF(__total_bytes, __bufptr, __bufcap, ...) \
	do { \
		int __wanted_bytes = snprintf((__bufptr), (__bufcap), \
		    __VA_ARGS__); \
		int __avail_bytes = (__wanted_bytes <= (int)(__bufcap) ? \
		    (__bufcap) - __wanted_bytes : (__bufcap)); \
		ASSERT(__wanted_bytes >= 0); \
		(__bufptr) += __avail_bytes; \
		(__bufcap) -= __avail_bytes; \
		(__total_bytes) += __wanted_bytes; \
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

static const cpdlc_msg_info_t *
msg_infos_lookup(bool is_dl, int msg_type, char msg_subtype)
{
	const cpdlc_msg_info_t *infos =
	    (is_dl ? cpdlc_ul_infos : cpdlc_dl_infos);

	ASSERT3S(msg_type, >=, 0);
	if (is_dl)
		ASSERT3S(msg_type, <=, CPDLC_DM80_DEVIATING_dir_dist_OF_ROUTE);
	else
		ASSERT3S(msg_type, <=, CPDLC_UM182_CONFIRM_ATIS_CODE);
	if (is_dl && msg_type == 67) {
		ASSERT(msg_subtype == 0 ||
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

static unsigned
escape_percent(const char *in_buf, char *out_buf, unsigned cap)
{
	ASSERT(in_buf != NULL);

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
		if (isalnum(c) || c == ' ' || c == '.' || c == ',') {
			out_buf[j] = in_buf[j];
		} else {
			if (j + 4 < cap)
				snprintf(&out_buf[j], 4, "%%%02x", c);
			j += 2;
		}
	}
}

static int
unescape_percent(const char *in_buf, char *out_buf, unsigned cap)
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
			    sscanf(buf, "%02x", &v) != 1)
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
encode_arg(const cpdlc_arg_type_t arg_type, const cpdlc_arg_t *arg,
    unsigned *n_bytes_p, char **buf_p, unsigned *cap_p)
{
	char textbuf[1024];

	switch (arg_type) {
	case CPDLC_ARG_ALTITUDE:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s%d",
		    arg->alt.fl ? "FL" : "",
		    arg->alt.fl ? arg->alt.alt / 100 : arg->alt.alt);
		break;
	case CPDLC_ARG_SPEED:
		if (arg->spd.mach) {
			if (arg->spd.spd < 1000) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    " M.%03d", arg->spd.spd);
			} else {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    " M%d.%03d", arg->spd.spd / 1000,
				    arg->spd.spd % 1000);
			}
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %d",
			    arg->spd.spd);
		}
		break;
	case CPDLC_ARG_TIME:
		if (arg->time.hrs < 0) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " NOW");
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    " %02d:%02d", arg->time.hrs, arg->time.mins);
		}
		break;
	case CPDLC_ARG_POSITION:
		escape_percent(arg->pos, textbuf, sizeof (textbuf));
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s", textbuf);
		break;
	case CPDLC_ARG_DIRECTION:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %c",
		    arg->dir == CPDLC_DIR_ANY ? 'N' :
		    (arg->dir == CPDLC_DIR_LEFT ? 'L' : 'R'));
		break;
	case CPDLC_ARG_DISTANCE:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %.01f",
		    arg->dist);
		break;
	case CPDLC_ARG_VVI:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %d", arg->vvi);
		break;
	case CPDLC_ARG_TOFROM:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s",
		    arg->tofrom ? "TO" : "FROM");
		break;
	case CPDLC_ARG_ROUTE:
		if (arg->route != NULL)
			escape_percent(arg->route, textbuf, sizeof (textbuf));
		else
			*textbuf = '\0';
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s", textbuf);
		break;
	case CPDLC_ARG_PROCEDURE:
		escape_percent(arg->proc, textbuf, sizeof (textbuf));
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s", textbuf);
		break;
	case CPDLC_ARG_SQUAWK:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %04d",
		    arg->squawk);
		break;
	case CPDLC_ARG_ICAONAME:
		escape_percent(arg->icaoname, textbuf, sizeof (textbuf));
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s", textbuf);
		break;
	case CPDLC_ARG_FREQUENCY:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %.02f",
		    arg->freq);
		break;
	case CPDLC_ARG_DEGREES:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %d", arg->deg);
		break;
	case CPDLC_ARG_BARO:
		if (arg->baro.hpa) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " Q%.0f",
			    arg->baro.val);
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " A%.02f",
			    arg->baro.val);
		}
		break;
	case CPDLC_ARG_FREETEXT:
		if (arg->freetext != NULL) {
			escape_percent(arg->freetext, textbuf,
			    sizeof (textbuf));
		} else {
			*textbuf = '\0';
		}
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s", textbuf);
		break;
	}
}

static void
encode_seg(const cpdlc_msg_seg_t *seg, unsigned *n_bytes_p, char **buf_p,
    unsigned *cap_p)
{
	const cpdlc_msg_info_t *info = seg->info;

	APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "/DATA=%s%d",
	    info->is_dl ? "DM" : "UM", info->msg_type);
	if (info->msg_subtype != 0) {
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%c",
		    info->msg_subtype);
	}

	for (unsigned i = 0; i < info->num_args; i++) {
		encode_arg(info->args[i], &seg->args[i], n_bytes_p, buf_p,
		    cap_p);
	}
}

cpdlc_msg_t *
cpdlc_msg_alloc(unsigned min, unsigned mrn)
{
	cpdlc_msg_t *msg = safe_calloc(1, sizeof (*msg));

	ASSERT3U(min, <=, CPDLC_MAX_MSG_SEQ_NR);
	ASSERT3U(mrn, <=, CPDLC_MAX_MSG_SEQ_NR);

	msg->min = min;
	msg->mrn = mrn;

	return (msg);
}

void
cpdlc_msg_free(cpdlc_msg_t *msg)
{
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

unsigned
cpdlc_msg_encode(const cpdlc_msg_t *msg, char *buf, unsigned cap)
{
	unsigned n_bytes = 0;

	APPEND_SNPRINTF(n_bytes, buf, cap, "PKT=CPDLC/MIN=%d/MRN=%d",
	    msg->min, msg->mrn);

	for (unsigned i = 0; i < msg->num_segs; i++)
		encode_seg(&msg->segs[i], &n_bytes, &buf, &cap);
	APPEND_SNPRINTF(n_bytes, buf, cap, "\n");

	return (n_bytes);
}

static bool
validate_message(const cpdlc_msg_t *msg)
{
	if (msg->num_segs == 0) {
		fprintf(stderr, "Message malformed: no DATA segments found\n");
		return (false);
	}
	if (msg->min == 0) {
		fprintf(stderr, "Message malformed: missing MIN header\n");
		return (false);
	}
	if (msg->min == 0) {
		fprintf(stderr, "Message malformed: missing MIN header\n");
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
msg_decode_seg(cpdlc_msg_seg_t *seg, const char *start, const char *end)
{
	bool is_dl;
	int msg_type;
	char msg_subtype = 0;
	unsigned num_args = 0;
	const cpdlc_msg_info_t *info;
	char textbuf[512];

	if (strncmp(start, "DM", 2) == 0) {
		is_dl = true;
	} else if (strncmp(start, "UM", 2) == 0) {
		is_dl = false;
	} else {
		fprintf(stderr, "Malformed message: invalid segment letters\n");
		return (false);
	}
	start += 2;
	if (start >= end) {
		fprintf(stderr, "Malformed message: segment too short\n");
		return (false);
	}
	msg_type = atoi(start);
	if (msg_type < 0 ||
	    (is_dl && msg_type > CPDLC_DM80_DEVIATING_dir_dist_OF_ROUTE) ||
	    (!is_dl && msg_type > CPDLC_UM182_CONFIRM_ATIS_CODE)) {
		fprintf(stderr, "Malformed message: invalid message type\n");
		return (false);
	}
	while (start < end && isdigit(start[0]))
		start++;
	if (start >= end) {
		seg->info = msg_infos_lookup(is_dl, msg_type, 0);
		if (seg->info == NULL) {
			fprintf(stderr, "Malformed message: invalid message "
			    "type\n");
			return (false);
		}
		goto end;
	}
	if (!isspace(start[0])) {
		if (!is_dl || msg_type != 67) {
			fprintf(stderr, "Malformed message: only DM67 can "
			    "have a subtype suffix\n");
			return (false);
		}
		msg_subtype = start[0];
		if (msg_subtype < CPDLC_DM67b_WE_CAN_ACPT_alt_AT_time ||
		    msg_subtype > CPDLC_DM67i_WHEN_CAN_WE_EXPCT_DES_TO_alt) {
			fprintf(stderr, "Malformed message: invalid DM67 "
			    "subtype (%c)\n", msg_subtype);
			return (false);
		}
		start++;
	}

	info = msg_infos_lookup(is_dl, msg_type, 0);
	if (info == NULL) {
		fprintf(stderr, "Malformed message: invalid message type\n");
		return (false);
	}
	seg->info = info;

	if (!isspace(start[0])) {
		fprintf(stderr, "Malformed message: expected space after "
		    "message type\n");
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
			if (start + 2 < end && start[0] == 'F' &&
			    start[1] == 'L') {
				arg->alt.fl = true;
				arg->alt.alt = atoi(&start[2]) * 100;
				if (seg->args[num_args].alt.alt <= 0) {
					fprintf(stderr, "Malformed message: "
					    "invalid flight level\n");
					return (false);
				}
			} else {
				arg->alt.alt = atoi(start);
				if (arg->alt.alt < -2000 ||
				    arg->alt.alt > 100000) {
					fprintf(stderr, "Malformed message: "
					    "invalid altitude\n");
					return (false);
				}
			}
			break;
		case CPDLC_ARG_SPEED:
			if (start + 1 < end && start[0] == 'M') {
				arg->spd.mach = true;
				arg->spd.spd = round(atof(&start[1]) * 1000);
				if (arg->spd.spd < 100) {
					fprintf(stderr, "Malformed message: "
					    "invalid Mach\n");
					return (false);
				}
			} else {
				arg->spd.spd = atoi(start);
				if (arg->spd.spd < 0) {
					fprintf(stderr, "Malformed message: "
					    "invalid airspeed\n");
					return (false);
				}
			}
			break;
		case CPDLC_ARG_TIME:
			if (strncmp(start, "NOW", 3) == 0) {
				arg->time.hrs = -1;
			} else {
				if (sscanf(start, "%02d:%02d", &arg->time.hrs,
				    &arg->time.mins) != 2 ||
				    arg->time.hrs < 0 || arg->time.hrs > 23 ||
				    arg->time.mins < 0 || arg->time.mins > 59) {
					fprintf(stderr, "Malformed message: "
					    "invalid time\n");
					return (false);
				}
			}
			break;
		case CPDLC_ARG_POSITION:
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(end - start) + 1));
			if (unescape_percent(textbuf, arg->pos,
			    sizeof (arg->pos)) == -1) {
				fprintf(stderr, "Malformed message: "
				    "invalid position\n");
				return (false);
			}
			break;
		case CPDLC_ARG_DIRECTION:
			switch (start[0]) {
			case 'N':
				if (is_hold(is_dl, msg_type)) {
					fprintf(stderr, "Malformed message: "
					    "hold legs cannot specify a turn "
					    "direction of 'ANY'.\n");
					return (false);
				}
				arg->dir = CPDLC_DIR_ANY;
				break;
			case 'L':
				arg->dir = CPDLC_DIR_LEFT;
				break;
			case 'R':
				arg->dir = CPDLC_DIR_RIGHT;
				break;
			default:
				fprintf(stderr, "Malformed message: invalid "
				    "direction letter (%c)\n", start[0]);
				return (false);
			}
			break;
		case CPDLC_ARG_DISTANCE:
			arg->dist = atof(start);
			if (arg->dist < 0 || arg->dist > 20000) {
				fprintf(stderr, "Malformed message: invalid "
				    "distance (%.2f)\n", arg->dist);
				return (false);
			}
			break;
		case CPDLC_ARG_VVI:
			arg->vvi = atoi(start);
			if (arg->vvi < -10000 || arg->vvi > 10000) {
				fprintf(stderr, "Malformed message: invalid "
				    "VVI (%d)\n", arg->vvi);
				return (false);
			}
			break;
		case CPDLC_ARG_TOFROM:
			if (strncmp(start, "TO", 2) == 0) {
				arg->tofrom = true;
			} else if (strncmp(start, "FROM", 4) == 0) {
				arg->tofrom = false;
			} else {
				fprintf(stderr, "Malformed message: invalid "
				    "TO/FROM flag\n");
				return (false);
			}
			break;
		case CPDLC_ARG_ROUTE:
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(end - start) + 1));
			l = unescape_percent(textbuf, NULL, 0);
			if (l == -1) {
				fprintf(stderr, "Malformed message: "
				    "invalid percent escapes\n");
				return (false);
			}
			if (arg->route != NULL)
				free(arg->route);
			arg->route = malloc(l + 1);
			unescape_percent(textbuf, arg->route, l + 1);
			break;
		case CPDLC_ARG_PROCEDURE:
			arg_end = find_arg_end(start, end);
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(arg_end - start) + 1));
			if (unescape_percent(textbuf, arg->proc,
			    sizeof (arg->proc)) == -1) {
				fprintf(stderr, "Malformed message: "
				    "invalid percent escapes\n");
				return (false);
			}
			break;
		case CPDLC_ARG_SQUAWK:
			arg->squawk = atoi(start);
			if (!is_valid_squawk(arg->squawk)) {
				fprintf(stderr, "Malformed message: "
				    "invalid squawk code\n");
				return (false);
			}
			break;
		case CPDLC_ARG_ICAONAME:
			arg_end = find_arg_end(start, end);
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(arg_end - start) + 1));
			if (unescape_percent(textbuf, arg->icaoname,
			    sizeof (arg->icaoname)) == -1) {
				fprintf(stderr, "Malformed message: "
				    "invalid percent escapes\n");
				return (false);
			}
			break;
		case CPDLC_ARG_FREQUENCY:
			arg->freq = atof(start);
			if (arg->freq <= 0) {
				fprintf(stderr, "Malformed message: "
				    "invalid frequency\n");
				return (false);
			}
			break;
		case CPDLC_ARG_DEGREES:
			arg->deg = atoi(start);
			if (arg->deg >= 360) {
				fprintf(stderr, "Malformed message: "
				    "invalid heading/track\n");
				return (false);
			}
			break;
		case CPDLC_ARG_BARO:
			if (start + 4 >= end) {
				fprintf(stderr, "Malformed message: baro "
				    "too short\n");
				return (false);
			}
			switch(start[0]) {
			case 'A':
				arg->baro.val = atof(&start[1]);
				if (arg->baro.val < 28 || arg->baro.val > 32) {
					fprintf(stderr, "Malformed message: "
					    "invalid baro value\n");
					return (false);
				}
				break;
			case 'Q':
				arg->baro.val = atoi(&start[1]);
				if (arg->baro.val < 900 ||
				    arg->baro.val > 1100) {
					fprintf(stderr, "Malformed message: "
					    "invalid baro value\n");
					return (false);
				}
				break;
			default:
				fprintf(stderr, "Malformed message: "
				    "invalid baro type (%c)\n", start[0]);
				return (false);
			}
			break;
		case CPDLC_ARG_FREETEXT:
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(end - start) + 1));
			l = unescape_percent(textbuf, NULL, 0);
			if (l == -1) {
				fprintf(stderr, "Malformed message: "
				    "invalid percent escapes\n");
				return (false);
			}
			if (arg->freetext != NULL)
				free(arg->freetext);
			arg->freetext = malloc(l + 1);
			unescape_percent(textbuf, arg->freetext, l + 1);
			break;
		}

		SKIP_NONSPACE(start, end);
		SKIP_SPACE(start, end);
	}

end:
	if (seg->info->num_args != num_args) {
		fprintf(stderr, "Malformed message: invalid number of "
		    "arguments\n");
		return (false);
	}

	return (true);
}

cpdlc_msg_t *
cpdlc_msg_decode(const char *in_buf, int *consumed)
{
	const char *term;
	cpdlc_msg_t *msg;
	bool pkt_is_cpdlc = false;

	ASSERT(in_buf != NULL);
	ASSERT(msg != NULL);

	term = strchr(in_buf, '\n');
	if (term == NULL) {
		/* No complete message in buffer */
		if (consumed != NULL)
			*consumed = 0;
		return (NULL);
	}

	msg = safe_calloc(1, sizeof (*msg));
	msg->min = -1u;
	msg->mrn = -1u;

	while (in_buf < term) {
		const char *sep = strchr(in_buf, '/');

		if (sep == NULL)
			sep = term;
		if (strncmp(in_buf, "PKT=", 4) == 0) {
			if (strncmp(&in_buf[4], "CPDLC/", 6) != 0) {
				fprintf(stderr, "Message malformed: invalid "
				    "PKT type\n");
				goto errout;
			}
			pkt_is_cpdlc = true;
		} else if (strncmp(in_buf, "MIN=", 4) == 0) {
			msg->min = atoi(&in_buf[4]);
			if (msg->min > CPDLC_MAX_MSG_SEQ_NR) {
				fprintf(stderr, "Message malformed: invalid "
				    "MIN value\n");
				goto errout;
			}
		} else if (strncmp(in_buf, "MRN=", 4) == 0) {
			msg->mrn = atoi(&in_buf[4]);
			if (msg->mrn > CPDLC_MAX_MSG_SEQ_NR) {
				fprintf(stderr, "Message malformed: invalid "
				    "MRN value\n");
				goto errout;
			}
		} else if (strncmp(in_buf, "DATA=", 5) == 0) {
			if (msg->num_segs == CPDLC_MAX_MSG_SEGS) {
				fprintf(stderr, "Message malformed: too many "
				    "DATA segments\n");
				goto errout;
			}
			if (!msg_decode_seg(&msg->segs[msg->num_segs],
			    &in_buf[5], sep)) {
				goto errout;
			}
			msg->num_segs++;
		}

		in_buf = sep + 1;
	}

	if (!pkt_is_cpdlc || !validate_message(msg))
		goto errout;

	if (consumed != NULL)
		*consumed = ((term - in_buf) + 1);
	return (msg);
errout:
	free(msg);
	if (consumed != NULL)
		*consumed = 0;
	return (NULL);
}

unsigned
cpdlc_msg_get_num_segs(const cpdlc_msg_t *msg)
{
	ASSERT(msg != NULL);
	return (msg->num_segs);
}

int
cpdlc_msg_add_seg(cpdlc_msg_t *msg, bool is_dl, int msg_type, int msg_subtype)
{
	cpdlc_msg_seg_t *seg;

	ASSERT(msg != NULL);
	if (is_dl) {
		ASSERT3U(msg_type, <=, CPDLC_UM182_CONFIRM_ATIS_CODE);
		ASSERT0(msg_subtype);
	} else {
		ASSERT3U(msg_type, <=, CPDLC_DM80_DEVIATING_dir_dist_OF_ROUTE);
		if (msg_subtype != 0) {
			ASSERT3U(msg_subtype, >=,
			    CPDLC_DM67b_WE_CAN_ACPT_alt_AT_time);
			ASSERT3U(msg_subtype, <=,
			    CPDLC_DM67i_WHEN_CAN_WE_EXPCT_DES_TO_alt);
		}
	}
	if (msg->num_segs >= CPDLC_MAX_MSG_SEGS)
		return (-1);
	seg = &msg->segs[msg->num_segs];

	seg->info = msg_infos_lookup(is_dl, msg_type, msg_subtype);
	ASSERT(seg->info != NULL);

	return (msg->num_segs++);
}

unsigned
cpdlc_msg_seg_get_num_args(const cpdlc_msg_t *msg, unsigned seg_nr)
{
	const cpdlc_msg_seg_t *seg;

	ASSERT(msg != NULL);
	ASSERT3U(seg_nr, <, msg->num_segs);
	seg = &msg->segs[seg_nr];
	ASSERT(seg->info != NULL);

	return (seg->info->num_args);
}

cpdlc_arg_type_t
cpdlc_msg_seg_get_arg_type(const cpdlc_msg_t *msg, unsigned seg_nr,
    unsigned arg_nr)
{
	const cpdlc_msg_seg_t *seg;
	const cpdlc_msg_info_t *info;

	ASSERT(msg != NULL);
	ASSERT3U(seg_nr, <, msg->num_segs);
	seg = &msg->segs[seg_nr];
	ASSERT(seg->info != NULL);
	info = seg->info;
	ASSERT3U(arg_nr, <, info->num_args);

	return (info->args[arg_nr]);
}

void
cpdlc_msg_seg_set_arg(cpdlc_msg_t *msg, unsigned seg_nr, unsigned arg_nr,
    void *arg_val1, void *arg_val2)
{
	const cpdlc_msg_info_t *info;
	cpdlc_msg_seg_t *seg;
	cpdlc_arg_t *arg;

	ASSERT(msg != NULL);
	ASSERT3U(seg_nr, <, msg->num_segs);
	seg = &msg->segs[seg_nr];
	ASSERT(seg->info != NULL);
	info = seg->info;
	ASSERT3U(arg_nr, <, info->num_args);
	arg = &seg->args[arg_nr];

	ASSERT(arg_val1 != NULL);

	switch (info->args[arg_nr]) {
	case CPDLC_ARG_ALTITUDE:
		ASSERT(arg_val2 != NULL);
		arg->alt.fl = *(bool *)arg_val1;
		arg->alt.alt = *(int *)arg_val2;
		break;
	case CPDLC_ARG_SPEED:
		ASSERT(arg_val2 != NULL);
		arg->spd.mach = *(bool *)arg_val1;
		arg->spd.spd = *(int *)arg_val2;
		break;
	case CPDLC_ARG_TIME:
		ASSERT(arg_val2 != NULL);
		arg->time.hrs = *(int *)arg_val1;
		arg->time.mins = *(int *)arg_val2;
		break;
	case CPDLC_ARG_POSITION:
		cpdlc_strlcpy(arg->pos, arg_val1, sizeof (arg->pos));
		break;
	case CPDLC_ARG_DIRECTION:
		arg->dir = *(cpdlc_dir_t *)arg_val1;
		break;
	case CPDLC_ARG_DISTANCE:
		arg->dist = *(double *)arg_val1;
		break;
	case CPDLC_ARG_VVI:
		arg->vvi = *(int *)arg_val1;
		break;
	case CPDLC_ARG_TOFROM:
		arg->tofrom = *(bool *)arg_val1;
		break;
	case CPDLC_ARG_ROUTE:
		cpdlc_strlcpy(arg->route, arg_val1, sizeof (arg->route));
		break;
	case CPDLC_ARG_PROCEDURE:
		cpdlc_strlcpy(arg->proc, arg_val1, sizeof (arg->proc));
		break;
	case CPDLC_ARG_SQUAWK:
		arg->squawk = *(unsigned *)arg_val1;
		break;
	case CPDLC_ARG_ICAONAME:
		cpdlc_strlcpy(arg->icaoname, arg_val1, sizeof (arg->icaoname));
		break;
	case CPDLC_ARG_FREQUENCY:
		arg->freq = *(double *)arg_val1;
		break;
	case CPDLC_ARG_DEGREES:
		arg->deg = *(unsigned *)arg_val1;
		break;
	case CPDLC_ARG_BARO:
		ASSERT(arg_val2 != NULL);
		arg->baro.hpa = *(bool *)arg_val1;
		arg->baro.val = *(double *)arg_val2;
		break;
	case CPDLC_ARG_FREETEXT:
		if (arg->freetext)
			free(arg->freetext);
		arg->freetext = strdup(arg_val1);
		break;
	default:
		VERIFY_MSG(0, "Message %p segment %d (%d/%d/%d) contains "
		    "invalid argument %d type %x", msg, seg_nr, info->is_dl,
		    info->msg_type, info->msg_subtype, arg_nr,
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

	ASSERT(msg != NULL);
	ASSERT3U(seg_nr, <, msg->num_segs);
	seg = &msg->segs[seg_nr];
	ASSERT(seg->info != NULL);
	info = seg->info;
	ASSERT3U(arg_nr, <, info->num_args);
	arg = &seg->args[arg_nr];

	switch (info->args[arg_nr]) {
	case CPDLC_ARG_ALTITUDE:
		ASSERT(arg_val1 != NULL);
		ASSERT(arg_val2 != NULL);
		*(bool *)arg_val1 = arg->alt.fl;
		*(int *)arg_val2 = arg->alt.alt;
		return (sizeof (arg->alt));
	case CPDLC_ARG_SPEED:
		ASSERT(arg_val1 != NULL);
		ASSERT(arg_val2 != NULL);
		*(bool *)arg_val1 = arg->spd.mach;
		*(int *)arg_val2 = arg->spd.spd;
		return (sizeof (arg->spd));
	case CPDLC_ARG_TIME:
		ASSERT(arg_val1 != NULL);
		ASSERT(arg_val2 != NULL);
		*(int *)arg_val1 = arg->time.hrs;
		*(int *)arg_val2 = arg->time.mins;
		return (sizeof (arg->time));
	case CPDLC_ARG_POSITION:
		ASSERT(arg_val1 != NULL || str_cap == 0);
		cpdlc_strlcpy(arg_val1, arg->pos, str_cap);
		return (strlen(arg->pos));
	case CPDLC_ARG_DIRECTION:
		ASSERT(arg_val1 != NULL);
		*(cpdlc_dir_t *)arg_val1 = arg->dir;
		return (sizeof (arg->dir));
	case CPDLC_ARG_DISTANCE:
		ASSERT(arg_val1 != NULL);
		*(double *)arg_val1 = arg->dist;
		return (arg->dist);
	case CPDLC_ARG_VVI:
		ASSERT(arg_val1 != NULL);
		*(int *)arg_val1 = arg->vvi;
		return (arg->vvi);
	case CPDLC_ARG_TOFROM:
		ASSERT(arg_val1 != NULL);
		*(bool *)arg_val1 = arg->tofrom;
		return (arg->tofrom);
	case CPDLC_ARG_ROUTE:
		ASSERT(arg_val1 != NULL || str_cap == 0);
		if (arg->route == NULL) {
			if (str_cap > 0)
				*(char *)arg_val1 = '\0';
			return (0);
		}
		cpdlc_strlcpy(arg_val1, arg->route, str_cap);
		return (strlen(arg->route));
	case CPDLC_ARG_PROCEDURE:
		ASSERT(arg_val1 != NULL || str_cap == 0);
		cpdlc_strlcpy(arg_val1, arg->proc, str_cap);
		return (strlen(arg->proc));
	case CPDLC_ARG_SQUAWK:
		ASSERT(arg_val1 != NULL);
		*(unsigned *)arg_val1 = arg->squawk;
		return (sizeof (arg->squawk));
	case CPDLC_ARG_ICAONAME:
		ASSERT(arg_val1 != NULL || str_cap == 0);
		cpdlc_strlcpy(arg_val1, arg->icaoname, str_cap);
		return (strlen(arg->icaoname));
	case CPDLC_ARG_FREQUENCY:
		ASSERT(arg_val1 != NULL);
		*(double *)arg_val1 = arg->freq;
		return (sizeof (arg->freq));
	case CPDLC_ARG_DEGREES:
		ASSERT(arg_val1 != NULL);
		*(unsigned *)arg_val1 = arg->deg;
		return (sizeof (arg->deg));
	case CPDLC_ARG_BARO:
		ASSERT(arg_val1 != NULL);
		ASSERT(arg_val2 != NULL);
		*(bool *)arg_val1 = arg->baro.hpa;
		*(double *)arg_val2 = arg->baro.val;
		return (sizeof (arg->baro));
	case CPDLC_ARG_FREETEXT:
		ASSERT(arg_val1 != NULL || str_cap == 0);
		if (arg->freetext == NULL) {
			if (str_cap > 0)
				*(char *)arg_val1 = '\0';
			return (0);
		}
		cpdlc_strlcpy(arg_val1, arg->freetext, str_cap);
		return (strlen(arg->freetext));
	}
	VERIFY_MSG(0, "Message %p segment %d (%d/%d/%d) contains invalid "
	    "argument %d type %x", msg, seg_nr, info->is_dl, info->msg_type,
	    info->msg_subtype, arg_nr, info->args[arg_nr]);
}
