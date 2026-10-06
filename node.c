#include <stdlib.h>
#include <string.h>

#include "node.h"

#define CHILD_CAP 4

struct node *node_new(void)
{
	return calloc(1, sizeof(struct node));
}

void node_free(struct node *n)
{
	for (int i = 0; i < n->count; i++)
		node_free(n->children[i]);

	text_buffer_free(&n->data.text);
	free(n->children);
	free(n);
}

struct node *node_append(struct node *parent, struct node *child)
{
	if (parent->count == parent->cap) {
		int cap = parent->cap ? parent->cap * 2 : CHILD_CAP;
		size_t bytes = sizeof(*parent->children) * (size_t)cap;
		struct node **children = realloc(parent->children, bytes);

		if (!children)
			return NULL;

		parent->children = children;
		parent->cap = cap;
	}

	parent->children[parent->count++] = child;
	child->parent = parent;
	return child;
}

void node_remove(struct node *n)
{
	struct node *parent = n->parent;

	if (parent) {
		int i = node_index(n);

		memmove(&parent->children[i], &parent->children[i + 1],
			(parent->count - i - 1) * sizeof(*parent->children));
		parent->count--;
	}

	node_free(n);
}

int node_index(const struct node *n)
{
	if (!n->parent)
		return -1;

	for (int i = 0; i < n->parent->count; i++) {
		if (n->parent->children[i] == n)
			return i;
	}

	return -1;
}