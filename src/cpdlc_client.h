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

#include <stdbool.h>
#include <stdint.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "cpdlc_msg.h"

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

typedef enum {
	CPDLC_MSG_STATUS_SENDING,
	CPDLC_MSG_STATUS_SENT,
	CPDLC_MSG_STATUS_SEND_FAILED,
	CPDLC_MSG_STATUS_INVALID_TOKEN
} cpdlc_msg_status_t;

typedef struct cpdlc_client_s cpdlc_client_t;
typedef uint64_t cpdlc_msg_token_t;
#define	CPDLC_INVALID_MSG_TOKEN	((cpdlc_msg_token_t)-1)

cpdlc_client_t *cpdlc_client_init(const char *server_hostname,
    int server_port, const char *cafile, bool is_atc);
void cpdlc_client_fini(cpdlc_client_t *cl);

void cpdlc_client_set_key_file(cpdlc_client_t *cl, const char *keyfile,
    const char *key_pass, gnutls_pkcs_encrypt_flags_t key_enctype,
    const char *certfile);
void cpdlc_client_set_key_mem(cpdlc_client_t *cl, const char *key_pem_data,
    const char *key_pass, gnutls_pkcs_encrypt_flags_t key_enctype,
    const char *cert_pem_data);

void cpdlc_client_logon(cpdlc_client_t *cl, const char *logon_data,
    const char *from, const char *to);
void cpdlc_client_logoff(cpdlc_client_t *cl);

cpdlc_client_logon_status_t cpdlc_client_get_logon_status(
    const cpdlc_client_t *cl);

cpdlc_msg_token_t cpdlc_client_send_msg(cpdlc_client_t *cl,
    const cpdlc_msg_t *msg);
cpdlc_msg_status_t cpdlc_client_get_msg_status(cpdlc_client_t *cl,
    cpdlc_msg_token_t token);

cpdlc_msg_t *cpdlc_client_recv_msg(cpdlc_client_t *cl);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_CLIENT_H_ */
