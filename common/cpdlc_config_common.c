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

#include <string.h>

#include "cpdlc_config_common.h"

/*
 * Converts the string value of the "tls/keyfile_enctype" config key into
 * a key encryption type suitable for GnuTLS.
 */
gnutls_pkcs_encrypt_flags_t
cpdlc_config_str2encflags(const char *value)
{
	if (strcmp(value, "3DES") == 0 || strcmp(value, "3des") == 0)
		return (GNUTLS_PKCS_PBES2_3DES);
	if (strcmp(value, "RC4") == 0 || strcmp(value, "rc4") == 0)
		return (GNUTLS_PKCS_PKCS12_ARCFOUR);
	if (strcmp(value, "AES128") == 0 || strcmp(value, "aes128") == 0)
		return (GNUTLS_PKCS_PBES2_AES_128);
	if (strcmp(value, "AES192") == 0 || strcmp(value, "aes192") == 0)
		return (GNUTLS_PKCS_PBES2_AES_192);
	if (strcmp(value, "AES256") == 0 || strcmp(value, "aes256") == 0)
		return (GNUTLS_PKCS_PBES2_AES_256);
	if (strcmp(value, "PKCS12/3DES") == 0 ||
	    strcmp(value, "pkcs12/3des") == 0)
		return (GNUTLS_PKCS_PKCS12_3DES);
	return (GNUTLS_PKCS_PLAIN);
}
