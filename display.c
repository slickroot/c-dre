#include <stddef.h>

#include "display.h"

#include "editor.h"
#include "node.h"

static const uint32_t canvas = 0x0A0A0Bu;
static const uint32_t ink = 0xC9C9CFu;
static const uint32_t dim = 0x6B6B73u;
static const uint32_t highlight = 0x1C1C20u;

static void emit(struct display_list *out, enum op_kind kind, struct rect r,
		 const char *text, uint32_t colour)
{
	if (out->count >= DISPLAY_MAX_OPS)
		return;

	struct op *op = &out->ops[out->count++];
	op->kind = kind;
	op->rect = r;
	op->text = text;
	op->colour = colour;
}

void display_list(const struct editor *e, struct display_list *out)
{
	out->count = 0;
	out->caret_visible = 0;
	out->caret_row = 0;
	out->caret_col = 0;

	const struct node *root = editor_root(e);
	const struct node *selected = editor_selected(e);

	emit(out, OP_FILL, root->box, NULL, canvas);
	if (root->box.cols <= 0)
		return;

	for (const struct node *band = root->first_child; band;
	     band = band->next) {
		if (band->box.row + band->data.pad > root->box.rows)
			break;

		if (band == selected && editor_mode(e) == MODE_MOVE)
			emit(out, OP_FILL, band->box, NULL, highlight);

		for (const struct node *text = band->first_child; text;
		     text = text->next)
			emit(out, OP_TEXT, text->box, text->data.text.data,
			     band->data.style.dim ? dim : ink);

		if (band == selected && editor_mode(e) == MODE_TYPE) {
			const struct node *t = band->last_child;
			out->caret_visible = 1;
			out->caret_row = t->box.row;
			out->caret_col = t->box.col + t->data.text.cursor;
		}
	}
}
