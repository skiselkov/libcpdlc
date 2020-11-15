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

#ifndef	_CPDLCD_AUTH_H_
#define	_CPDLCD_AUTH_H_

#include <stdbool.h>
#include <stdint.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <acfutils/conf.h>

#include "../src/cpdlc_msg.h"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * This is an asynchronous logon authenticator. To begin an authentication
 * session, first call auth_sess_open, passing the details of the logon
 * message and the connection identity. The authenticator fires up a
 * background thread that contacts the authentication URL (as set in
 * `auth_init'). Once a response is received, the authenticator calls a
 * callback with the result. Alternatively, an authentication session can
 * be terminated early with a call to auth_sess_kill.
 */

typedef uint64_t auth_sess_key_t;
typedef void (*auth_done_cb_t)(bool result, bool is_atc, void *userinfo);

bool auth_init(const conf_t *conf);
void auth_fini(void);

auth_sess_key_t auth_sess_open(const cpdlc_msg_t *logon_msg,
    const char *remote_addr, auth_done_cb_t done_cb, void *userinfo);
void auth_sess_kill(auth_sess_key_t key);

bool auth_encrypt_userpwd(bool silent);

#ifdef	__cplusplus
}
#endif

#endif	/* _CPDLCD_AUTH_H_ */
