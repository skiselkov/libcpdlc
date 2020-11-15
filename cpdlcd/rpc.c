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
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "../src/cpdlc_string.h"

#include "rpc.h"

#define	RPC_TIMEOUT		10
#define	REALLOC_STEP		(16 << 10)	/* 16 KiB */
#define	MAX_DL_SIZE		(128 << 10)	/* 128 KiB */

typedef struct {
	const rpc_spec_t	*spec;
	uint8_t			*buf;
	size_t			bufcap;
	size_t			bufsz;
} dl_info_t;

/*
 * cURL data download write callback (CURLOPT_WRITEFUNCTION).
 * This is used to grab all incoming server response data.
 */
static size_t
dl_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	dl_info_t *dl_info = userdata;
	size_t bytes = size * nmemb;

	ASSERT(dl_info != NULL);

	/*
	 * Protect against the server flooding us with a giant response.
	 * This is mostly to protect against somebody accidentally
	 * pointing the URL at something like a giant file download.
	 */
	if (dl_info->bufsz + bytes > MAX_DL_SIZE) {
		logMsg("%s has returned too much data (%ld bytes), bailing out",
		    dl_info->spec->url, (long)(dl_info->bufsz + bytes));
		return (0);
	}
	if (dl_info->bufcap < dl_info->bufsz + bytes + 1) {
		dl_info->bufcap += REALLOC_STEP;
		dl_info->buf = safe_realloc(dl_info->buf, dl_info->bufcap);
	}
	memcpy(&dl_info->buf[dl_info->bufsz], ptr, bytes);
	dl_info->bufsz += bytes;
	/* Make sure the buffer is properly NUL-terminated */
	dl_info->buf[dl_info->bufsz] = '\0';

	return (bytes);
}

bool
rpc_spec_parse(const conf_t *conf, const char *prefix, rpc_spec_t *spec)
{
	const char *str;
	bool debug;

	ASSERT(conf != NULL);
	ASSERT(prefix != NULL);
	ASSERT(spec != NULL);

	memset(spec, 0, sizeof (*spec));
	spec->timeout = RPC_TIMEOUT;

	if (!conf_get_str_v(conf, "%s/style", &str, prefix)) {
		logMsg("Missing config key %s/style is required when using RPC",
		    prefix);
		return (false);
	}
	if (strcmp(str, "www-form") == 0) {
		spec->style = RPC_STYLE_WWW_FORM;
	} else if (strcmp(str, "xmlrpc") == 0) {
		spec->style = RPC_STYLE_XML_RPC;
	} else {
		logMsg("Bad value for %s/style = %s, must be \"www-form\" or "
		    "\"xmlrpc\"", prefix, str);
		return (false);
	}
	if (!conf_get_str_v(conf, "%s/url", &str, prefix)) {
		logMsg("Missing config key %s/url is required when using RPC",
		    prefix);
		return (false);
	}
	cpdlc_strlcpy(spec->url, str, sizeof (spec->url));
	if (spec->style == RPC_STYLE_XML_RPC) {
		if (!conf_get_str_v(conf, "%s/methodName", &str, prefix)) {
			logMsg("Missing config key %s/methodName is required "
			    "when %s/style is set to \"xmlrpc\"",
			    prefix, prefix);
			return (false);
		}
		cpdlc_strlcpy(spec->methodName, str, sizeof (spec->methodName));
	}
	for (int i = 0; i < RPC_MAX_PARAMS; i++) {
		if (!conf_get_str_v(conf, "%s/param/%d", &str, prefix, i))
			break;
		cpdlc_strlcpy(spec->params[i], str, sizeof (spec->params[i]));
		spec->num_params = i + 1;
	}
	if (conf_get_b2_v(conf, "%s/debug", &debug, prefix) && debug)
		spec->debug = debug;
	if (conf_get_str_v(conf, "%s/username", &str, prefix))
		cpdlc_strlcpy(spec->username, str, sizeof (spec->username));
	if (conf_get_str_v(conf, "%s/password", &str, prefix))
		cpdlc_strlcpy(spec->password, str, sizeof (spec->password));
	if (conf_get_str(conf, "cainfo", &str))
		cpdlc_strlcpy(spec->cainfo, str, sizeof (spec->cainfo));
	conf_get_i_v(conf, "%s/timeout", (int *)&spec->timeout, prefix);

	return (true);
}

void
rpc_curl_setup(CURL *curl, const rpc_spec_t *spec)
{
	ASSERT(curl != NULL);
	ASSERT(spec != NULL);

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, spec->timeout);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dl_write);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, spec->url);
	if (spec->cainfo[0] != '\0')
		curl_easy_setopt(curl, CURLOPT_CAINFO, spec->cainfo);
	if (spec->username[0] != '\0')
		curl_easy_setopt(curl, CURLOPT_USERNAME, spec->username);
	if (spec->password[0] != '\0')
		curl_easy_setopt(curl, CURLOPT_PASSWORD, spec->password);
}

static bool
req_prepare_www_form(const rpc_spec_t *spec, CURL *curl, va_list ap)
{
	char *buf = NULL;
	size_t bufcap = 0;

	for (unsigned i = 0; i < spec->num_params; i++) {
		va_list ap2;

		va_copy(ap2, ap);
		for (const char *name = va_arg(ap2, const char *);;
		    name = va_arg(ap2, const char *)) {
			const char *value;
			if (name == NULL) {
				logMsg("RPC configuration error: parameter "
				    "%s not valid", spec->params[i]);
				va_end(ap2);
				goto errout;
			}
			value = va_arg(ap2, const char *);
			if (strcmp(spec->params[i], name) == 0) {
				char *name_enc, *value_enc;

				name_enc = curl_easy_escape(curl, name, 0);
				value_enc = curl_easy_escape(curl, value, 0);
				append_format(&buf, &bufcap, "%s%s=%s",
				    i == 0 ? "" : "&", name_enc, value_enc);
				curl_free(name_enc);
				curl_free(value_enc);
				break;
			}
		}
		va_end(ap2);
	}
	curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, buf);
	free(buf);
	return (true);
errout:
	free(buf);
	return (false);
}

static bool
resp_parse_www_form(const char *buf, CURL *curl, rpc_result_t *result)
{
	char **comps;
	size_t n_comps;

	ASSERT(buf != NULL);
	ASSERT(curl != NULL);
	ASSERT(result != NULL);

	comps = strsplit(buf, "&", true, &n_comps);
	if (n_comps == 0)
		goto errout;

	result->num_results = n_comps;
	for (size_t i = 0; i < n_comps; i++) {
		char *value_unenc, *value;

		value = strchr(comps[i], '=');
		/*
		 * If the value lacks a key=value pair, just treat the
		 * entire string as a non-named return value.
		 */
		if (value != NULL) {
			char *key_unenc;

			*value = '\0';
			value++;

			key_unenc = curl_easy_unescape(curl, comps[i], 0, NULL);
			cpdlc_strlcpy(result->names[i], key_unenc,
			    sizeof (result->names[i]));
			curl_free(key_unenc);
		} else {
			value = comps[i];
		}
		value_unenc = curl_easy_unescape(curl, value, 0, NULL);
		cpdlc_strlcpy(result->values[i], value_unenc,
		    sizeof (result->values[i]));
		curl_free(value_unenc);
	}
	free_strlist(comps, n_comps);
	return (true);
errout:
	free_strlist(comps, n_comps);
	return (false);
}

static bool
req_prepare_xml_rpc(const rpc_spec_t *spec, CURL *curl, va_list ap)
{
	xmlDocPtr doc;
	xmlNodePtr methodCall, params;
	xmlChar *buf;
	int bufsz;

	ASSERT(spec != NULL);
	ASSERT(curl != NULL);

	doc = xmlNewDoc((xmlChar *)XML_DEFAULT_VERSION);
	methodCall = xmlNewDocNode(doc, NULL, (xmlChar *)"methodCall", NULL);
	xmlDocSetRootElement(doc, methodCall);
	(void)xmlNewChild(methodCall, NULL, (xmlChar *)"methodName",
	    (xmlChar *)spec->methodName);
	params = xmlNewChild(methodCall, NULL, (xmlChar *)"params", NULL);

	for (unsigned i = 0; i < spec->num_params; i++) {
		va_list ap2;

		va_copy(ap2, ap);
		for (const char *name = va_arg(ap2, const char *);;
		    name = va_arg(ap2, const char *)) {
			const char *value;

			if (name == NULL) {
				logMsg("RPC configuration error: parameter "
				    "%s not valid", spec->params[i]);
				va_end(ap2);
				goto errout;
			}
			value = va_arg(ap2, const char *);
			if (strcmp(spec->params[i], name) == 0) {
				xmlNodePtr param;

				param = xmlNewChild(params, NULL,
				    (xmlChar *)"param", NULL);
				xmlNewChild(param, NULL, (xmlChar *)"value",
				    (xmlChar *)value);
				break;
			}
		}
		va_end(ap2);
	}
	xmlDocDumpMemory(doc, &buf, &bufsz);
	curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, buf);
	xmlFree(buf);
	xmlFreeDoc(doc);

	return (true);
errout:
	free(buf);
	return (false);
}

static bool
resp_parse_xml_rpc(const char *buf, rpc_result_t *result)
{
	xmlDoc *doc = NULL;
	xmlXPathContext *xpath_ctx = NULL;
	xmlXPathObject *result_obj = NULL;

	ASSERT(buf != NULL);
	ASSERT(result != NULL);
	memset(result, 0, sizeof (*result));

	doc = xmlParseDoc((const xmlChar *)buf);
	if (doc == NULL) {
		logMsg("Error parsing RPC server response: XML parsing error");
		goto errout;
	}
	xpath_ctx = xmlXPathNewContext(doc);
	if (xpath_ctx == NULL) {
		logMsg("Error parsing server response: failed to create "
		    "XPath context for document");
		goto errout;
	}
#define	PARAM_VALUE	"/methodResponse/params/param/value"
	result_obj = xmlXPathEvalExpression((xmlChar *)
	    "("
	    PARAM_VALUE "/string/text()|"
	    PARAM_VALUE "/int/text()|"
	    PARAM_VALUE "/i4/text()|"
	    PARAM_VALUE "/boolean/text()|"
	    PARAM_VALUE "/double/text()|"
	    PARAM_VALUE "/dateTime.iso8601/text()|"
	    PARAM_VALUE "/base64/text()|"
	    PARAM_VALUE "/text()[normalize-space(.) != '']|"
	    PARAM_VALUE "/array/data/value/string/text()|"
	    PARAM_VALUE "/array/data/value/int/text()|"
	    PARAM_VALUE "/array/data/value/i4/text()|"
	    PARAM_VALUE "/array/data/value/boolean/text()|"
	    PARAM_VALUE "/array/data/value/double/text()|"
	    PARAM_VALUE "/array/data/value/dateTime.iso8601/text()|"
	    PARAM_VALUE "/array/data/value/base64/text()|"
	    PARAM_VALUE "/array/data/value/text()[normalize-space(.) != '']"
	    ")[not(./*)]",
	    xpath_ctx);
#undef	PARAM_VALUE
	if (result_obj != NULL && result_obj->nodesetval != NULL &&
	    result_obj->nodesetval->nodeNr != 0) {
		for (int i = 0; i < result_obj->nodesetval->nodeNr &&
		    i < RPC_MAX_PARAMS; i++) {
			result->num_results = i + 1;
			cpdlc_strlcpy(result->values[i], (const char *)
			    result_obj->nodesetval->nodeTab[i]->content,
			    sizeof (result->values[i]));
		}
	} else {
		logMsg("Error parsing server response: XML-RPC response "
		    "contained an error");
	}
	if (result_obj != NULL)
		xmlXPathFreeObject(result_obj);
	xmlXPathFreeContext(xpath_ctx);
	xmlFreeDoc(doc);

	return (result->num_results != 0);
errout:
	if (xpath_ctx != NULL)
		xmlXPathFreeContext(xpath_ctx);
	if (doc != NULL)
		xmlFreeDoc(doc);
	return (false);
}

bool
rpc_perform(const rpc_spec_t *spec, rpc_result_t *rpc_res, CURL *curl, ...)
{
	struct curl_slist *hdrs;
	bool ret;
	dl_info_t dl_info = { .spec = spec };
	CURLcode curl_res;
	long code = 0;
	va_list ap;

	ASSERT(spec != NULL);
	ASSERT(curl != NULL);
	ASSERT(rpc_res != NULL);

	va_start(ap, curl);
	switch (spec->style) {
	case RPC_STYLE_WWW_FORM:
		hdrs = curl_slist_append(NULL,
		    "Content-Type: application/x-www-form-urlencoded");
		ret = req_prepare_www_form(spec, curl, ap);
		break;
	case RPC_STYLE_XML_RPC:
		hdrs = curl_slist_append(NULL, "Content-Type: application/xml");
		ret = req_prepare_xml_rpc(spec, curl, ap);
		break;
	default:
		VERIFY_FAIL();
	}
	va_end(ap);
	if (!ret) {
		curl_slist_free_all(hdrs);
		return (false);
	}
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dl_info);

	curl_res = curl_easy_perform(curl);
	if (curl_res == CURLE_OK)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

	if (curl_res == CURLE_OK && code == 200 && dl_info.bufsz != 0) {
		if (spec->debug) {
			printf("RPC debug response:\n%s",
			    (const char *)dl_info.buf);
		}
		switch (spec->style) {
		case RPC_STYLE_WWW_FORM:
			ret = resp_parse_www_form((const char *)dl_info.buf,
			    curl, rpc_res);
			break;
		case RPC_STYLE_XML_RPC:
			ret = resp_parse_xml_rpc((const char *)dl_info.buf,
			    rpc_res);
			break;
		default:
			VERIFY_FAIL();
		}
	} else {
		if (curl_res != CURLE_OK) {
			logMsg("RPC error %s: %s", spec->url,
			    curl_easy_strerror(curl_res));
		} else if (dl_info.bufsz == 0) {
			logMsg("RPC error %s: no data in response", spec->url);
		} else {
			logMsg("RPC error %s: HTTP error %ld", spec->url, code);
		}
		ret = false;
	}
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
	curl_slist_free_all(hdrs);

	return (ret);
}
