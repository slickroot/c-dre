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
#define BOX_BORDER_COLS 2
#define BOX_ROWS 3
#define CHILD_GAP 1

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

static struct node *new_box(const char *s)
{
	struct node *n = new_text(s);

	n->data.style.border = 1;
	return n;
}

static int box_cols(int len)
{
	return len + BOX_BORDER_COLS;
}

static int col_after(struct rect r)
{
	return r.col + r.cols + CHILD_GAP;
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

static void test_lone_text_keeps_its_column_when_a_box_is_added(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(0);

	node_append(band, new_text("Login"));
	node_append(band, new_box(""));
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert_rect(band->first_child->box, 2, centred(5), 1, 5);

	node_free(root);
}

static void test_box_goes_after_the_text_with_a_gap(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(0);

	node_append(band, new_text("Login"));
	node_append(band, new_box("ab"));
	node_append(root, band);

	layout(root, COLS, ROWS);

	struct rect text = band->first_child->box;
	assert_rect(band->last_child->box, text.row - 1, col_after(text),
		    BOX_ROWS, box_cols(2));

	node_free(root);
}

static void test_boxes_follow_each_other_with_a_gap(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(0);

	node_append(band, new_text("Login"));
	node_append(band, new_box(""));
	node_append(band, new_box("abc"));
	node_append(root, band);

	layout(root, COLS, ROWS);

	struct node *first = band->first_child->next;
	assert_rect(band->last_child->box, first->box.row, col_after(first->box),
		    BOX_ROWS, box_cols(3));

	node_free(root);
}

static void test_band_grows_to_the_box_height(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(0);

	node_append(band, new_text("Login"));
	node_append(band, new_box(""));
	node_append(root, band);
	node_append(root, new_band(0));

	layout(root, COLS, ROWS);

	assert_rect(band->box, 1, 1, BOX_ROWS, COLS);
	assert_rect(band->next->box, 1 + BOX_ROWS, 1, 1, COLS);

	node_free(root);
}

static void test_text_and_box_share_the_band_middle_row(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(0);

	node_append(band, new_text("Login"));
	node_append(band, new_box(""));
	node_append(root, band);

	layout(root, COLS, ROWS);

	int mid = band->box.row + (band->box.rows - 1) / 2;
	assert(band->first_child->box.row == mid);
	assert(band->last_child->box.row + 1 == mid);

	node_free(root);
}

static void test_tall_pad_keeps_its_height_with_a_box(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(2);

	node_append(band, new_text("Login"));
	node_append(band, new_box(""));
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert_rect(band->box, 1, 1, 5, COLS);
	assert(band->first_child->box.row == 3);
	assert(band->last_child->box.row == 2);

	node_free(root);
}

static void test_box_alone_in_a_band_starts_at_the_first_column(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(0);

	node_append(band, new_box("ab"));
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert_rect(band->first_child->box, 1, 1, BOX_ROWS, box_cols(2));

	node_free(root);
}

static struct node *new_column_band(int pad)
{
	struct node *band = new_band(pad);

	band->data.vertical = 1;
	return band;
}

static int stack_of(int text_count, int box_count)
{
	int children = text_count + box_count;

	return text_count + box_count * BOX_ROWS + (children - 1) * CHILD_GAP;
}

static void test_column_stacks_texts_and_boxes_in_child_order(void)
{
	struct node *root = new_tree();
	struct node *band = new_column_band(0);
	struct node *first = new_text("Login");
	struct node *second = new_box("ab");
	struct node *third = new_text("end");

	node_append(band, first);
	node_append(band, second);
	node_append(band, third);
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert_rect(first->box, 1, centred(5), 1, 5);
	assert_rect(second->box, first->box.row + 1 + CHILD_GAP,
		    centred(box_cols(2)), BOX_ROWS, box_cols(2));
	assert_rect(third->box, second->box.row + BOX_ROWS + CHILD_GAP,
		    centred(3), 1, 3);

	node_free(root);
}

static void test_column_height_holds_the_children_and_the_gaps(void)
{
	struct node *root = new_tree();
	struct node *band = new_column_band(0);

	node_append(band, new_text("a"));
	node_append(band, new_box("b"));
	node_append(band, new_box("c"));
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert(band->box.rows == stack_of(1, 2));

	node_free(root);
}

static void test_column_pad_is_the_minimum_height(void)
{
	struct node *root = new_tree();
	struct node *band = new_column_band(5);
	struct node *first = new_text("a");
	struct node *second = new_text("b");

	node_append(band, first);
	node_append(band, second);
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert(band->box.rows == 2 * 5 + 1);
	assert(first->box.row == 1 + (band->box.rows - stack_of(2, 0)) / 2);
	assert(second->box.row == first->box.row + 1 + CHILD_GAP);

	node_free(root);
}

static void test_column_lone_padded_child_stays_on_the_mid_row(void)
{
	struct node *root = new_tree();
	struct node *band = new_column_band(2);
	struct node *only = new_text("Login");

	node_append(band, only);
	node_append(root, band);

	layout(root, COLS, ROWS);

	assert_rect(band->box, 1, 1, 5, COLS);
	assert_rect(only->box, 3, centred(5), 1, 5);

	node_free(root);
}

static void test_column_moves_the_next_band_down(void)
{
	struct node *root = new_tree();
	struct node *band = new_column_band(0);
	struct node *below = new_band(0);

	node_append(band, new_text("a"));
	node_append(band, new_box("b"));
	node_append(root, band);
	node_append(root, below);

	layout(root, COLS, ROWS);

	assert(below->box.row == 1 + stack_of(1, 1));

	node_free(root);
}

static void test_switching_back_restores_the_row_geometry(void)
{
	struct node *root = new_tree();
	struct node *band = new_band(0);
	struct node *text = new_text("Login");
	struct node *box = new_box("ab");

	node_append(band, text);
	node_append(band, box);
	node_append(root, band);
	layout(root, COLS, ROWS);
	struct rect text_before = text->box;
	struct rect box_before = box->box;
	struct rect band_before = band->box;

	band->data.vertical = 1;
	layout(root, COLS, ROWS);
	band->data.vertical = 0;
	layout(root, COLS, ROWS);

	assert_rect(text->box, text_before.row, text_before.col,
		    text_before.rows, text_before.cols);
	assert_rect(box->box, box_before.row, box_before.col, box_before.rows,
		    box_before.cols);
	assert_rect(band->box, band_before.row, band_before.col,
		    band_before.rows, band_before.cols);

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
	test_lone_text_keeps_its_column_when_a_box_is_added();
	test_box_goes_after_the_text_with_a_gap();
	test_boxes_follow_each_other_with_a_gap();
	test_band_grows_to_the_box_height();
	test_text_and_box_share_the_band_middle_row();
	test_tall_pad_keeps_its_height_with_a_box();
	test_column_stacks_texts_and_boxes_in_child_order();
	test_column_height_holds_the_children_and_the_gaps();
	test_column_pad_is_the_minimum_height();
	test_column_lone_padded_child_stays_on_the_mid_row();
	test_column_moves_the_next_band_down();
	test_switching_back_restores_the_row_geometry();
	test_box_alone_in_a_band_starts_at_the_first_column();
	return 0;
}
