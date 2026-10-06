#include <assert.h>
#include <stddef.h>

#include "node.h"
#include "text_buffer.h"

#define MANY 10
#define TEXT_CAP 8

static void test_new_is_empty(void)
{
	struct node *n = node_new();

	assert(n != NULL);
	assert(n->parent == NULL);
	assert(n->children == NULL);
	assert(n->count == 0);
	assert(n->cap == 0);
	assert(n->data.text.data == NULL);
	assert(n->data.text.cap == 0);
	assert(n->data.style.dim == 0);
	assert(n->data.pad == 0);

	node_free(n);
}

static void test_append_sets_parent_and_order(void)
{
	struct node *root = node_new();
	struct node *a = node_new();
	struct node *b = node_new();
	struct node *c = node_new();

	assert(node_append(root, a) == a);
	assert(node_append(root, b) == b);
	assert(node_append(root, c) == c);

	assert(a->parent == root);
	assert(b->parent == root);
	assert(c->parent == root);
	assert(root->count == 3);
	assert(root->children[0] == a);
	assert(root->children[1] == b);
	assert(root->children[2] == c);

	node_free(root);
}

static void test_append_grows_past_initial_cap(void)
{
	struct node *root = node_new();
	struct node *kids[MANY];

	for (int i = 0; i < MANY; i++) {
		kids[i] = node_new();
		assert(kids[i] != NULL);
		assert(node_append(root, kids[i]) == kids[i]);
	}

	assert(root->count == MANY);
	assert(root->cap >= MANY);

	for (int i = 0; i < MANY; i++) {
		assert(root->children[i] == kids[i]);
		assert(kids[i]->parent == root);
		assert(node_index(kids[i]) == i);
	}

	node_free(root);
}

static void test_index_of_first_middle_and_last(void)
{
	struct node *root = node_new();
	struct node *a = node_new();
	struct node *b = node_new();
	struct node *c = node_new();

	node_append(root, a);
	node_append(root, b);
	node_append(root, c);

	assert(node_index(a) == 0);
	assert(node_index(b) == 1);
	assert(node_index(c) == 2);

	node_free(root);
}

static void test_index_of_root_is_minus_one(void)
{
	struct node *root = node_new();

	assert(node_index(root) == -1);

	node_free(root);
}

static void test_remove_first_child(void)
{
	struct node *root = node_new();
	struct node *a = node_new();
	struct node *b = node_new();
	struct node *c = node_new();

	node_append(root, a);
	node_append(root, b);
	node_append(root, c);

	node_remove(a);

	assert(root->count == 2);
	assert(root->children[0] == b);
	assert(root->children[1] == c);
	assert(node_index(b) == 0);
	assert(node_index(c) == 1);

	node_free(root);
}

static void test_remove_middle_child(void)
{
	struct node *root = node_new();
	struct node *a = node_new();
	struct node *b = node_new();
	struct node *c = node_new();

	node_append(root, a);
	node_append(root, b);
	node_append(root, c);

	node_remove(b);

	assert(root->count == 2);
	assert(root->children[0] == a);
	assert(root->children[1] == c);
	assert(node_index(a) == 0);
	assert(node_index(c) == 1);

	node_free(root);
}

static void test_remove_last_child(void)
{
	struct node *root = node_new();
	struct node *a = node_new();
	struct node *b = node_new();
	struct node *c = node_new();

	node_append(root, a);
	node_append(root, b);
	node_append(root, c);

	node_remove(c);

	assert(root->count == 2);
	assert(root->children[0] == a);
	assert(root->children[1] == b);
	assert(node_index(a) == 0);
	assert(node_index(b) == 1);

	node_free(root);
}

static void test_remove_frees_subtree(void)
{
	struct node *root = node_new();
	struct node *band = node_new();
	struct node *band_child = node_new();

	node_append(root, band);
	node_append(band, band_child);
	text_buffer_init(&band_child->data.text, TEXT_CAP);
	text_buffer_insert(&band_child->data.text, 'x');

	node_remove(band);

	assert(root->count == 0);
	assert(node_index(root) == -1);

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
	band->data.style.dim = 1;

	node_free(root);
}

int main(void)
{
	test_new_is_empty();
	test_append_sets_parent_and_order();
	test_append_grows_past_initial_cap();
	test_index_of_first_middle_and_last();
	test_index_of_root_is_minus_one();
	test_remove_first_child();
	test_remove_middle_child();
	test_remove_last_child();
	test_remove_frees_subtree();
	test_free_tree_with_mixed_text_buffers();
	return 0;
}