#include <stdio.h>
#include <unistd.h>

#include "paint.h"

void paint_wallpaper(void)
{
	write(STDOUT_FILENO, "\x1b[?1049h", sizeof "\x1b[?1049h" - 1);
	write(STDOUT_FILENO, "\x1b[?25l", sizeof "\x1b[?25l" - 1);
	write(STDOUT_FILENO, "\x1b[48;2;10;10;11m",
	      sizeof "\x1b[48;2;10;10;11m" - 1);
	write(STDOUT_FILENO, "\x1b[2J", sizeof "\x1b[2J" - 1);
	write(STDOUT_FILENO, "\x1b[1;1H", sizeof "\x1b[1;1H" - 1);
}

void paint_frame(const struct layout *l)
{
	write(STDOUT_FILENO, "\x1b[?2026h", sizeof "\x1b[?2026h" - 1);
	write(STDOUT_FILENO, "\x1b[38;2;201;201;207m",
	      sizeof "\x1b[38;2;201;201;207m" - 1);
	write(STDOUT_FILENO, "\x1b[2J", sizeof "\x1b[2J" - 1);

	for (int i = 0; i < l->count; i++) {
		const struct placed_label *p = &l->labels[i];

		char cup[32];
		int n = snprintf(cup, sizeof cup, "\x1b[%d;%dH", p->row,
				 p->col);

		write(STDOUT_FILENO, cup, n);
		write(STDOUT_FILENO, p->text, p->len);
	}

	if (l->caret_visible) {
		char caret[32];
		int c = snprintf(caret, sizeof caret, "\x1b[%d;%dH",
				 l->caret_row, l->caret_col);

		write(STDOUT_FILENO, caret, c);
		write(STDOUT_FILENO, "\x1b[?25h", sizeof "\x1b[?25h" - 1);
	} else {
		write(STDOUT_FILENO, "\x1b[?25l", sizeof "\x1b[?25l" - 1);
	}

	write(STDOUT_FILENO, "\x1b[?2026l", sizeof "\x1b[?2026l" - 1);
}
