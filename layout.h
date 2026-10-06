#ifndef LAYOUT_H
#define LAYOUT_H

#define LAYOUT_MAX_BANDS 256

struct style {
	int dim;
	int highlight;
};

struct placed_text {
	int col;
	const char *text;
	int len;
};

struct placed_band {
	int row;
	int pad;
	struct style style;
	struct placed_text texts[2];
	int count;
};

struct layout {
	struct placed_band bands[LAYOUT_MAX_BANDS];
	int count;
	int caret_visible;
	int caret_row;
	int caret_col;
};

#endif