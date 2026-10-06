#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "editor.h"
#include "grid.h"
#include "input.h"
#include "layout.h"
#include "paint.h"
#include "term.h"

static struct termios saved_tty;
static struct display_list dl;

static void enter(int *cols, int *rows)
{
	tcgetattr(STDIN_FILENO, &saved_tty);

	struct termios raw = saved_tty;
	cfmakeraw(&raw);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

	term_enter();

	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	*cols = ws.ws_col;
	*rows = ws.ws_row;
}

static void restore(void)
{
	term_leave();
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_tty);
	fflush(stdout);
}

int main(void)
{
	if (!isatty(STDIN_FILENO)) {
		fprintf(stderr, "dre: stdin is not a terminal\n");
		exit(1);
	}

	atexit(restore);

	int cols, rows;
	enter(&cols, &rows);

	struct grid *g = grid_new(cols, rows);
	if (!g)
		exit(1);

	struct editor *e = editor_new(cols, rows);
	if (!e)
		exit(1);

	char byte;
	while (read(STDIN_FILENO, &byte, 1) == 1) {
		struct key_event ev = input_parse(byte, editor_mode(e));
		if (ev.type == EVENT_QUIT)
			break;
		editor_apply(e, ev);
		layout(editor_root(e), cols, rows);
		display_list(e, &dl);
		paint_frame(&dl, g);
		term_flush(g);
	}

	grid_free(g);
	editor_free(e);

	return 0;
}
