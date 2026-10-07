#include <stdlib.h>

#include "editor.h"
#include "node.h"
#include "text_buffer.h"

struct editor {
	struct node *root;
	struct node *selected;
	enum app_mode mode;
	int cols;
	int rows;
};

static struct node *new_text(struct editor *e)
{
	struct node *t = node_new();
	if (!t)
		return NULL;

	text_buffer_init(&t->data.text, e->cols - 1);
	return t;
}

static struct node *active_text(struct node *band)
{
	return band->last_child;
}

static void add_band(struct editor *e)
{
	struct node *band = node_new();
	if (!band)
		return;

	struct node *text = new_text(e);
	if (!text) {
		node_free(band);
		return;
	}

	node_append(band, text);
	node_append(e->root, band);

	e->selected = band;
	e->mode = MODE_TYPE;
}

static void add_text(struct editor *e)
{
	if (!e->selected || e->selected->first_child != e->selected->last_child)
		return;

	struct node *text = new_text(e);
	if (!text)
		return;

	node_append(e->selected, text);

	e->mode = MODE_TYPE;
}

struct editor *editor_new(int cols, int rows)
{
	struct editor *e = malloc(sizeof *e);
	if (!e)
		return NULL;

	e->root = node_new();
	if (!e->root) {
		free(e);
		return NULL;
	}

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

	node_free(e->root);
	free(e);
}

enum app_mode editor_mode(const struct editor *e)
{
	return e->mode;
}

struct node *editor_root(const struct editor *e)
{
	return e->root;
}

const struct node *editor_selected(const struct editor *e)
{
	return e->selected;
}

void editor_apply(struct editor *e, struct key_event ev)
{
	switch (ev.type) {
	case EVENT_ADD_BAND:
		add_band(e);
		break;
	case EVENT_ADD_TEXT:
		add_text(e);
		break;
	case EVENT_ESCAPE:
		e->mode = MODE_MOVE;
		break;
	case EVENT_ENTER_TYPE:
		e->mode = MODE_TYPE;
		break;
	case EVENT_SELECT_UP:
		if (e->selected && e->selected->prev)
			e->selected = e->selected->prev;
		break;
	case EVENT_SELECT_DOWN:
		if (e->selected && e->selected->next)
			e->selected = e->selected->next;
		break;
	case EVENT_CHAR:
		if (e->selected)
			text_buffer_insert(&active_text(e->selected)->data.text,
					   ev.ch);
		break;
	case EVENT_BACKSPACE:
		if (e->selected)
			text_buffer_backspace(
				&active_text(e->selected)->data.text);
		break;
	case EVENT_DELETE_BAND: {
		if (!e->selected)
			break;

		struct node *heir = e->selected->prev;

		if (!heir)
			heir = e->selected->next;

		node_delete(e->selected);
		e->selected = heir;
		break;
	}
	case EVENT_TOGGLE_DIM:
		if (e->selected) {
			struct node *band = e->selected;

			band->data.style.dim = !band->data.style.dim;
		}
		break;
	case EVENT_GROW_BAND:
		if (e->selected)
			e->selected->data.pad++;
		break;
	case EVENT_SHRINK_BAND:
		if (e->selected && e->selected->data.pad > 0)
			e->selected->data.pad--;
		break;
	case EVENT_QUIT:
	case EVENT_NONE:
		break;
	}
}
