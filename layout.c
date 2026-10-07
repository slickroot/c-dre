#include "layout.h"

#include "node.h"

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
		band->box.rows = 2 * band->data.pad + 1;
		band->box.cols = cols;

		int count = 0;
		for (struct node *t = band->first_child; t; t = t->next)
			count++;

		int mid = top + band->data.pad;
		int i = 0;
		for (struct node *text = band->first_child; text;
		     text = text->next, i++)
			text->box = text_box(mid, band->data.pad, cols,
					     text->data.text.len, i, count);

		top += band->box.rows;
	}
}
