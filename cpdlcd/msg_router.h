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

#ifndef	_LIBCPDLC_MSG_ROUTER_H_
#define	_LIBCPDLC_MSG_ROUTER_H_

#include <stdbool.h>

#include <acfutils/conf.h>

#include "../src/cpdlc_msg.h"
#include "common.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef void (*msg_router_cb_t)(const cpdlc_msg_t *msg, const char *to,
    void *userinfo);

bool msg_router_init(const conf_t *conf);
void msg_router_fini(void);

void msg_router(const char *conn_addr, bool is_atc, bool is_lws,
    cpdlc_msg_t *msg, const char *to, msg_router_cb_t cb, void *userinfo);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_MSG_ROUTER_H_ */
