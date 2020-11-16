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

#ifndef	_LIBCPDLC_RPC_H_
#define	_LIBCPDLC_RPC_H_

#include <stdbool.h>
#include <stdlib.h>
#include <curl/curl.h>

#include <acfutils/conf.h>
#include <acfutils/sysmacros.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define	RPC_MAX_PARAMS		32
#define	RPC_MAX_PARAM_NAME_LEN	128

typedef enum {
	RPC_STYLE_WWW_FORM,
	RPC_STYLE_XML_RPC
} rpc_style_t;

typedef struct {
	bool		debug;
	rpc_style_t	style;
	bool		need_return;
	char		url[PATH_MAX];
	char		methodName[128];
	unsigned	num_params;
	unsigned	timeout;
	char		params[RPC_MAX_PARAMS][RPC_MAX_PARAM_NAME_LEN];
	char		username[64];
	char		password[64];
	char		cainfo[PATH_MAX];
} rpc_spec_t;

typedef struct {
	unsigned	num_results;
	char		names[RPC_MAX_PARAMS][RPC_MAX_PARAM_NAME_LEN];
	char		values[RPC_MAX_PARAMS][RPC_MAX_PARAM_NAME_LEN];
} rpc_result_t;

bool rpc_spec_parse(const conf_t *conf, const char *prefix, bool need_return,
    rpc_spec_t *spec);
void rpc_curl_setup(CURL *curl, const rpc_spec_t *spec);
bool rpc_perform(const rpc_spec_t *spec, rpc_result_t *result,
    CURL *curl, ...) SENTINEL_ATTR;

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_RPC_H_ */
