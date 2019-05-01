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
#include <stdio.h>
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
		escape_percent(arg->route, textbuf, sizeof (textbuf));
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s", textbuf);
		break;
	case CPDLC_ARG_PROCEDURE:
		escape_percent(arg->proc, textbuf, sizeof (textbuf));
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %s", textbuf);
		break;
	case CPDLC_ARG_SQUAWK:
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %d", arg->squawk);
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
		APPEND_SNPRINTF(*n_bytes_p, *buf_p, *cap_p, " %d", arg->baro);
		break;
	case CPDLC_ARG_FREETEXT:
		escape_percent(arg->freetext, textbuf, sizeof (textbuf));
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
cpdlc_msg_alloc(const char *from, const char *to, unsigned min, unsigned mrn)
{
	cpdlc_msg_t *msg = safe_calloc(1, sizeof (*msg));

	cpdlc_strlcpy(msg->from, from, sizeof (msg->from));
	cpdlc_strlcpy(msg->to, to, sizeof (msg->to));
	msg->min = min;
	msg->mrn = mrn;

	return (msg);
}

void
cpdlc_msg_free(cpdlc_msg_t *msg)
{
	free(msg);
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
	const cpdlc_msg_info_t *infos;

	ASSERT(msg != NULL);
	if (is_dl) {
		ASSERT3U(msg_type, <=, CPDLC_UL182_CONFIRM_ATIS_CODE);
		ASSERT0(msg_subtype);
	} else {
		ASSERT3U(msg_type, <=, CPDLC_DL80_DEVIATING_dir_dist_OF_ROUTE);
		if (msg_subtype != 0) {
			ASSERT3U(msg_subtype, >=,
			    CPDLC_DL67b_WE_CAN_ACPT_alt_AT_time);
			ASSERT3U(msg_subtype, <=,
			    CPDLC_DL67i_WHEN_CAN_WE_EXPCT_DES_TO_alt);
		}
	}
	if (msg->num_segs >= CPDLC_MAX_MSG_SEGS)
		return (-1);
	seg = &msg->segs[msg->num_segs];

	infos = (is_dl ? cpdlc_ul_infos : cpdlc_dl_infos);
	for (int i = 0; infos[i].msg_type != -1; i++) {
		const cpdlc_msg_info_t *info = &infos[i];

		if (info->is_dl == is_dl && info->msg_type == msg_type &&
		    info->msg_subtype == msg_subtype) {
			seg->info = info;
			break;
		}
	}
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
		arg->baro = *(unsigned *)arg_val1;
		break;
	case CPDLC_ARG_FREETEXT:
		cpdlc_strlcpy(arg->freetext, arg_val1, sizeof (arg->freetext));
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
		*(unsigned *)arg_val1 = arg->baro;
		return (sizeof (arg->baro));
	case CPDLC_ARG_FREETEXT:
		ASSERT(arg_val1 != NULL || str_cap == 0);
		cpdlc_strlcpy(arg_val1, arg->freetext, str_cap);
		return (strlen(arg->freetext));
	}
	VERIFY_MSG(0, "Message %p segment %d (%d/%d/%d) contains invalid "
	    "argument %d type %x", msg, seg_nr, info->is_dl, info->msg_type,
	    info->msg_subtype, arg_nr, info->args[arg_nr]);
}

unsigned
cpdlc_msg_encode(const cpdlc_msg_t *msg, char *buf, unsigned cap)
{
	unsigned n_bytes = 0;
	char to[32], from[32];

	escape_percent(msg->to, to, sizeof (to));
	escape_percent(msg->from, from, sizeof (from));

	APPEND_SNPRINTF(n_bytes, buf, cap, "PKT=CPDLC/FROM=%s/TO=%s/MIN=%d/"
	    "MRN=%d/SEGS=%d", from, to, msg->min, msg->mrn, msg->num_segs);

	for (unsigned i = 0; i < msg->num_segs; i++)
		encode_seg(&msg->segs[i], &n_bytes, &buf, &cap);
	APPEND_SNPRINTF(n_bytes, buf, cap, "\n");

	return (n_bytes);
}

int
cpdlc_msg_decode(const char *in_buf, cpdlc_msg_t *msg)
{
	const char *terminator;

	UNUSED(unescape_percent);

	ASSERT(in_buf != NULL);
	ASSERT(msg != NULL);

	terminator = strchr(in_buf, '\n');
	if (terminator == NULL) {
		/* No complete message in buffer */
		return (0);
	}

	return ((terminator - in_buf) + 1);
}
