#ifndef NODE_H
#define NODE_H

#include "layout.h"
#include "text_buffer.h"

struct node_data {
	struct text_buffer text;
	struct style style;
	int pad;
};

struct node {
	struct node_data data;
	struct node *parent;
	struct node **children;
	int count;
	int cap;
};

struct node *node_new(void);
void node_free(struct node *n);
struct node *node_append(struct node *parent, struct node *child);
void node_remove(struct node *n);
int node_index(const struct node *n);

#endif