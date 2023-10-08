/*
 * Copyright 2023 Saso Kiselkov
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
#include <locale.h>
#include <ncursesw/ncurses.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../src/cpdlc_alloc.h"
#include "../src/cpdlc_assert.h"
#include "../src/cpdlc_client.h"
#include "../src/cpdlc_msglist.h"
#include "../fans/fans.h"

#define	POLL_INTVAL	1000	/* ms */
#define	DEL_CHAR	0x7f
#define	OFF_X		5
#define	OFF_Y		1

static WINDOW *win = NULL;
static fans_t *fans = NULL;
static bool lsk_right = false;
static int lsk_nr = 0;

static void
handle_signal(int signum)
{
	CPDLC_UNUSED(signum);
	if (fans != NULL)
		fans_free(fans);
	if (win != NULL)
		endwin();
	exit(EXIT_SUCCESS);
}

static void
init_term(void)
{
	int max_y, max_x;
	char *buf;

	setlocale(LC_ALL, "");
	win = initscr();
	raw();
	keypad(stdscr, true);
	nodelay(win, true);
	noecho();
	CPDLC_ASSERT(win != NULL);
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
		printw("%s", buf);
	}
	refresh();
	free(buf);
}

static bool
is_fms_entry_key(int c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
	    (c >= '0' && c <= '9') || c == '/' || c == ' ' || c == '.');
}

static bool
proc_input(int c)
{
	if (c == 'Q' - 64 || c == 'C' - 64) {
		return (false);
	}
	if (c == KEY_BACKSPACE || c == KEY_DC) {
		fans_push_key(fans, FMS_KEY_CLR_DEL);
	} else if (is_fms_entry_key(c)) {
		fans_push_char(fans, c);
	} else if (c == '+' || c == '-') {
		fans_push_key(fans, FMS_KEY_PLUS_MINUS);
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
			fans_push_key(fans, FMS_KEY_LSK_R1 + lsk_nr);
		else
			fans_push_key(fans, FMS_KEY_LSK_L1 + lsk_nr);
	} else if (c == KEY_NPAGE) {
		fans_push_key(fans, FMS_KEY_NEXT);
	} else if (c == KEY_PPAGE) {
		fans_push_key(fans, FMS_KEY_PREV);
	}
	return (true);
}

static bool
read_input(void)
{
	int c = getch();
	if (c != ERR) {
		return (proc_input(c));
	} else {
		return (true);
	}
}

static const char *
symbol2str(char c)
{
	switch (c) {
	case '^':
		return ("\u2191");
	case 'v':
		return ("\u2193");
	case '`':
		return ("\u00B0");
	case '_':
		return ("\u2610");
	}
	return (NULL);
}

static const char *
letter2sc(char c)
{
	switch (c) {
	case 'A':
		return ("\u1d00");
	case 'B':
		return ("\u0299");
	case 'C':
		return ("\u1d04");
	case 'D':
		return ("\u1d05");
	case 'E':
		return ("\u1d07");
	case 'F':
		return ("\ua730");
	case 'G':
		return ("\u0262");
	case 'H':
		return ("\u029c");
	case 'I':
		return ("\u026a");
	case 'J':
		return ("\u1d0a");
	case 'K':
		return ("\u1d0b");
	case 'L':
		return ("\u029f");
	case 'M':
		return ("\u1d0d");
	case 'N':
		return ("\u0274");
	case 'O':
		return ("\u1d0f");
	case 'P':
		return ("\u1d18");
	case 'R':
		return ("\u0280");
	case 'S':
		return ("\ua731");
	case 'T':
		return ("\u1d1b");
	case 'U':
		return ("\u1d1c");
	case 'V':
		return ("\u1d20");
	case 'W':
		return ("\u1d21");
	case 'Y':
		return ("\u028f");
	case 'Z':
		return ("\u1d22");
	}
	return (NULL);
}

static void
draw_screen(void)
{
	fms_color_t color = FMS_COLOR_WHITE;

	attron(COLOR_PAIR(color));
	for (unsigned row_i = 0; row_i < FMS_ROWS; row_i++) {
		const fms_char_t *row = fans_get_screen_row(fans, row_i);

		move(row_i + OFF_Y, OFF_X);
		for (unsigned col_i = 0; col_i < FMS_COLS; col_i++) {
			const char *str;

			if (row[col_i].color != color) {
				attroff(COLOR_PAIR(color));
				color = row[col_i].color;
				attron(COLOR_PAIR(color));
			}
			if ((str = symbol2str(row[col_i].c)) != NULL) {
				addstr(str);
			} else if (row[col_i].size == FMS_FONT_SMALL) {
				if ((str = letter2sc(row[col_i].c)) != NULL)
					addstr(str);
				else
					printw("%c", tolower(row[col_i].c));
			} else {
				printw("%c", toupper(row[col_i].c));
			}
		}
	}
	attroff(COLOR_PAIR(color));

	move(0, OFF_X - 1);
	printw("+------------------------+");
	for (int i = 0; i < FMS_ROWS; i++) {
		move(OFF_Y + i, OFF_X - 1);
		printw("|");
		move(OFF_Y + i, OFF_X + FMS_COLS);
		printw("|");
	}
	move(FMS_ROWS + OFF_Y, OFF_X - 1);
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
		move(2 * i + 2 + OFF_Y, OFF_X + FMS_COLS + 1);
		printw("[R%d]", i + 1);
		if (lsk_right && lsk_nr == i)
			attroff(COLOR_PAIR(FMS_COLOR_WHITE_INV));
	}

	move(OFF_Y + FMS_ROWS + 2, 6);
	printw("   \u2195\u2194    Select LSK");
	move(OFF_Y + FMS_ROWS + 3, 6);
	printw("[Return] Push LSK");
	move(OFF_Y + FMS_ROWS + 4, 6);
	printw(" [PgUp]  PREV");
	move(OFF_Y + FMS_ROWS + 5, 6);
	printw(" [PgDn]  NEXT");
	move(OFF_Y + FMS_ROWS + 7, 6);
	printw("[Ctrl+Q] Quit");

	refresh();
}

int
main(void)
{
	struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
	struct stat st;

	init_term();

	fans = fans_alloc(NULL, NULL);
	CPDLC_ASSERT(fans != NULL);
	if (stat("ca_cert.pem", &st) == 0) {
		cpdlc_client_t *cl = fans_get_client(fans);
		cpdlc_client_set_ca_file(cl, "ca_cert.pem");
	}
	draw_screen();
	for (;;) {
		int res = poll(&pfd, 1, POLL_INTVAL);

		if (res == 1) {
			if (!read_input())
				break;
		}
		fans_update(fans);
		draw_screen();
	}

	fans_free(fans);
	echo();
	endwin();

	return (0);
}
