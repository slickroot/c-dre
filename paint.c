#include "paint.h"

static void paint_border(struct grid *g, struct rect r, uint32_t colour)
{
	int last_row = r.row + r.rows - 1;
	int last_col = r.col + r.cols - 1;

	for (int col = r.col; col <= last_col; col++) {
		int corner = col == r.col || col == last_col;
		char ch = corner ? BORDER_CORNER : BORDER_HORIZONTAL;

		grid_text(g, r.row, col, &ch, 1, colour);
		grid_text(g, last_row, col, &ch, 1, colour);
	}

	char side = BORDER_VERTICAL;

	for (int row = r.row + 1; row < last_row; row++) {
		grid_text(g, row, r.col, &side, 1, colour);
		grid_text(g, row, last_col, &side, 1, colour);
	}
}

void paint_frame(const struct display_list *dl, struct grid *g)
{
	grid_clear(g, 0, 0);

	for (int i = 0; i < dl->count; i++) {
		const struct op *op = &dl->ops[i];
		struct rect r = op->rect;

		if (op->kind == OP_FILL)
			grid_fill(g, r.row, r.col, r.cols, r.rows, op->colour);
		else if (op->kind == OP_BORDER)
			paint_border(g, r, op->colour);
		else
			grid_text(g, r.row, r.col, op->text, r.cols,
				  op->colour);
	}

	if (dl->caret_visible)
		grid_cursor(g, dl->caret_row, dl->caret_col);
}
