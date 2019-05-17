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

#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fmsbox_req.h"
#include "fmsbox_req_clx.h"
#include "fmsbox_scratchpad.h"
#include "fmsbox_vrfy.h"

static bool
can_verify_clx_req(fmsbox_t *box)
{
	ASSERT(box != NULL);

	return (box->clx_req.clx || (box->clx_req.type != CLX_REQ_NONE &&
	    box->clx_req.proc[0] != '\0'));
}

static void
verify_clx_req(fmsbox_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc();

	if (box->clx_req.clx) {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM25_REQ_PDC, 0);
	} else {
		char buf[16] = { 0 };

		ASSERT(box->clx_req.type != CLX_REQ_NONE);
		ASSERT(box->clx_req.proc[0] != '\0');

		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM23_REQ_proc, 0);
		if (box->clx_req.type == CLX_REQ_ARR ||
		    box->clx_req.type == CLX_REQ_APP) {
			if (box->clx_req.trans[0] != '\0') {
				strncat(buf, box->clx_req.trans,
				    sizeof (buf) - 1);
				strncat(buf, ".", sizeof (buf) - 1);
			}
			strncat(buf, box->clx_req.proc, sizeof (buf) - 1);
		} else {
			strncat(buf, box->clx_req.proc, sizeof (buf) - 1);
			if (box->clx_req.trans[0] != '\0') {
				strncat(buf, ".", sizeof (buf) - 1);
				strncat(buf, box->clx_req.trans,
				    sizeof (buf) - 1);
			}
		}
		cpdlc_msg_seg_set_arg(msg, seg, 0, buf, NULL);
	}
	fmsbox_req_add_common(box, msg);

	fmsbox_verify_msg(box, msg, "CLX REQ", FMS_PAGE_REQ_CLX, true);
}

static void
draw_main_page(fmsbox_t *box)
{
	const char *proc_type = NULL;

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_L1, "PROC TYPE");
	switch (box->clx_req.type) {
	case CLX_REQ_NONE:
		proc_type = "NONE";
		break;
	case CLX_REQ_ARR:
		proc_type = "ARRIVAL";
		break;
	case CLX_REQ_APP:
		proc_type = "APPROACH";
		break;
	case CLX_REQ_DEP:
		proc_type = "DEPARTURE";
		break;
	}
	fmsbox_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
	    FMS_FONT_LARGE, "v%s", proc_type);

	if (box->clx_req.type != CLX_REQ_NONE) {
		fmsbox_put_lsk_title(box, FMS_KEY_LSK_L2, "PROCEDURE");
		fmsbox_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", (box->clx_req.proc[0] != '\0' ?
		    box->clx_req.proc : "______"));

		fmsbox_put_lsk_title(box, FMS_KEY_LSK_L3, "TRANSITION");
		fmsbox_put_str(box, LSK3_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", (box->clx_req.trans[0] != '\0' ?
		    box->clx_req.trans : "-----"));
	}

	fmsbox_put_lsk_title(box, FMS_KEY_LSK_R1, "CLEARANCE");
	fmsbox_put_altn_selector(box, LSK1_ROW, true,
	    !box->clx_req.clx, "YES", "NO", NULL);
}

void
fmsbox_req_clx_draw_cb(fmsbox_t *box)
{
	ASSERT(box != NULL);

	fmsbox_set_num_subpages(box, 2);

	fmsbox_put_page_title(box, "FANS  CLX REQ");
	fmsbox_put_page_ind(box, FMS_COLOR_WHITE);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fmsbox_req_draw_freetext(box);

	if (can_verify_clx_req(box)) {
		fmsbox_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}
	fmsbox_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fmsbox_req_clx_key_cb(fmsbox_t *box, fms_key_t key)
{
	ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		box->clx_req.type = (box->clx_req.type + 1) % (CLX_REQ_DEP + 1);
		box->clx_req.proc[0] = '\0';
		box->clx_req.trans[0] = '\0';
		box->clx_req.clx = false;
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L2 &&
	    box->clx_req.type != CLX_REQ_NONE) {
		fmsbox_scratchpad_xfer(box, box->clx_req.proc,
		    sizeof (box->clx_req.proc), true);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L3 &&
	    box->clx_req.type != CLX_REQ_NONE) {
		fmsbox_scratchpad_xfer(box, box->clx_req.trans,
		    sizeof (box->clx_req.trans), true);
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R1) {
		box->clx_req.clx = !box->clx_req.clx;
		box->clx_req.type = CLX_REQ_NONE;
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_clx_req(box))
			verify_clx_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fmsbox_set_page(box, FMS_PAGE_REQUESTS);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fmsbox_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
