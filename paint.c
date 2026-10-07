#include "paint.h"

void paint_frame(const struct display_list *dl, struct grid *g)
{
	grid_clear(g, 0, 0);

	for (int i = 0; i < dl->count; i++) {
		const struct op *op = &dl->ops[i];
		struct rect r = op->rect;

		if (op->kind == OP_FILL)
			grid_fill(g, r.row, r.col, r.cols, r.rows, op->colour);
		else
			grid_text(g, r.row, r.col, op->text, r.cols,
				  op->colour);
	}

	if (dl->caret_visible)
		grid_cursor(g, dl->caret_row, dl->caret_col);
}
