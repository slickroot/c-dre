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

		int count = 0;
		int tallest = 0;
		for (struct node *c = band->first_child; c; c = c->next) {
			if (is_box(c))
				tallest = BOX_ROWS;
			else
				count++;
		}

		band->box.rows = 2 * band->data.pad + 1;
		if (tallest > band->box.rows)
			band->box.rows = tallest;

		int mid = top + (band->box.rows - 1) / 2;
		int next_col = 1;
		int i = 0;
		for (struct node *c = band->first_child; c; c = c->next) {
			if (is_box(c))
				continue;
			c->box = text_box(mid, band->data.pad, cols,
					  c->data.text.len, i++, count);
			next_col = c->box.col + c->box.cols + CHILD_GAP;
		}

		for (struct node *c = band->first_child; c; c = c->next) {
			if (!is_box(c))
				continue;
			c->box = box_rect(mid, next_col, c->data.text.len);
			next_col = c->box.col + c->box.cols + CHILD_GAP;
		}

		top += band->box.rows;
	}
}
