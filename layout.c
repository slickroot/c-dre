#include "layout.h"

#include "node.h"

#define BOX_BORDER_COLS 2
#define BOX_ROWS 3
#define CHILD_GAP 1

static int is_box(const struct node *n)
{
	return n->data.style.border;
}

static struct rect box_rect(int mid, int col, int len)
{
	struct rect r = { mid - 1, col, BOX_ROWS, len + BOX_BORDER_COLS };

	return r;
}

static struct rect text_box(int row, int pad, int cols, int len, int index,
			    int count)
{
	struct rect r;

	if (count == 1) {
		r.row = row;
		r.col = (cols - len) / 2 + 1;
	} else if (pad >= 1) {
		r.row = index == 0 ? row - 1 : row + 1;
		r.col = (cols - len) / 2 + 1;
	} else {
		r.row = row;
		r.col = index == 0 ? 3 : cols - len - 1;
	}

	r.rows = 1;
	r.cols = len;
	return r;
}

static int layout_row(struct node *band, int top, int cols)
{
	int count = 0;
	int tallest = 0;
	for (struct node *c = band->first_child; c; c = c->next) {
		if (is_box(c))
			tallest = BOX_ROWS;
		else
			count++;
	}

	int height = 2 * band->data.pad + 1;
	if (tallest > height)
		height = tallest;

	int mid = top + (height - 1) / 2;
	int next_col = 1;
	int i = 0;
	for (struct node *c = band->first_child; c; c = c->next) {
		if (is_box(c))
			continue;
		c->box = text_box(mid, band->data.pad, cols, c->data.text.len,
				  i++, count);
		next_col = c->box.col + c->box.cols + CHILD_GAP;
	}

	for (struct node *c = band->first_child; c; c = c->next) {
		if (!is_box(c))
			continue;
		c->box = box_rect(mid, next_col, c->data.text.len);
		next_col = c->box.col + c->box.cols + CHILD_GAP;
	}

	return height;
}

static int child_rows(const struct node *c)
{
	return is_box(c) ? BOX_ROWS : 1;
}

static int stack_rows(const struct node *band)
{
	int rows = 0;

	for (struct node *c = band->first_child; c; c = c->next) {
		if (rows > 0)
			rows += CHILD_GAP;
		rows += child_rows(c);
	}
	return rows;
}

static struct rect centred_rect(int row, int rows, int cols, int len)
{
	struct rect r = { row, (cols - len) / 2 + 1, rows, len };

	return r;
}

static int layout_column(struct node *band, int top, int cols)
{
	int stack = stack_rows(band);
	int height = 2 * band->data.pad + 1;
	if (stack > height)
		height = stack;

	int row = top + (height - stack) / 2;
	for (struct node *c = band->first_child; c; c = c->next) {
		int rows = child_rows(c);
		int len = c->data.text.len + (is_box(c) ? BOX_BORDER_COLS : 0);

		c->box = centred_rect(row, rows, cols, len);
		row += rows + CHILD_GAP;
	}

	return height;
}

void layout(struct node *root, int cols, int rows)
{
	root->box.row = 1;
	root->box.col = 1;
	root->box.rows = rows;
	root->box.cols = cols;

	if (cols <= 0)
		return;

	int top = 1;
	for (struct node *band = root->first_child; band; band = band->next) {
		band->box.row = top;
		band->box.col = 1;
		band->box.cols = cols;
		band->box.rows = band->data.vertical
					 ? layout_column(band, top, cols)
					 : layout_row(band, top, cols);
		top += band->box.rows;
	}
}
