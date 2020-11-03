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

#include <curl/curl.h>

#include <acfutils/safe_alloc.h>
#include <acfutils/taskq.h>
#include <acfutils/time.h>

#include "../src/cpdlc_string.h"

#include "common.h"
#include "msg_router.h"
#include "rpc.h"

#define	DFL_NUM_THREADS_MAX	8
#define	DFL_NUM_THREADS_MIN	0
#define	DFL_THR_STOP_DELAY	SEC2USEC(2)
#define	ROUTER_TIMEOUT		10L		/* secs */

typedef struct {
	char		addr[SOCKADDR_STRLEN];
	bool		is_atc;
	bool		is_lws;
	cpdlc_msg_t	*msg;
	char		to[CALLSIGN_LEN];
	msg_router_cb_t	fwd_cb;
	msg_router_cb_t	discard_cb;
	void		*userinfo;
} msg_routing_info_t;

typedef struct {
	CURL		*curl;
} router_t;

typedef enum {
	PARAM_FROM,
	PARAM_TO,
	PARAM_STA_TYPE,
	PARAM_CONN_TYPE,
	PARAM_ADDR
} param_t;
enum { MAX_PARAMS = 8 };

static struct {
	taskq_t		*tq;
	rpc_spec_t	spec;
} rpc = {};

static void *
router_init(void *userinfo)
{
	router_t *rt = safe_calloc(1, sizeof (*rt));

	UNUSED(userinfo);

	rt->curl = curl_easy_init();
	ASSERT(rt->curl != NULL);
	rpc_curl_setup(rt->curl, &rpc.spec);

	return (rt);
}

static void
router_fini(void *userinfo, void *thr_info)
{
	router_t *rt;

	UNUSED(userinfo);
	ASSERT(thr_info != NULL);
	rt = thr_info;
	ASSERT(rt->curl != NULL);

	curl_easy_cleanup(rt->curl);
	free(rt);
}

static void
router_proc(void *userinfo, void *thr_info, void *task)
{
	rpc_result_t result;
	router_t *rt;
	msg_routing_info_t *mri;
	char msgtype[8], min[8], mrn[8];

	UNUSED(userinfo);
	ASSERT(thr_info != NULL);
	rt = thr_info;
	ASSERT(task != NULL);
	mri = task;
	ASSERT(mri->fwd_cb != NULL);
	ASSERT(mri->discard_cb != NULL);

	snprintf(msgtype, sizeof (msgtype), "%s%d",
	    mri->msg->segs[0].info->is_dl ? "DM" : "UM",
	    mri->msg->segs[0].info->msg_type);
	snprintf(min, sizeof (min), "%d", cpdlc_msg_get_min(mri->msg));
	snprintf(mrn, sizeof (mrn), "%d", cpdlc_msg_get_mrn(mri->msg));
	if (rpc_perform(&rpc.spec, &result, rt->curl,
	    "FROM", cpdlc_msg_get_from(mri->msg),
	    "TO", mri->to,
	    "STATYPE", mri->is_atc ? "ATC" : "ACFT",
	    "CONNTYPE", mri->is_lws ? "WS" : "TLS",
	    "ADDR", mri->addr,
	    "MSGTYPE", msgtype,
	    "MIN", min,
	    "MRN", mrn,
	    NULL)) {
		cpdlc_msg_set_to(mri->msg, result.values[0]);
		mri->fwd_cb(mri->msg, mri->addr, mri->userinfo);
	} else {
		mri->discard_cb(mri->msg, mri->addr, mri->userinfo);
	}
	cpdlc_msg_free(mri->msg);
	free(mri);
}

static void
router_discard(void *userinfo, void *task)
{
	msg_routing_info_t *mri;

	UNUSED(userinfo);
	ASSERT(task != NULL);
	mri = task;
	cpdlc_msg_free(mri->msg);
	free(mri);
}

static bool
rpc_router_init(const conf_t *conf)
{
	const char *str;
	int min_threads = DFL_NUM_THREADS_MIN;
	int max_threads = DFL_NUM_THREADS_MAX;
	uint64_t stop_delay = DFL_THR_STOP_DELAY;

	/*
	 * If the required RPC keys are missing, the user doesn't want
	 * to have dynamic routing enabled.
	 */
	if (!conf_get_str(conf, "msg_router/rpc/style", &str) &&
	    !conf_get_str(conf, "msg_router/rpc/url", &str)) {
		return (true);
	}
	if (!rpc_spec_parse(conf, "msg_router/rpc", &rpc.spec))
		return (false);

	conf_get_i(conf, "msg_router/min_threads", &min_threads);
	conf_get_i(conf, "msg_router/max_threads", &max_threads);
	max_threads = MAX(max_threads, 1);
	min_threads = MIN(min_threads, max_threads);
	if (conf_get_lli(conf, "msg_router/stop_delay",
	    (long long *)&stop_delay)) {
		stop_delay = SEC2USEC(stop_delay);
	}
	rpc.tq = taskq_alloc(min_threads, max_threads, stop_delay,
	    router_init, router_fini, router_proc, router_discard, NULL);

	return (true);
}

bool
msg_router_init(const conf_t *conf)
{
	if (conf != NULL && !rpc_router_init(conf))
		return (false);
	return (true);
}

void
msg_router_fini(void)
{
	if (rpc.tq != NULL)
		taskq_free(rpc.tq);
	memset(&rpc, 0, sizeof (rpc));
}

void
msg_router(const char *conn_addr, bool is_atc, bool is_lws,
    cpdlc_msg_t *msg, const char *to, msg_router_cb_t fwd_cb,
    msg_router_cb_t discard_cb, void *userinfo)
{
	ASSERT(conn_addr != NULL);
	ASSERT(msg != NULL);
	ASSERT(to != NULL);
	ASSERT(fwd_cb != NULL);
	ASSERT(discard_cb != NULL);

	if (rpc.tq == NULL) {
		/* RPC router not initialized, just pass the message through */
		cpdlc_msg_set_to(msg, to);
		fwd_cb(msg, conn_addr, userinfo);
		cpdlc_msg_free(msg);
	} else {
		msg_routing_info_t *mri = safe_calloc(1, sizeof (*mri));

		cpdlc_strlcpy(mri->addr, conn_addr, sizeof (mri->addr));
		mri->is_atc = is_atc;
		mri->is_lws = is_lws;
		mri->msg = msg;
		cpdlc_strlcpy(mri->to, to, sizeof (mri->to));
		mri->fwd_cb = fwd_cb;
		mri->discard_cb = discard_cb;
		mri->userinfo = userinfo;

		taskq_submit(rpc.tq, mri);
	}
}
