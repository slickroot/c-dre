#ifndef LAYOUT_H
#define LAYOUT_H

#define LAYOUT_MAX_LABELS 256

struct style {
	int dim;
	int highlight;
};

struct placed_label {
	int row;
	int col;
	const char *text;
	int len;
	int pad;
	struct style style;
};

struct layout {
	struct placed_label labels[LAYOUT_MAX_LABELS];
	int count;
	int caret_visible;
	int caret_row;
	int caret_col;
};

#endif