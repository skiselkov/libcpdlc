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

#ifndef	_CPDLC_CONFIG_COMMON_H_
#define	_CPDLC_CONFIG_COMMON_H_

#include <stdbool.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define	CPDLC_HOSTNAME_MAX_LEN	128

gnutls_pkcs_encrypt_flags_t cpdlc_config_str2encflags(const char *value);
bool cpdlc_config_str2hostname_port(const char *name_port,
    char hostname[CPDLC_HOSTNAME_MAX_LEN], int *port, bool is_lws);

#ifdef	__cplusplus
}
#endif

#endif	/* _CPDLC_CONFIG_COMMON_H_ */
