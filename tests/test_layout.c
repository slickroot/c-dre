#include <assert.h>
#include <stddef.h>

#include "layout.h"
#include "node.h"
#include "rect.h"
#include "text_buffer.h"

#define COLS 40
#define ROWS 20
#define TEXT_CAP 32

#define LEFT_COL 3

static void assert_rect(struct rect got, int row, int col, int rows, int cols)
{
	assert(got.row == row);
	assert(got.col == col);
	assert(got.rows == rows);
	assert(got.cols == cols);
}

static struct node *new_text(const char *s)
{
	struct node *n = node_new();

	assert(n);
	text_buffer_init(&n->data.text, TEXT_CAP);
	for (; *s; s++)
		assert(text_buffer_insert(&n->data.text, *s));
	return n;
}

static struct node *new_band(int pad)
{
	struct node *band = node_new();

	assert(band);
	band->data.pad = pad;
	return band;
}

static struct node *new_tree(void)
{
	struct node *root = node_new();

	assert(root);
	return root;
}

static int centred(int len)
{
	return (COLS - len) / 2 + 1;
}

static void test_root_box_is_the_whole_screen(void)
{
	struct node *root = new_tree();

	layout(root, COLS, ROWS);

	assert_rect(root->box, 1, 1, ROWS, COLS);

	node_free(root);
}

static void test_bands_stack_from_the_top_by_one_row(void)
{
	struct node *root = new_tree();

	for (int i = 0; i < 3; i++)
		node_append(root, new_band(0));

	layout(root, COLS, ROWS);

	struct node *band = root->first_child;
	for (int i = 0; i < 3; i++) {
		assert_rect(band->box, i + 1, 1, 1, COLS);
		band = band->next;
	}

	node_free(root);
}

static void test_next_band_starts_after_the_previous_band_rows(void)
{
	struct node *root = new_tree();

	node_append(root, new_band(2));
	node_append(root, new_band(0));
	node_append(root, new_band(1));

	layout(root, COLS, ROWS);

	struct node *band = root->first_child;
	assert_rect(band->box, 1, 1, 5, COLS);
	band = band->next;
	assert_rect(band->box, 6, 1, 1, COLS);
	band = band->next;
	assert_rect(band->box, 7, 1, 3, COLS);

	node_free(root);
}

static void test_one_text_is_centred_on_the_band_mid(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(1);

	node_append(band, new_text("Login"));
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert_rect(band->box, 1, 1, 3, COLS);
	assert_rect(band->first_child->box, 2, centred(5), 1, 5);

	node_free(root);
}

static void test_two_texts_with_no_pad_stay_side_by_side(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(0);

	node_append(band, new_text("Login"));
	node_append(band, new_text("Logout"));
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert_rect(band->first_child->box, 1, LEFT_COL, 1, 5);
	assert_rect(band->last_child->box, 1, COLS - 6 - 1, 1, 6);

	node_free(root);
}

static void test_two_texts_with_pad_stack_around_the_mid(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(2);

	node_append(band, new_text("Login"));
	node_append(band, new_text("Logout"));
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert_rect(band->box, 1, 1, 5, COLS);
	assert_rect(band->first_child->box, 2, centred(5), 1, 5);
	assert_rect(band->last_child->box, 4, centred(6), 1, 6);

	node_free(root);
}

static void test_text_rect_is_one_row_wide_as_the_text(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(0);

	node_append(band, new_text("ab"));
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert_rect(band->first_child->box, 1, centred(2), 1, 2);

	node_free(root);
}

static void test_bands_below_the_screen_still_get_boxes(void)
{
	struct node *root = new_tree();

	for (int i = 0; i < 4; i++)
		node_append(root, new_band(0));

	layout(root, COLS, 2);

	for (int i = 0; i < 4; i++) {
		struct node *band = root->first_child;

		for (int j = 0; j < i; j++)
			band = band->next;

		assert_rect(band->box, i + 1, 1, 1, COLS);
	}

	node_free(root);
}

static void test_not_positive_cols_boxes_only_the_root(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(2);

	node_append(band, new_text("Login"));
	node_append(root, band);

	layout(root, 0, ROWS);

	assert_rect(root->box, 1, 1, ROWS, 0);

	/* the early return leaves every band and text box zeroed */
	assert_rect(band->box, 0, 0, 0, 0);
	assert_rect(band->first_child->box, 0, 0, 0, 0);

	node_free(root);
}

static void test_negative_cols_boxes_only_the_root(void)
{
	struct node *root = new_tree();

	node_append(root, new_band(0));

	layout(root, -1, ROWS);

	assert_rect(root->box, 1, 1, ROWS, -1);
	assert_rect(root->first_child->box, 0, 0, 0, 0);

	node_free(root);
}

int main(void)
{
	test_root_box_is_the_whole_screen();
	test_bands_stack_from_the_top_by_one_row();
	test_next_band_starts_after_the_previous_band_rows();
	test_one_text_is_centred_on_the_band_mid();
	test_two_texts_with_no_pad_stay_side_by_side();
	test_two_texts_with_pad_stack_around_the_mid();
	test_text_rect_is_one_row_wide_as_the_text();
	test_bands_below_the_screen_still_get_boxes();
	test_not_positive_cols_boxes_only_the_root();
	test_negative_cols_boxes_only_the_root();
	return 0;
}
