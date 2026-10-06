#include "paint.h"

static const uint32_t canvas = 0x0A0A0Bu;
static const uint32_t ink = 0xC9C9CFu;
static const uint32_t dim = 0x6B6B73u;
static const uint32_t highlight = 0x1C1C20u;

void paint_frame(const struct layout *l, struct grid *g)
{
	grid_clear(g, ink, canvas);

	for (int i = 0; i < l->count; i++) {
		const struct placed_band *p = &l->bands[i];

		if (p->style.highlight)
			grid_fill(g, p->row - p->pad, 1, g->cols,
				  2 * p->pad + 1, highlight);

		for (int t = 0; t < p->count; t++)
			grid_text(g, p->texts[t].row, p->texts[t].col,
				  p->texts[t].text, p->texts[t].len,
				  p->style.dim ? dim : ink);
	}

	if (l->caret_visible)
		grid_cursor(g, l->caret_row, l->caret_col);
}