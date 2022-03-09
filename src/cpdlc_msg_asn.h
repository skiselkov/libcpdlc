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

#ifndef	_LIBCPDLC_CPDLC_MSG_ASN_H_
#define	_LIBCPDLC_CPDLC_MSG_ASN_H_

#include "cpdlc_msg.h"

#ifdef	__cplusplus
extern "C" {
#endif

bool cpdlc_msg_encode_asn_impl(const cpdlc_msg_t *msg, unsigned *n_bytes_p,
    char **buf_p, unsigned *cap_p);
bool cpdlc_msg_decode_asn_impl(cpdlc_msg_t *msg, const void *struct_ptr,
    bool is_dl);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_CPDLC_MSG_ASN_H_ */
