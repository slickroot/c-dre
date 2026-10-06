#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "editor.h"
#include "input.h"
#include "layout.h"
#include "paint.h"

static struct termios saved_tty;

static void enter(int *cols, int *rows)
{
	tcgetattr(STDIN_FILENO, &saved_tty);

	struct termios raw = saved_tty;
	cfmakeraw(&raw);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

	paint_wallpaper();

	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	*cols = ws.ws_col;
	*rows = ws.ws_row;
}

static void restore(void)
{
	write(STDOUT_FILENO, "\x1b[0m", sizeof "\x1b[0m" - 1);
	write(STDOUT_FILENO, "\x1b[?1049l", sizeof "\x1b[?1049l" - 1);
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

	struct editor *e = editor_new(cols, rows);
	if (!e)
		exit(1);

	struct input_parser parser;
	input_parser_init(&parser);

	char byte;
	while (read(STDIN_FILENO, &byte, 1) == 1) {
		struct key_event ev =
			input_parse(&parser, byte, editor_mode(e));
		if (ev.type == EVENT_QUIT)
			break;
		editor_apply(e, ev);
		struct layout l = layout(e);
		paint_frame(&l);
	}

	editor_free(e);

	return 0;
}
