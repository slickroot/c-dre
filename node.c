#include <stdlib.h>

#include "node.h"

struct node *node_new(void)
{
	return calloc(1, sizeof(struct node));
}

void node_free(struct node *n)
{
	struct node *child = n->first_child;

	while (child) {
		struct node *next = child->next;
		node_free(child);
		child = next;
	}

	text_buffer_free(&n->data.text);
	free(n);
}

void node_append(struct node *parent, struct node *child)
{
	child->parent = parent;
	child->prev = parent->last_child;

	if (parent->last_child)
		parent->last_child->next = child;
	else
		parent->first_child = child;

	parent->last_child = child;
}

void node_insert_after(struct node *n, struct node *new_node)
{
	new_node->parent = n->parent;
	new_node->prev = n;
	new_node->next = n->next;

	if (n->next)
		n->next->prev = new_node;
	else
		n->parent->last_child = new_node;

	n->next = new_node;
}

void node_delete(struct node *n)
{
	struct node *parent = n->parent;

	if (parent) {
		if (n->prev)
			n->prev->next = n->next;
		else
			parent->first_child = n->next;

		if (n->next)
			n->next->prev = n->prev;
		else
			parent->last_child = n->prev;
	}

	node_free(n);
}
