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

#ifndef	_LIBCPDLC_CLIENT_H_
#define	_LIBCPDLC_CLIENT_H_

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
	/* Network socket not connected */
	CPDLC_LOGON_NONE,
	/* Network socket connection in progress on background */
	CPDLC_LOGON_CONNECTING_LINK,
	/* Network socket connection complete, TLS handshake in progress */
	CPDLC_LOGON_HANDSHAKING_LINK,
	/* Secure network connection established, LOGON request not yet sent */
	CPDLC_LOGON_LINK_AVAIL,
	/* LOGON request sent, awaiting response */
	CPDLC_LOGON_IN_PROG,
	/* LOGON complete, ready for operation */
	CPDLC_LOGON_COMPLETE
} cpdlc_client_logon_status_t;

typedef struct cpdlc_client_s cpdlc_client_t;

cpdlc_client_t *cpdlc_client_init(const char *server_hostname,
    int server_port, const char *cafile, bool is_atc);
void cpdlc_client_fini(cpdlc_client *cl);

void cpdlc_client_set_logon_data(cpdlc_client_t *cl, const char *logon_data,
    const char *from, const char *to);

bool cpdlc_client_do_connect_link(cpdlc_client_t *cl);
void cpdlc_client_do_disconnect_link(cpdlc_client_t *cl);
bool cpdlc_client_do_logon(cpdlc_client_t *cl);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_CLIENT_H_ */
