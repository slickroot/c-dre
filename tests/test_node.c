#include <assert.h>
#include <stddef.h>

#include "node.h"
#include "text_buffer.h"

#define MANY 5
#define TEXT_CAP 8

static void test_new_is_empty(void)
{
	struct node *n = node_new();

	assert(n != NULL);
	assert(n->parent == NULL);
	assert(n->first_child == NULL);
	assert(n->last_child == NULL);
	assert(n->prev == NULL);
	assert(n->next == NULL);
	assert(n->data.text.data == NULL);
	assert(n->data.text.cap == 0);
	assert(n->data.style.dim == 0);
	assert(n->data.style.border == 0);
	assert(n->data.pad == 0);

	node_free(n);
}

static void test_append_to_empty_parent(void)
{
	struct node *root = node_new();
	struct node *a = node_new();

	node_append(root, a);

	assert(root->first_child == a);
	assert(root->last_child == a);
	assert(a->parent == root);
	assert(a->prev == NULL);
	assert(a->next == NULL);

	node_free(root);
}

static void test_append_several_keeps_order(void)
{
	struct node *root = node_new();
	struct node *kids[MANY];

	for (int i = 0; i < MANY; i++) {
		kids[i] = node_new();
		node_append(root, kids[i]);
	}

	assert(root->first_child == kids[0]);
	assert(root->last_child == kids[MANY - 1]);
	assert(kids[0]->prev == NULL);
	assert(kids[MANY - 1]->next == NULL);

	for (int i = 0; i < MANY; i++) {
		assert(kids[i]->parent == root);
		if (i > 0)
			assert(kids[i]->prev == kids[i - 1]);
		if (i + 1 < MANY)
			assert(kids[i]->next == kids[i + 1]);
	}

	struct node *n = root->first_child;
	for (int i = 0; i < MANY; i++) {
		assert(n == kids[i]);
		n = n->next;
	}
	assert(n == NULL);

	n = root->last_child;
	for (int i = MANY; i > 0; i--) {
		assert(n == kids[i - 1]);
		n = n->prev;
	}
	assert(n == NULL);

	node_free(root);
}

static void test_delete_first_child(void)
{
	struct node *root = node_new();
	struct node *a = node_new();
	struct node *b = node_new();
	struct node *c = node_new();

	node_append(root, a);
	node_append(root, b);
	node_append(root, c);

	node_delete(a);

	assert(root->first_child == b);
	assert(root->last_child == c);
	assert(b->prev == NULL);
	assert(b->next == c);
	assert(c->prev == b);

	node_free(root);
}

static void test_delete_middle_child(void)
{
	struct node *root = node_new();
	struct node *a = node_new();
	struct node *b = node_new();
	struct node *c = node_new();

	node_append(root, a);
	node_append(root, b);
	node_append(root, c);

	node_delete(b);

	assert(root->first_child == a);
	assert(root->last_child == c);
	assert(a->next == c);
	assert(c->prev == a);

	node_free(root);
}

static void test_delete_last_child(void)
{
	struct node *root = node_new();
	struct node *a = node_new();
	struct node *b = node_new();
	struct node *c = node_new();

	node_append(root, a);
	node_append(root, b);
	node_append(root, c);

	node_delete(c);

	assert(root->first_child == a);
	assert(root->last_child == b);
	assert(a->next == b);
	assert(b->next == NULL);

	node_free(root);
}

static void test_delete_only_child(void)
{
	struct node *root = node_new();
	struct node *a = node_new();

	node_append(root, a);

	node_delete(a);

	assert(root->first_child == NULL);
	assert(root->last_child == NULL);

	node_free(root);
}

static void test_delete_root_frees_it(void)
{
	struct node *root = node_new();
	struct node *a = node_new();

	node_append(root, a);

	node_delete(root);
}

static void test_delete_frees_subtree(void)
{
	struct node *root = node_new();
	struct node *band = node_new();
	struct node *t1 = node_new();
	struct node *t2 = node_new();

	node_append(root, band);
	node_append(band, t1);
	node_append(band, t2);
	text_buffer_init(&t2->data.text, TEXT_CAP);
	text_buffer_insert(&t2->data.text, 'x');

	node_delete(band);

	assert(root->first_child == NULL);
	assert(root->last_child == NULL);

	node_free(root);
}

static void test_free_tree_with_mixed_text_buffers(void)
{
	struct node *root = node_new();
	struct node *band = node_new();
	struct node *t1 = node_new();
	struct node *t2 = node_new();

	text_buffer_init(&t1->data.text, TEXT_CAP);
	text_buffer_insert(&t1->data.text, 'h');
	text_buffer_insert(&t1->data.text, 'i');

	node_append(root, band);
	node_append(band, t1);
	node_append(band, t2);
	band->data.pad = 2;

	node_free(root);
}

int main(void)
{
	test_new_is_empty();
	test_append_to_empty_parent();
	test_append_several_keeps_order();
	test_delete_first_child();
	test_delete_middle_child();
	test_delete_last_child();
	test_delete_only_child();
	test_delete_root_frees_it();
	test_delete_frees_subtree();
	test_free_tree_with_mixed_text_buffers();
	return 0;
}
