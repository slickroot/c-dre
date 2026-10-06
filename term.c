#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "term.h"

#define BUF_SIZE (64 * 1024)

static char buf[BUF_SIZE];
static size_t buf_len;

static void buf_flush(void)
{
	if (buf_len) {
		write(STDOUT_FILENO, buf, buf_len);
		buf_len = 0;
	}
}

static void emit(const char *s, size_t n)
{
	if (buf_len + n > sizeof buf)
		buf_flush();

	memcpy(buf + buf_len, s, n);
	buf_len += n;
}

static void emit_str(const char *s)
{
	emit(s, strlen(s));
}

static void emit_escape(const char *fmt, int a, int b)
{
	char esc[32];
	int n = snprintf(esc, sizeof esc, fmt, a, b);

	emit(esc, (size_t)n);
}

static void emit_color(int fg, uint32_t c)
{
	char esc[32];
	int n = snprintf(esc, sizeof esc,
			 fg ? "\x1b[38;2;%u;%u;%um" : "\x1b[48;2;%u;%u;%um",
			 (unsigned)((c >> 16) & 0xff),
			 (unsigned)((c >> 8) & 0xff), (unsigned)(c & 0xff));

	emit(esc, (size_t)n);
}

void term_enter(void)
{
	emit_str("\x1b[?1049h");
	emit_str("\x1b[?25l");
	emit_str("\x1b[48;2;10;10;11m");
	emit_str("\x1b[2J");
	emit_str("\x1b[1;1H");
	buf_flush();
}

void term_flush(const struct grid *g)
{
	uint32_t fg = 0, bg = 0;
	int have = 0;

	emit_str("\x1b[?2026h");

	for (int row = 1; row <= g->rows; row++) {
		emit_escape("\x1b[%d;1H", row, 0);

		for (int col = 1; col <= g->cols; col++) {
			const struct cell *cell = grid_at(g, row, col);

			if (!have || cell->fg != fg) {
				emit_color(1, cell->fg);
				fg = cell->fg;
			}
			if (!have || cell->bg != bg) {
				emit_color(0, cell->bg);
				bg = cell->bg;
			}
			have = 1;

			emit(&cell->ch, 1);
		}
	}

	if (g->cursor_visible) {
		emit_escape("\x1b[%d;%dH", g->cursor_row, g->cursor_col);
		emit_str("\x1b[?25h");
	} else {
		emit_str("\x1b[?25l");
	}

	emit_str("\x1b[?2026l");
	buf_flush();
}

void term_leave(void)
{
	emit_str("\x1b[0m");
	emit_str("\x1b[?1049l");
	buf_flush();
}
