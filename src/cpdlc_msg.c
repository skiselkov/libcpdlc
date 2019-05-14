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

#define	INVALID_MSG_SEQ_NR	UINT32_MAX

static const cpdlc_msg_info_t *
msg_infos_lookup(bool is_dl, int msg_type, char msg_subtype)
{
	const cpdlc_msg_info_t *infos =
	    (is_dl ? cpdlc_dl_infos : cpdlc_ul_infos);

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

unsigned
cpdlc_escape_percent(const char *in_buf, char *out_buf, unsigned cap)
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
		if (isalnum(c) || c == '.' || c == ',') {
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
encode_arg(const cpdlc_arg_type_t arg_type, const cpdlc_arg_t *arg,
    bool readable, unsigned *n_bytes_p, char **buf_p, unsigned *cap_p)
{
	char textbuf[1024];

	switch (arg_type) {
	case CPDLC_ARG_ALTITUDE:
		if (readable) {
			const char *units;
			int value;

			if (arg->alt.fl) {
				if (arg->alt.met) {
					value = arg->alt.alt;
					units = " M";
				} else {
					value = arg->alt.alt / 100;
					units = "";
				}
			} else {
				if (arg->alt.met) {
					value = arg->alt.alt;
					units = " M";
				} else {
					value = arg->alt.alt;
					units = " FT";
				}
			}
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s%d%s",
			    arg->alt.fl ? "FL" : "", value, units);
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s%d%s",
			    arg->alt.fl ? "FL" : "",
			    (arg->alt.fl && !arg->alt.met) ?
			    arg->alt.alt / 100 : arg->alt.alt,
			    arg->alt.met ? "M" : "");
		}
		break;
	case CPDLC_ARG_SPEED:
		if (arg->spd.mach) {
			if (arg->spd.spd < 1000) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%sM.%03d", readable ? "" : " ",
				    arg->spd.spd);
			} else {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%sM%d.%03d", readable ? "" : " ",
				    arg->spd.spd / 1000,
				    arg->spd.spd % 1000);
			}
		} else {
			if (readable) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%dKT", arg->spd.spd);
			} else {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    " %d", arg->spd.spd);
			}
		}
		break;
	case CPDLC_ARG_TIME:
		if (arg->time.hrs < 0) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%sNOW",
			    readable ? "" : " ");
		} else {
			if (readable) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%02d:%02d", arg->time.hrs, arg->time.mins);
			} else {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    " %02d%02d", arg->time.hrs, arg->time.mins);
			}
		}
		break;
	case CPDLC_ARG_POSITION:
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s",
			    arg->pos);
		} else {
			cpdlc_escape_percent(arg->pos, textbuf,
			    sizeof (textbuf));
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s",
			    textbuf);
		}
		break;
	case CPDLC_ARG_DIRECTION:
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s",
			    arg->dir == CPDLC_DIR_ANY ? "" :
			    (arg->dir == CPDLC_DIR_LEFT ? "LEFT" : "RIGHT"));
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %c",
			    arg->dir == CPDLC_DIR_ANY ? 'N' :
			    (arg->dir == CPDLC_DIR_LEFT ? 'L' : 'R'));
		}
		break;
	case CPDLC_ARG_DISTANCE:
		if (readable) {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    "%.01f NM", arg->dist);
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
			if (readable) {
				APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
				    "%s", arg->route);
			} else {
				cpdlc_escape_percent(arg->route, textbuf,
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
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    "%.03f MHZ", arg->freq);
		} else {
			APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p,
			    " %.03f", arg->freq);
		}
		break;
	case CPDLC_ARG_DEGREES:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, "%s%d",
		    readable ? "" : " ", arg->deg);
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
		encode_arg(info->args[i], &seg->args[i], false, n_bytes_p,
		    buf_p, cap_p);
	}
}

cpdlc_msg_t *
cpdlc_msg_alloc(void)
{
	return (safe_calloc(1, sizeof (cpdlc_msg_t)));
}

void
cpdlc_msg_free(cpdlc_msg_t *msg)
{
	ASSERT(msg != NULL);

	for (unsigned i = 0; i < msg->num_segs; i++) {
		cpdlc_msg_seg_t *seg = &msg->segs[i];

		free(msg->logon_data);
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
	if (msg->is_logon) {
		char textbuf[64];
		ASSERT(msg->logon_data != NULL);
		cpdlc_escape_percent(msg->logon_data, textbuf,
		    sizeof (textbuf));
		APPEND_SNPRINTF(n_bytes, buf, cap, "/LOGON=%s", textbuf);
	}
	if (msg->from[0] != '\0') {
		char textbuf[32];
		cpdlc_escape_percent(msg->from, textbuf, sizeof (textbuf));
		APPEND_SNPRINTF(n_bytes, buf, cap, "/FROM=%s", textbuf);
	}
	if (msg->to[0] != '\0') {
		char textbuf[32];
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
	char textbuf[512];

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
			ASSERT3U(arg, ==, info->num_args);
			break;
		}

		encode_arg(info->args[arg], &seg->args[arg], true, n_bytes_p,
		    buf_p, cap_p);
		/*
		 * If the argument was at the end of the text line, `start'
		 * can now be pointing past `end'. So we must NOT deref it
		 * before it gets re-validated at the top of this loop.
		 */
		start = close + 1;
		if (info->args[arg] == CPDLC_ARG_DIRECTION &&
		    seg->args[arg].dir == CPDLC_DIR_ANY) {
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
			APPEND_SNPRINTF(n_bytes, buf, cap, " ");
	}

	return (n_bytes);
}

static bool
validate_logon_message(const cpdlc_msg_t *msg)
{
	if (msg->num_segs != 0) {
		fprintf(stderr, "Message malformed: LOGON messages "
		    "may not contain MSG segments\n");
		return (false);
	}
	if (msg->from[0] == '\0' && strcmp(msg->logon_data, "FAILURE") != 0) {
		fprintf(stderr, "Message malformed: LOGON messages "
		    "MUST contain a FROM header\n");
		return (false);
	}
	if (msg->min == INVALID_MSG_SEQ_NR) {
		fprintf(stderr, "Message malformed: missing or invalid "
		    "MIN header\n");
		return (false);
	}
	return (true);
}

static bool
validate_message(const cpdlc_msg_t *msg)
{
	/* LOGON message format is special */
	if (msg->is_logon)
		return (validate_logon_message(msg));

	if (msg->num_segs == 0) {
		fprintf(stderr, "Message malformed: no message segments found\n");
		return (false);
	}
	if (msg->min == INVALID_MSG_SEQ_NR) {
		fprintf(stderr, "Message malformed: missing or invalid "
		    "MIN header\n");
		return (false);
	}
	if (msg->mrn == INVALID_MSG_SEQ_NR) {
		fprintf(stderr, "Message malformed: missing or invalid "
		    "MRN header\n");
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

	info = msg_infos_lookup(is_dl, msg_type, msg_subtype);
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
				if (sscanf(&start[2], "%d",
				    &arg->alt.alt) != 1 ||
				    arg->alt.alt <= 0) {
					fprintf(stderr, "Malformed message: "
					    "invalid flight level\n");
					return (false);
				}
			} else {
				arg->alt.alt = atoi(start);
				if (sscanf(start, "%d", &arg->alt.alt) != 1 ||
				    arg->alt.alt < -1500 ||
				    arg->alt.alt > 100000) {
					fprintf(stderr, "Malformed message: "
					    "invalid altitude\n");
					return (false);
				}
			}
			arg_end = find_arg_end(start, end);
			if (*(arg_end - 1) == 'M')
				arg->alt.met = true;
			/* Multiply non-metric FLs by 100 */
			if (arg->alt.fl && !arg->alt.met)
				arg->alt.alt *= 100;
			/* Second round of validation for FLs */
			if (arg->alt.fl &&
			    ((!arg->alt.met && arg->alt.alt > 100000) ||
			    (arg->alt.met && arg->alt.alt > 30000))) {
				fprintf(stderr, "Malformed message: "
				    "invalid flight level\n");
				return (false);
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
				char hrs[3] = { 0 }, mins[3] = { 0 };

				arg_end = find_arg_end(start, end);

				if (arg_end - start != 4) {
					fprintf(stderr, "Malformed message: "
					    "invalid time\n");
					return (false);
				}
				hrs[0] = start[0];
				hrs[1] = start[1];
				mins[0] = start[2];
				mins[1] = start[3];
				if (sscanf(hrs, "%d", &arg->time.hrs) != 1 ||
				    sscanf(mins, "%d", &arg->time.mins) != 1 ||
				    arg->time.hrs < 0 || arg->time.hrs > 23 ||
				    arg->time.mins < 0 || arg->time.mins > 59) {
					fprintf(stderr, "Malformed message: "
					    "invalid time\n");
					return (false);
				}
			}
			break;
		case CPDLC_ARG_POSITION:
			arg_end = find_arg_end(start, end);
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(arg_end - start) + 1));
			if (cpdlc_unescape_percent(textbuf, arg->pos,
			    sizeof (arg->pos)) == -1) {
				fprintf(stderr, "Malformed message: "
				    "invalid percent escapes\n");
				return (false);
			}
			if (contains_spaces(arg->pos)) {
				fprintf(stderr, "Malformed message: position "
				    "cannot contain whitespace\n");
				return (false);
			}
			break;
		case CPDLC_ARG_DIRECTION:
			switch (start[0]) {
			case 'N':
				if (is_hold(is_dl, msg_type) ||
				    is_offset(is_dl, msg_type)) {
					fprintf(stderr, "Malformed message: "
					    "this message type cannot specify "
					    "a direction of 'ANY'.\n");
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
			if (arg->vvi < 0 || arg->vvi > 10000) {
				fprintf(stderr, "Malformed message: invalid "
				    "VVI (%d)\n", arg->vvi);
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
				fprintf(stderr, "Malformed message: invalid "
				    "TO/FROM flag\n");
				return (false);
			}
			break;
		case CPDLC_ARG_ROUTE:
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(end - start) + 1));
			l = cpdlc_unescape_percent(textbuf, NULL, 0);
			if (l == -1) {
				fprintf(stderr, "Malformed message: "
				    "invalid percent escapes\n");
				return (false);
			}
			if (arg->route != NULL)
				free(arg->route);
			arg->route = malloc(l + 1);
			cpdlc_unescape_percent(textbuf, arg->route, l + 1);
			start = end;
			break;
		case CPDLC_ARG_PROCEDURE:
			arg_end = find_arg_end(start, end);
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(arg_end - start) + 1));
			if (cpdlc_unescape_percent(textbuf, arg->proc,
			    sizeof (arg->proc)) == -1) {
				fprintf(stderr, "Malformed message: "
				    "invalid percent escapes\n");
				return (false);
			}
			if (contains_spaces(arg->pos)) {
				fprintf(stderr, "Malformed message: procedure "
				    "name cannot contain whitespace\n");
				return (false);
			}
			break;
		case CPDLC_ARG_SQUAWK:
			if (sscanf(start, "%d", &arg->squawk) != 1 ||
			    !is_valid_squawk(arg->squawk)) {
				fprintf(stderr, "Malformed message: "
				    "invalid squawk code\n");
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
				fprintf(stderr, "Malformed message: "
				    "invalid percent escapes\n");
				return (false);
			}
			if (contains_spaces(arg->icaoname.icao)) {
				fprintf(stderr, "Malformed message: icaoname "
				    "cannot contain whitespace\n");
				return (false);
			}
			/* Followed by whitespace */
			SKIP_NONSPACE(start, end);
			SKIP_SPACE(start, end);
			if (start == end) {
				fprintf(stderr, "Malformed message: icaoname "
				    "must be followed by descriptive name\n");
				return (false);
			}
			/* And finally a percent-escaped descriptive name */
			arg_end = find_arg_end(start, end);
			cpdlc_strlcpy(textbuf, start, MIN(sizeof (textbuf),
			    (uintptr_t)(arg_end - start) + 1));
			if (cpdlc_unescape_percent(textbuf, arg->icaoname.name,
			    sizeof (arg->icaoname.name)) == -1) {
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
			l = cpdlc_unescape_percent(textbuf, NULL, 0);
			if (l == -1) {
				fprintf(stderr, "Malformed message: "
				    "invalid percent escapes\n");
				return (false);
			}
			free(arg->freetext);
			arg->freetext = malloc(l + 1);
			cpdlc_unescape_percent(textbuf, arg->freetext, l + 1);
			start = end;
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
	if (start != end) {
		fprintf(stderr, "Malformed message: too much data in "
		    "message\n");
		return (false);
	}

	return (true);
}

bool
cpdlc_msg_decode(const char *in_buf, cpdlc_msg_t **msg_p, int *consumed)
{
	const char *start, *term;
	cpdlc_msg_t *msg;
	bool pkt_is_cpdlc = false;
	bool skipped_cr = false;

	ASSERT(in_buf != NULL);
	ASSERT(msg_p != NULL);
	ASSERT(consumed != NULL);

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
		*consumed = 0;
		return (true);
	}

	msg = safe_calloc(1, sizeof (*msg));
	msg->min = INVALID_MSG_SEQ_NR;
	msg->mrn = INVALID_MSG_SEQ_NR;

	start = in_buf;
	while (in_buf < term) {
		const char *sep = strchr(in_buf, '/');

		if (sep == NULL || sep > term)
			sep = term;
		if (strncmp(in_buf, "PKT=", 4) == 0) {
			if (strncmp(&in_buf[4], "CPDLC/", 6) != 0) {
				fprintf(stderr, "Message malformed: invalid "
				    "PKT type\n");
				goto errout;
			}
			pkt_is_cpdlc = true;
		} else if (strncmp(in_buf, "MIN=", 4) == 0) {
			if (sscanf(&in_buf[4], "%u", &msg->min) != 1) {
				fprintf(stderr, "Message malformed: invalid "
				    "MIN value\n");
				goto errout;
			}
		} else if (strncmp(in_buf, "MRN=", 4) == 0) {
			if (sscanf(&in_buf[4], "%u", &msg->mrn) != 1) {
				fprintf(stderr, "Message malformed: invalid "
				    "MRN value\n");
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
				fprintf(stderr, "Message malformed: too many "
				    "message segments\n");
				goto errout;
			}
			seg = &msg->segs[msg->num_segs];
			if (!msg_decode_seg(seg, &in_buf[4], sep))
				goto errout;
			if (msg->num_segs > 0 && msg->segs[0].info->is_dl !=
			    seg->info->is_dl) {
				fprintf(stderr, "Message malformed: can't "
				    "mix DM and UM message segments\n");
				goto errout;
			}
			msg->num_segs++;
		} else {
			fprintf(stderr, "Message malformed: unknown message "
			    "header\n");
			goto errout;
		}

		in_buf = sep + 1;
	}

	if (!pkt_is_cpdlc || !validate_message(msg))
		goto errout;

	*msg_p = msg;
	*consumed = ((term - start) + 1 + (skipped_cr ? 1 : 0));
	return (true);
errout:
	free(msg);
	*msg_p = NULL;
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
	ASSERT(msg->segs[0].info != NULL);
	return (msg->segs[0].info->is_dl);
}

void
cpdlc_msg_set_min(cpdlc_msg_t *msg, unsigned min)
{
	ASSERT(msg != NULL);
	ASSERT(min != INVALID_MSG_SEQ_NR);
	msg->min = min;
}

unsigned
cpdlc_msg_get_min(const cpdlc_msg_t *msg)
{
	ASSERT(msg != NULL);
	return (msg->min);
}

void
cpdlc_msg_set_mrn(cpdlc_msg_t *msg, unsigned mrn)
{
	ASSERT(msg != NULL);
	ASSERT(mrn != INVALID_MSG_SEQ_NR);
	msg->mrn = mrn;
}

unsigned
cpdlc_msg_get_mrn(const cpdlc_msg_t *msg)
{
	ASSERT(msg != NULL);
	return (msg->mrn);
}

const char *
cpdlc_msg_get_logon_data(const cpdlc_msg_t *msg)
{
	ASSERT(msg != NULL);
	return (msg->logon_data);
}

void
cpdlc_msg_set_logon_data(cpdlc_msg_t *msg, const char *logon_data)
{
	ASSERT(msg != NULL);
	free(msg->logon_data);
	msg->logon_data = strdup(logon_data);
	msg->is_logon = true;
}

unsigned
cpdlc_msg_get_num_segs(const cpdlc_msg_t *msg)
{
	ASSERT(msg != NULL);
	return (msg->num_segs);
}

int
cpdlc_msg_add_seg(cpdlc_msg_t *msg, bool is_dl, unsigned msg_type,
    unsigned char msg_subtype)
{
	cpdlc_msg_seg_t *seg;

	ASSERT(msg != NULL);
	if (!is_dl) {
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
	ASSERT_MSG(msg->num_segs == 0 || msg->segs[0].info->is_dl == is_dl,
	    "Can't mix DM and UM message segments in a single message %p", msg);
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
		ASSERT(arg_val2 != NULL);
		cpdlc_strlcpy(arg->icaoname.icao, arg_val1,
		    sizeof (arg->icaoname.icao));
		cpdlc_strlcpy(arg->icaoname.name, arg_val2,
		    sizeof (arg->icaoname.name));
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
		cpdlc_strlcpy(arg_val1, arg->icaoname.icao, str_cap);
		cpdlc_strlcpy(arg_val2, arg->icaoname.name, str_cap);
		return (strlen(arg->icaoname.name));
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
