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

#include <ctype.h>
#include <ncurses.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

#include "../src/cpdlc_alloc.h"
#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_client.h"
#include "../src/cpdlc_msglist.h"
#include "../fmsbox/fmsbox.h"

#define	POLL_INTVAL	1000	/* ms */
#define	DEL_CHAR	0x7f
#define	OFF_X		5
#define	OFF_Y		1

static WINDOW *win = NULL;
static fmsbox_t *fmsbox = NULL;
static bool lsk_right = false;
static int lsk_nr = 0;

static void
handle_signal(int signum)
{
	UNUSED(signum);
	if (fmsbox != NULL)
		fmsbox_free(fmsbox);
	if (win != NULL)
		endwin();
	exit(EXIT_SUCCESS);
}

static void
init_term(void)
{
	int max_y, max_x;
	char *buf;

	win = initscr();
	keypad(stdscr, true);
	nodelay(win, true);
	noecho();
	ASSERT(win != NULL);
	start_color();

	init_pair(FMS_COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
	init_pair(FMS_COLOR_WHITE_INV, COLOR_BLACK, COLOR_WHITE);
	init_pair(FMS_COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
	init_pair(FMS_COLOR_AMBER, COLOR_YELLOW, COLOR_BLACK);
	init_pair(FMS_COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
	init_pair(FMS_COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);

	signal(SIGINT, handle_signal);
	signal(SIGQUIT, SIG_IGN);

	getmaxyx(win, max_y, max_x);
	buf = safe_calloc(max_x + 1, sizeof (*buf));
	for (int i = 0; i < max_x; i++)
		buf[i] = ' ';
	for (int y = 0; y < max_y; y++) {
		move(y, 0);
		printw(buf);
	}
	refresh();
	free(buf);
}

static void
proc_input(int c)
{
	if (c == KEY_BACKSPACE || c == KEY_DC) {
		fmsbox_push_key(fmsbox, FMS_KEY_CLR_DEL);
	} else if (isalnum(c) || c == '/' || c == ' ' || c == '.') {
		fmsbox_push_char(fmsbox, c);
	} else if (c == '+' || c == '-') {
		fmsbox_push_key(fmsbox, FMS_KEY_PLUS_MINUS);
	} else if (c == KEY_LEFT || c == KEY_RIGHT) {
		lsk_right = !lsk_right;
	} else if (c == KEY_UP) {
		if (lsk_nr > 0)
			lsk_nr--;
		else
			lsk_nr = 5;
	} else if (c == KEY_DOWN) {
		lsk_nr = (lsk_nr + 1) % 6;
	} else if (c == KEY_ENTER || c == '\n') {
		if (lsk_right)
			fmsbox_push_key(fmsbox, FMS_KEY_LSK_R1 + lsk_nr);
		else
			fmsbox_push_key(fmsbox, FMS_KEY_LSK_L1 + lsk_nr);
	}
}

static void
read_input(void)
{
	int c = getch();
	if (c != ERR)
		proc_input(c);
}

static void
draw_screen(void)
{
	fms_color_t color = FMS_COLOR_WHITE;

	attron(COLOR_PAIR(color));
	for (unsigned row_i = 0; row_i < FMSBOX_ROWS; row_i++) {
		const fmsbox_char_t *row = fmsbox_get_screen_row(fmsbox, row_i);

		move(row_i + OFF_Y, OFF_X);
		for (unsigned col_i = 0; col_i < FMSBOX_COLS; col_i++) {
			if (row[col_i].color != color) {
				attroff(COLOR_PAIR(color));
				color = row[col_i].color;
				attron(COLOR_PAIR(color));
			}
			if (row[col_i].size == FMS_FONT_SMALL)
				printw("%c", tolower(row[col_i].c));
			else
				printw("%c", toupper(row[col_i].c));
		}
	}
	attroff(COLOR_PAIR(color));

	move(0, OFF_X - 1);
	printw("+------------------------+");
	for (int i = 0; i < FMSBOX_ROWS; i++) {
		move(OFF_Y + i, OFF_X - 1);
		printw("|");
		move(OFF_Y + i, OFF_X + FMSBOX_COLS);
		printw("|");
	}
	move(FMSBOX_ROWS + OFF_Y, OFF_X - 1);
	printw("+------------------------+");
	for (int i = 0; i < 6; i++) {
		if (!lsk_right && lsk_nr == i)
			attron(COLOR_PAIR(FMS_COLOR_WHITE_INV));
		move(2 * i + 2 + OFF_Y, 0);
		printw("[L%d]", i + 1);
		if (!lsk_right && lsk_nr == i)
			attroff(COLOR_PAIR(FMS_COLOR_WHITE_INV));

		if (lsk_right && lsk_nr == i)
			attron(COLOR_PAIR(FMS_COLOR_WHITE_INV));
		move(2 * i + 2 + OFF_Y, OFF_X + FMSBOX_COLS + 1);
		printw("[R%d]", i + 1);
		if (lsk_right && lsk_nr == i)
			attroff(COLOR_PAIR(FMS_COLOR_WHITE_INV));
	}

	move(OFF_Y + FMSBOX_ROWS + 2, 0);

	refresh();
}

int
main(void)
{
	const char *hostname = "localhost";
	const char *ca_file = "cpdlc_cert.pem";
	int port = 0;
	struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };

	init_term();

	fmsbox = fmsbox_alloc(hostname, port, ca_file);
	ASSERT(fmsbox != NULL);

	draw_screen();
	for (;;) {
		int res = poll(&pfd, 1, POLL_INTVAL);

		if (res == 1)
			read_input();
		fmsbox_update(fmsbox);
		draw_screen();
	}

	fmsbox_free(fmsbox);
	echo();
	endwin();

	return (0);
}
