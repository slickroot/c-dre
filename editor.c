#include <stdlib.h>

#include "editor.h"
#include "text_buffer.h"

struct band {
	struct text_buffer buf;
	struct style style;
	int pad;
	struct band *next;
	struct band *prev;
};

struct editor {
	struct band *bands;
	struct band *selected;
	enum app_mode mode;
	int cols;
	int rows;
};

static void insert_node(struct editor *e)
{
	struct band *b = malloc(sizeof *b);
	if (!b)
		return;

	b->style = (struct style){0};
	b->pad = 0;
	text_buffer_init(&b->buf, e->cols - 1);
	b->next = e->bands;
	b->prev = NULL;
	if (e->bands)
		e->bands->prev = b;
	e->bands = b;
	e->selected = b;
}

static void delete_node(struct editor *e, struct band *node)
{
	if (!node)
		return;

	struct band *heir = node->next ? node->next : node->prev;

	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
	if (e->bands == node)
		e->bands = node->next;

	text_buffer_free(&node->buf);
	free(node);
	e->selected = heir;
}

struct editor *editor_new(int cols, int rows)
{
	struct editor *e = malloc(sizeof *e);
	if (!e)
		return NULL;

	e->bands = NULL;
	e->selected = NULL;
	e->mode = MODE_MOVE;
	e->cols = cols;
	e->rows = rows;
	return e;
}

void editor_free(struct editor *e)
{
	if (!e)
		return;

	while (e->bands) {
		struct band *next = e->bands->next;
		text_buffer_free(&e->bands->buf);
		free(e->bands);
		e->bands = next;
	}
	free(e);
}

enum app_mode editor_mode(const struct editor *e)
{
	return e->mode;
}

void editor_apply(struct editor *e, struct key_event ev)
{
	switch (ev.type) {
	case EVENT_ADD_BAND:
		insert_node(e);
		e->mode = MODE_TYPE;
		break;
	case EVENT_ESCAPE:
		e->mode = MODE_MOVE;
		break;
	case EVENT_ENTER_TYPE:
		e->mode = MODE_TYPE;
		break;
	case EVENT_SELECT_UP:
		if (e->selected && e->selected->next)
			e->selected = e->selected->next;
		break;
	case EVENT_SELECT_DOWN:
		if (e->selected && e->selected->prev)
			e->selected = e->selected->prev;
		break;
	case EVENT_CHAR:
		if (e->selected)
			text_buffer_insert(&e->selected->buf, ev.ch);
		break;
	case EVENT_BACKSPACE:
		if (e->selected)
			text_buffer_backspace(&e->selected->buf);
		break;
	case EVENT_DELETE_BAND:
		delete_node(e, e->selected);
		break;
	case EVENT_TOGGLE_DIM:
		if (e->selected)
			e->selected->style.dim = !e->selected->style.dim;
		break;
	case EVENT_GROW_BAND:
		if (e->selected)
			e->selected->pad++;
		break;
	case EVENT_SHRINK_BAND:
		if (e->selected && e->selected->pad > 0)
			e->selected->pad--;
		break;
	case EVENT_QUIT:
	case EVENT_NONE:
		break;
	}
}

struct layout layout(const struct editor *e)
{
	struct layout l;
	int visible = e->rows < LAYOUT_MAX_LABELS ? e->rows : LAYOUT_MAX_LABELS;

	l.count = 0;
	l.caret_visible = 0;
	l.caret_row = 0;
	l.caret_col = 0;

	if (e->cols <= 0)
		return l;

	struct band *oldest = e->bands;
	while (oldest && oldest->next)
		oldest = oldest->next;

	int top = 1;
	for (; oldest; oldest = oldest->prev) {
		int row = top + oldest->pad;
		if (row > visible)
			break;

		struct placed_label *p = &l.labels[l.count];
		p->row = row;
		p->col = (e->cols - oldest->buf.len) / 2 + 1;
		p->text = oldest->buf.data;
		p->len = oldest->buf.len;
		p->style = oldest->style;
		l.count++;

		if (oldest == e->selected) {
			l.caret_visible = 1;
			l.caret_row = row;
			l.caret_col = (e->cols - oldest->buf.len) / 2 +
				      oldest->buf.cursor + 1;
		}

		top += 2 * oldest->pad + 1;
	}

	return l;
}