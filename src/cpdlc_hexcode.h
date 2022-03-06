/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * CDDL HEADER END
*/
/*
 * Copyright 2022 Saso Kiselkov. All rights reserved.
 */

#ifndef	_LIBCPDLC_HEXCODE_H_
#define	_LIBCPDLC_HEXCODE_H_

#include <stdbool.h>
#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

void cpdlc_hex_enc(const void *in_raw, size_t len, void *out_enc,
    size_t out_cap);
bool cpdlc_hex_dec(const void *in_enc, size_t len, void *out_raw,
    size_t out_cap);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_HEXCODE_H_ */
