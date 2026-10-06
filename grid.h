#ifndef GRID_H
#define GRID_H

#include <stdint.h>

struct cell {
	char ch;     /* ' ' when empty */
	uint32_t fg; /* 0xRRGGBB */
	uint32_t bg; /* 0xRRGGBB */
};

struct grid {
	int cols, rows;
	struct cell *cells; /* rows * cols, row-major */
	int cursor_visible;
	int cursor_row, cursor_col;
};

struct grid *grid_new(int cols, int rows);
void grid_free(struct grid *g);
void grid_clear(struct grid *g, uint32_t fg, uint32_t bg);
void grid_fill(struct grid *g, int row, int col, int w, int h, uint32_t bg);
void grid_text(struct grid *g, int row, int col, const char *s, int len,
	       uint32_t fg);
void grid_cursor(struct grid *g, int row, int col);
const struct cell *grid_at(const struct grid *g, int row, int col);

#endif