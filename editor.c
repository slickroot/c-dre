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

static int is_band(const struct editor *e, const struct node *n)
{
	return n && n->parent == e->root;
}

static int is_child_of_band(const struct editor *e, const struct node *n)
{
	return n && is_band(e, n->parent);
}

static int is_box(const struct editor *e, const struct node *n)
{
	return is_child_of_band(e, n) && n->data.style.border;
}

static int is_text(const struct editor *e, const struct node *n)
{
	return is_child_of_band(e, n) && !is_box(e, n);
}

static int same_row(const struct node *a, const struct node *b)
{
	return a->box.row == b->box.row;
}

/* Sibling shown beside the text, in the same row; NULL if none. */
static struct node *beside(const struct node *text, struct node *sibling)
{
	return sibling && same_row(text, sibling) ? sibling : NULL;
}

/* Sibling shown above or below the text, in another row; NULL if none. */
static struct node *stacked_with(const struct node *text, struct node *sibling)
{
	return sibling && !same_row(text, sibling) ? sibling : NULL;
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

static struct node *new_box(struct editor *e)
{
	struct node *box = new_text(e);
	if (!box)
		return NULL;

	box->data.style.border = 1;
	return box;
}

static void add_box(struct editor *e)
{
	struct node *box = new_box(e);
	if (!box)
		return;

	node_append(e->selected, box);

	e->selected = box;
}

static void add_box_after(struct editor *e)
{
	struct node *box = new_box(e);
	if (!box)
		return;

	node_insert_after(e->selected, box);

	e->selected = box;
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

static void toggle_dim(struct node *text)
{
	text->data.style.dim = !text->data.style.dim;
}

void editor_apply(struct editor *e, struct key_event ev)
{
	switch (ev.type) {
	case EVENT_ADD_BAND: /* the `a` key */
		if (!e->selected)
			add_band(e);
		else if (is_band(e, e->selected))
			add_box(e);
		break;
	case EVENT_ADD_TEXT:
		if (is_band(e, e->selected))
			add_text(e);
		else if (is_box(e, e->selected))
			add_box_after(e);
		break;
	case EVENT_ESCAPE:
		e->mode = MODE_MOVE;
		break;
	case EVENT_ENTER_TYPE:
		if (is_band(e, e->selected))
			e->mode = MODE_TYPE;
		break;
	case EVENT_SELECT_UP:
		if (is_band(e, e->selected) && e->selected->prev)
			e->selected = e->selected->prev;
		else if (is_text(e, e->selected) &&
			 stacked_with(e->selected, e->selected->prev))
			e->selected = e->selected->prev;
		break;
	case EVENT_SELECT_DOWN:
		if (is_band(e, e->selected) && e->selected->next)
			e->selected = e->selected->next;
		else if (is_text(e, e->selected) &&
			 stacked_with(e->selected, e->selected->next))
			e->selected = e->selected->next;
		break;
	case EVENT_CHAR:
		if (is_band(e, e->selected))
			text_buffer_insert(&active_text(e->selected)->data.text,
					   ev.ch);
		break;
	case EVENT_BACKSPACE:
		if (is_band(e, e->selected))
			text_buffer_backspace(
				&active_text(e->selected)->data.text);
		break;
	case EVENT_DELETE_BAND: {
		if (!is_band(e, e->selected))
			break;

		struct node *heir = e->selected->prev;

		if (!heir)
			heir = e->selected->next;

		node_delete(e->selected);
		e->selected = heir;
		break;
	}
	case EVENT_TOGGLE_DIM:
		if (is_text(e, e->selected))
			toggle_dim(e->selected);
		else if (is_band(e, e->selected))
			for (struct node *text = e->selected->first_child; text;
			     text = text->next)
				toggle_dim(text);
		break;
	case EVENT_GROW_BAND:
		if (is_band(e, e->selected))
			e->selected->data.pad++;
		break;
	case EVENT_SHRINK_BAND:
		if (is_band(e, e->selected) && e->selected->data.pad > 0)
			e->selected->data.pad--;
		break;
	case EVENT_SWITCH_DIRECTION:
		if (is_band(e, e->selected))
			e->selected->data.vertical =
				!e->selected->data.vertical;
		break;
	case EVENT_STEP_IN:
		if (e->mode == MODE_MOVE && is_band(e, e->selected) &&
		    e->selected->first_child)
			e->selected = e->selected->first_child;
		break;
	case EVENT_STEP_OUT:
		if (e->selected && !is_band(e, e->selected))
			e->selected = e->selected->parent;
		break;
	case EVENT_SELECT_PREV_TEXT:
		if (e->mode == MODE_MOVE && is_text(e, e->selected) &&
		    beside(e->selected, e->selected->prev))
			e->selected = e->selected->prev;
		break;
	case EVENT_SELECT_NEXT_TEXT:
		if (e->mode == MODE_MOVE && is_text(e, e->selected) &&
		    beside(e->selected, e->selected->next))
			e->selected = e->selected->next;
		break;
	case EVENT_QUIT:
	case EVENT_NONE:
		break;
	}
}
