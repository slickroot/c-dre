#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#include "rect.h"

#define DISPLAY_MAX_OPS 1024

enum op_kind { OP_FILL, OP_TEXT, OP_BORDER };

struct op {
	enum op_kind kind;
	struct rect rect;
	const char *text; /* TEXT only */
	uint32_t colour;  /* FILL: background; TEXT, BORDER: foreground */
};

struct display_list {
	struct op ops[DISPLAY_MAX_OPS];
	int count;
	int caret_visible;
	int caret_row, caret_col;
};

struct editor;

void display_list(const struct editor *e, struct display_list *out);

#endif
