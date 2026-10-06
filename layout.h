#ifndef LAYOUT_H
#define LAYOUT_H

#define LAYOUT_MAX_LABELS 256

struct placed_label {
	int row;
	int col;
	const char *text;
	int len;
};

struct layout {
	struct placed_label labels[LAYOUT_MAX_LABELS];
	int count;
	int caret_visible;
	int caret_row;
	int caret_col;
};

#endif