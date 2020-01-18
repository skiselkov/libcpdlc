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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <cpdlc_assert.h>
#include <cpdlc_core.h>
#include "../common/cpdlc_config_common.h"

#include <acfutils/conf.h>

#if	APL || LIN
#include <unistd.h>	/* for getopt */
#endif

#include "main.h"

cpdlc_client_t *client = NULL;
cpdlc_msglist_t *msglist = NULL;

static void
log_dbg_string(const char *str)
{
	fputs(str, stderr);
}

static void
print_usage(FILE *fp, const char *progname)
{
	CPDLC_ASSERT(fp != NULL);
	CPDLC_ASSERT(progname != NULL);
	fprintf(fp, "Usage: %s [-h]\n", progname);
}

static void
print_glfw_error(int error, const char* description)
{
	UNUSED(error);
	fprintf(stderr, "GLFW error: %s\n", description);
}

static void
assfail_trip(const char *filename, int line, const char *msg, void *userinfo)
{
	UNUSED(userinfo);
	fprintf(stderr, "%s:%d: %s\n", filename, line, msg);
	abort();
}

static bool
parse_config(const char *confpath)
{
	conf_t *conf;
	int errline = 0;

	CPDLC_ASSERT(confpath != NULL);

	conf = conf_read_file(confpath, &errline);
	if (conf == NULL) {
		logMsg("Can't read config file %s:%d", confpath, errline);
		goto errout;
	}

	conf_free(conf);
	return (true);
errout:
	if (conf != NULL)
		conf_free(conf);
	return (false);
}

int
main(int argc, char **argv)
{
	int opt;
	const char *confpath = "./cpdlc_atc.cfg";

	cpdlc_assfail = assfail_trip;
	log_init(log_dbg_string, "cpdlc_atc");

	while ((opt = getopt(argc, argv, "hc:")) != -1) {
		switch (opt) {
		case 'h':
			print_usage(stdout, argv[0]);
			break;
		case 'c':
			confpath = optarg;
			break;
		default:
			logMsg("Unknown option %c. Try -h for help.\n", opt);
			return (1);
		}
	}

	if (!glfwInit()) {
		logMsg("Can't initialize GLFW\n");
		return (1);
	}
	glfwSetErrorCallback(print_glfw_error);

	if (gl3wInit()) {
		glfwTerminate();
		logMsg("Can't initialize GL3W\n");
		return (1);
	}

	if (!parse_config(confpath)) {
	}

	glfwTerminate();

	return (0);
}
