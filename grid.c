#include <stdlib.h>

#include "grid.h"

static struct cell *cell_at(struct grid *g, int row, int col)
{
	if (row < 1 || row > g->rows || col < 1 || col > g->cols)
		return NULL;

	return &g->cells[(row - 1) * g->cols + (col - 1)];
}

struct grid *grid_new(int cols, int rows)
{
	struct grid *g = malloc(sizeof(*g));
	if (!g)
		return NULL;

	g->cells = malloc(sizeof(*g->cells) * (size_t)cols * (size_t)rows);
	if (!g->cells) {
		free(g);
		return NULL;
	}

	g->cols = cols;
	g->rows = rows;
	g->cursor_visible = 0;
	g->cursor_row = 0;
	g->cursor_col = 0;
	return g;
}

void grid_free(struct grid *g)
{
	if (!g)
		return;

	free(g->cells);
	free(g);
}

void grid_clear(struct grid *g, uint32_t fg, uint32_t bg)
{
	g->cursor_visible = 0;

	for (int i = 0; i < g->rows * g->cols; i++) {
		g->cells[i].ch = ' ';
		g->cells[i].fg = fg;
		g->cells[i].bg = bg;
	}
}

void grid_fill(struct grid *g, int row, int col, int w, int h, uint32_t bg)
{
	for (int r = row; r < row + h; r++) {
		for (int c = col; c < col + w; c++) {
			struct cell *cell = cell_at(g, r, c);
			if (cell)
				cell->bg = bg;
		}
	}
}

void grid_text(struct grid *g, int row, int col, const char *s, int len,
	       uint32_t fg)
{
	for (int i = 0; i < len; i++) {
		struct cell *cell = cell_at(g, row, col + i);
		if (!cell)
			continue;

		cell->ch = s[i];
		cell->fg = fg;
	}
}

void grid_cursor(struct grid *g, int row, int col)
{
	g->cursor_row = row;
	g->cursor_col = col;
	g->cursor_visible = 1;
}

const struct cell *grid_at(const struct grid *g, int row, int col)
{
	return cell_at((struct grid *)g, row, col);
}