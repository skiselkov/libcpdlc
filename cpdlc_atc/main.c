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

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <cpdlc_assert.h>
#include <cpdlc_core.h>

#if	APL || LIN
#include <unistd.h>	/* for getopt */
#endif

static void
print_usage(FILE *fp, const char *progname)
{
	ASSERT(fp != NULL);
	ASSERT(progname != NULL);
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

int
main(int argc, char **argv)
{
	int opt;

	cpdlc_assfail = assfail_trip;

	while ((opt = getopt(argc, argv, "h")) != -1) {
		switch (opt) {
		case 'h':
			print_usage(stdout, argv[0]);
			break;
		default:
			fprintf(stderr, "Unknown option %c. Try -h for help.\n",
			    opt);
			return (1);
		}
	}

	if (!glfwInit()) {
		fprintf(stderr, "Can't initialize GLFW\n");
		return (1);
	}
	glfwSetErrorCallback(print_glfw_error);

	if (gl3wInit()) {
		glfwTerminate();
		fprintf(stderr, "Can't initialize GL3W\n");
		return (1);
	}

	glfwTerminate();

	return (0);
}
