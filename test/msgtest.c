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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/cpdlc.h"

static void
handle_buf(const char *buf)
{
	int consumed;
	cpdlc_msg_t *msg = cpdlc_msg_decode(buf, &consumed);

	if (msg != NULL) {
		unsigned l = cpdlc_msg_encode(msg, NULL, 0);
		char *newbuf = malloc(l + 1);

		cpdlc_msg_encode(msg, newbuf, l + 1);
		printf("%s", newbuf);

		free(newbuf);
		cpdlc_msg_free(msg);
	}
}

int
main(void)
{
	char buf[1024];
	char c;

	memset(buf, 0, sizeof (buf));
	printf("> ");
	fflush(stdout);

	while ((c = fgetc(stdin)) != EOF) {
		buf[strlen(buf)] = c;

		if (c == '\n') {
			if (strlen(buf) > 1)
				handle_buf(buf);

			memset(buf, 0, sizeof (buf));
			printf("> ");
			fflush(stdout);
			continue;
		}
	}

	return (0);
}
