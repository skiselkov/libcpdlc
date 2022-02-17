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

#include <string.h>

#include "../src/cpdlc_assert.h"

#include "fans_req.h"
#include "fans_req_clx.h"
#include "fans_scratchpad.h"
#include "fans_vrfy.h"

static bool
can_verify_clx_req(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	return (box->clx_req.clx || (box->clx_req.type != CLX_REQ_NONE &&
	    box->clx_req.proc[0] != '\0'));
}

static void
verify_clx_req(fans_t *box)
{
	int seg = 0;
	cpdlc_msg_t *msg = cpdlc_msg_alloc(CPDLC_PKT_CPDLC);

	if (box->clx_req.clx) {
		seg = cpdlc_msg_add_seg(msg, true, CPDLC_DM25_REQ_PDC, 0);
	} else {
		char buf[16] = { 0 };

		CPDLC_ASSERT(box->clx_req.type != CLX_REQ_NONE);
		CPDLC_ASSERT(box->clx_req.proc[0] != '\0');

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
	fans_req_add_common(box, msg);

	fans_verify_msg(box, msg, "CLX REQ", FMS_PAGE_REQ_CLX, true);
}

static void
draw_main_page(fans_t *box)
{
	const char *proc_type = NULL;

	fans_put_lsk_title(box, FMS_KEY_LSK_L1, "PROC TYPE");
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
	fans_put_str(box, LSK1_ROW, 0, false, FMS_COLOR_GREEN,
	    FMS_FONT_LARGE, "v%s", proc_type);

	if (box->clx_req.type != CLX_REQ_NONE) {
		fans_put_lsk_title(box, FMS_KEY_LSK_L2, "PROCEDURE");
		fans_put_str(box, LSK2_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", (box->clx_req.proc[0] != '\0' ?
		    box->clx_req.proc : "______"));

		fans_put_lsk_title(box, FMS_KEY_LSK_L3, "TRANSITION");
		fans_put_str(box, LSK3_ROW, 0, false, FMS_COLOR_WHITE,
		    FMS_FONT_LARGE, "%s", (box->clx_req.trans[0] != '\0' ?
		    box->clx_req.trans : "-----"));
	}

	fans_put_lsk_title(box, FMS_KEY_LSK_R1, "CLEARANCE");
	fans_put_altn_selector(box, LSK1_ROW, true,
	    !box->clx_req.clx, "YES", "NO", NULL);
}

void
fans_req_clx_init_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);
	memset(&box->clx_req, 0, sizeof (box->clx_req));
}

void
fans_req_clx_draw_cb(fans_t *box)
{
	CPDLC_ASSERT(box != NULL);

	fans_set_num_subpages(box, 2);

	fans_put_page_title(box, "FANS  CLX REQ");
	fans_put_page_ind(box);

	if (box->subpage == 0)
		draw_main_page(box);
	else
		fans_req_draw_freetext(box);

	if (can_verify_clx_req(box)) {
		fans_put_lsk_action(box, FMS_KEY_LSK_L5, FMS_COLOR_WHITE,
		    "<VERIFY");
	}
	fans_put_lsk_action(box, FMS_KEY_LSK_L6, FMS_COLOR_WHITE, "<RETURN");
}

bool
fans_req_clx_key_cb(fans_t *box, fms_key_t key)
{
	bool read_back;

	CPDLC_ASSERT(box != NULL);

	if (box->subpage == 0 && key == FMS_KEY_LSK_L1) {
		box->clx_req.type = (box->clx_req.type + 1) % (CLX_REQ_DEP + 1);
		box->clx_req.proc[0] = '\0';
		box->clx_req.trans[0] = '\0';
		box->clx_req.clx = false;
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L2 &&
	    box->clx_req.type != CLX_REQ_NONE) {
		if (fans_scratchpad_xfer(box, box->clx_req.proc,
		    sizeof (box->clx_req.proc), true, &read_back) &&
		    !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_L3 &&
	    box->clx_req.type != CLX_REQ_NONE) {
		if (fans_scratchpad_xfer(box, box->clx_req.trans,
		    sizeof (box->clx_req.trans), true, &read_back) &&
		    !read_back) {
			fans_scratchpad_clear(box);
		}
	} else if (box->subpage == 0 && key == FMS_KEY_LSK_R1) {
		box->clx_req.clx = !box->clx_req.clx;
		box->clx_req.type = CLX_REQ_NONE;
	} else if (key == FMS_KEY_LSK_L5) {
		if (can_verify_clx_req(box))
			verify_clx_req(box);
	} else if (key == FMS_KEY_LSK_L6) {
		fans_set_page(box, FMS_PAGE_REQUESTS, true);
	} else if (KEY_IS_REQ_FREETEXT(box, key, 1)) {
		fans_req_key_freetext(box, key);
	} else {
		return (false);
	}

	return (true);
}
