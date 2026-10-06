#include <stdio.h>
#include <string.h>
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

static const char *fg_for(struct style s)
{
	return s.dim ? "\x1b[38;2;107;107;115m" /* #6B6B73 */
		     : "\x1b[38;2;201;201;207m";
}

static const char *bg_for(struct style s)
{
	return s.highlight ? "\x1b[48;2;28;28;32m"  /* #1C1C20 */
			   : "\x1b[48;2;10;10;11m"; /* #0A0A0B */
}

void paint_frame(const struct layout *l)
{
	write(STDOUT_FILENO, "\x1b[?2026h", sizeof "\x1b[?2026h" - 1);
	write(STDOUT_FILENO, "\x1b[2J", sizeof "\x1b[2J" - 1);

	for (int i = 0; i < l->count; i++) {
		const struct placed_band *p = &l->bands[i];

		const char *bg = bg_for(p->style);
		write(STDOUT_FILENO, bg, strlen(bg));

		if (p->style.highlight) {
			for (int line = p->row - p->pad;
			     line <= p->row + p->pad; line++) {
				char edge[32];
				int e = snprintf(edge, sizeof edge,
						 "\x1b[%d;1H", line);

				write(STDOUT_FILENO, edge, e);
				write(STDOUT_FILENO, "\x1b[K",
				      sizeof "\x1b[K" - 1);
			}
		}

		const char *fg = fg_for(p->style);
		write(STDOUT_FILENO, fg, strlen(fg));

		for (int t = 0; t < p->count; t++) {
			char cup[32];
			int n = snprintf(cup, sizeof cup, "\x1b[%d;%dH", p->row,
					 p->texts[t].col);

			write(STDOUT_FILENO, cup, n);
			write(STDOUT_FILENO, p->texts[t].text, p->texts[t].len);
		}
	}

	/* canvas must stay the active background so the next ESC[2J clears to
	 * canvas instead of to the highlight colour */
	write(STDOUT_FILENO, "\x1b[48;2;10;10;11m",
	      sizeof "\x1b[48;2;10;10;11m" - 1);

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
