#ifndef NODE_H
#define NODE_H

#include "rect.h"
#include "text_buffer.h"

struct style {
	int dim;
	int border;
};

struct node_data {
	struct text_buffer text;
	struct style style;
	int pad;
};

struct node {
	struct node_data data;
	struct rect box; /* computed by layout(), not data */
	struct node *parent;
	struct node *first_child, *last_child;
	struct node *prev, *next;
};

struct node *node_new(void);
void node_free(struct node *n);
void node_append(struct node *parent, struct node *child);
void node_delete(struct node *n);

#endif
