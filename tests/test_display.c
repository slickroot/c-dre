#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "display.h"
#include "editor.h"
#include "layout.h"
#include "node.h"
#include "rect.h"

#define COLS 40
#define ROWS 20
#define BIG_ROWS 512
#define BIG_BANDS 512

#define CANVAS 0x0A0A0Bu
#define INK 0xC9C9CFu
#define DIM 0x6B6B73u
#define HIGHLIGHT 0x1C1C20u

static void apply(struct editor *e, enum key_event_type type)
{
	editor_apply(e, (struct key_event){.type = type, .ch = 0});
}

static void type(struct editor *e, const char *s)
{
	for (; *s; s++)
		editor_apply(e,
			     (struct key_event){.type = EVENT_CHAR, .ch = *s});
}

static void next_band(struct editor *e)
{
	if (!editor_selected(e)) {
		apply(e, EVENT_ADD_BAND);
		return;
	}

	struct node *band = node_new();
	struct node *text = node_new();
	assert(band && text);
	text_buffer_init(&text->data.text, COLS - 1);
	node_append(band, text);
	node_append(editor_root(e), band);
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_ENTER_TYPE);
}

static struct editor *fresh(int cols, int rows)
{
	struct editor *e = editor_new(cols, rows);

	assert(e);
	return e;
}

static void render(struct editor *e, int cols, int rows,
		   struct display_list *dl)
{
	layout(editor_root(e), cols, rows);
	display_list(e, dl);
}

static int same_rect(struct rect a, struct rect b)
{
	return a.row == b.row && a.col == b.col && a.rows == b.rows &&
	       a.cols == b.cols;
}

static void assert_rect(struct rect got, struct rect want)
{
	assert(same_rect(got, want));
}

static int find_fill(const struct display_list *dl, struct rect r, uint32_t c)
{
	for (int i = 0; i < dl->count; i++)
		if (dl->ops[i].kind == OP_FILL && dl->ops[i].colour == c &&
		    same_rect(dl->ops[i].rect, r))
			return i;

	return -1;
}

static int find_text(const struct display_list *dl, struct rect r)
{
	for (int i = 0; i < dl->count; i++)
		if (dl->ops[i].kind == OP_TEXT && same_rect(dl->ops[i].rect, r))
			return i;

	return -1;
}

static int fills_with_colour(const struct display_list *dl, uint32_t c)
{
	int n = 0;

	for (int i = 0; i < dl->count; i++)
		if (dl->ops[i].kind == OP_FILL && dl->ops[i].colour == c)
			n++;

	return n;
}

static struct node *band_at(struct node *root, int n)
{
	struct node *band = root->first_child;

	while (n-- > 1)
		band = band->next;
	return band;
}

static struct editor *bands_with_text(int n)
{
	struct editor *e = fresh(COLS, ROWS);

	for (int i = 0; i < n; i++) {
		next_band(e);
		type(e, "ab");
	}
	return e;
}

static int count_borders(const struct display_list *dl)
{
	int n = 0;

	for (int i = 0; i < dl->count; i++)
		if (dl->ops[i].kind == OP_BORDER)
			n++;

	return n;
}

static void test_box_emits_a_border_and_its_text_inside_it(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_ADD_BAND);
	struct node *box = (struct node *)editor_selected(e);
	text_buffer_insert(&box->data.text, 'x');
	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	struct rect inner = {box->box.row + 1, box->box.col + 1, 1, 1};
	int border = -1;

	for (int i = 0; i < dl.count; i++)
		if (dl.ops[i].kind == OP_BORDER)
			border = i;

	assert(border >= 0);
	assert(count_borders(&dl) == 1);
	assert_rect(dl.ops[border].rect, box->box);
	assert(dl.ops[border].colour == INK);
	assert(find_text(&dl, inner) > border);

	editor_free(e);
}

static void test_text_children_emit_no_border(void)
{
	struct editor *e = bands_with_text(2);
	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	assert(count_borders(&dl) == 0);

	editor_free(e);
}

static void test_selected_box_highlight_precedes_its_border(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_BAND);
	const struct node *box = editor_selected(e);
	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	int fill = find_fill(&dl, box->box, HIGHLIGHT);

	assert(fill >= 0);
	for (int i = 0; i < fill; i++)
		assert(dl.ops[i].kind != OP_BORDER);

	editor_free(e);
}

static void test_dim_band_border_is_dim(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_STEP_OUT);
	apply(e, EVENT_TOGGLE_DIM);
	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	for (int i = 0; i < dl.count; i++)
		if (dl.ops[i].kind == OP_BORDER)
			assert(dl.ops[i].colour == DIM);

	assert(count_borders(&dl) == 1);

	editor_free(e);
}

static void test_root_fill_is_the_first_op(void)
{
	struct editor *e = bands_with_text(1);
	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	assert(dl.count > 0);
	assert(dl.ops[0].kind == OP_FILL);
	assert(dl.ops[0].colour == CANVAS);
	assert(dl.ops[0].text == NULL);
	assert_rect(dl.ops[0].rect, editor_root(e)->box);

	editor_free(e);
}

static void test_empty_tree_is_only_the_root_fill(void)
{
	struct editor *e = fresh(COLS, ROWS);
	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	assert(dl.count == 1);
	assert(dl.ops[0].kind == OP_FILL);
	assert(dl.ops[0].colour == CANVAS);
	assert(dl.caret_visible == 0);

	editor_free(e);
}

static void test_band_fill_precedes_its_own_texts(void)
{
	struct editor *e = bands_with_text(3);

	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SELECT_UP);
	assert(editor_mode(e) == MODE_MOVE);

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	struct node *first = band_at(editor_root(e), 1);
	struct node *second = band_at(editor_root(e), 2);
	struct node *third = band_at(editor_root(e), 3);

	int fill = find_fill(&dl, first->box, HIGHLIGHT);
	int one = find_text(&dl, first->first_child->box);
	int two = find_text(&dl, second->first_child->box);
	int three = find_text(&dl, third->first_child->box);

	assert(fill >= 0);
	assert(one >= 0);
	assert(two >= 0);
	assert(three >= 0);
	assert(fill < one);
	assert(one < two);
	assert(two < three);

	editor_free(e);
}

static void test_highlight_is_only_on_the_selected_band(void)
{
	struct editor *e = bands_with_text(3);

	apply(e, EVENT_ESCAPE);

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	struct node *first = band_at(editor_root(e), 1);
	struct node *second = band_at(editor_root(e), 2);
	struct node *third = band_at(editor_root(e), 3);

	assert(fills_with_colour(&dl, HIGHLIGHT) == 1);
	assert(find_fill(&dl, first->box, HIGHLIGHT) == -1);
	assert(find_fill(&dl, second->box, HIGHLIGHT) == -1);
	assert(find_fill(&dl, third->box, HIGHLIGHT) >= 0);
	assert(find_text(&dl, first->first_child->box) >= 0);
	assert(find_text(&dl, second->first_child->box) >= 0);
	assert(find_text(&dl, third->first_child->box) >= 0);

	editor_free(e);
}

static void test_unselected_bands_have_no_fill_op_at_all(void)
{
	struct editor *e = bands_with_text(2);

	apply(e, EVENT_ESCAPE);

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	struct node *first = band_at(editor_root(e), 1);
	struct node *second = band_at(editor_root(e), 2);

	assert(find_fill(&dl, first->box, HIGHLIGHT) == -1);
	assert(find_fill(&dl, first->box, CANVAS) == -1);
	assert(find_fill(&dl, second->box, HIGHLIGHT) >= 0);

	editor_free(e);
}

static void test_type_mode_emits_no_highlight(void)
{
	struct editor *e = bands_with_text(2);

	assert(editor_mode(e) == MODE_TYPE);

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	assert(fills_with_colour(&dl, HIGHLIGHT) == 0);
	assert(find_text(&dl, editor_root(e)->first_child->first_child->box) >=
	       0);

	editor_free(e);
}

static void test_dim_band_texts_are_dim(void)
{
	struct editor *e = bands_with_text(3);

	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_TOGGLE_DIM);

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	struct node *first = band_at(editor_root(e), 1);
	struct node *second = band_at(editor_root(e), 2);
	struct node *third = band_at(editor_root(e), 3);

	int one = find_text(&dl, first->first_child->box);
	int two = find_text(&dl, second->first_child->box);
	int three = find_text(&dl, third->first_child->box);

	assert(one >= 0);
	assert(two >= 0);
	assert(three >= 0);
	assert(dl.ops[two].colour == DIM);
	assert(dl.ops[one].colour == INK);
	assert(dl.ops[three].colour == INK);

	editor_free(e);
}

static void test_toggle_dim_twice_returns_to_ink(void)
{
	struct editor *e = bands_with_text(1);

	apply(e, EVENT_TOGGLE_DIM);

	struct display_list dim;

	render(e, COLS, ROWS, &dim);

	int i = find_text(&dim, editor_root(e)->first_child->first_child->box);

	assert(i >= 0);
	assert(dim.ops[i].colour == DIM);

	apply(e, EVENT_TOGGLE_DIM);

	struct display_list ink;

	render(e, COLS, ROWS, &ink);

	i = find_text(&ink, editor_root(e)->first_child->first_child->box);

	assert(i >= 0);
	assert(ink.ops[i].colour == INK);

	editor_free(e);
}

static void test_text_op_carries_the_buffer(void)
{
	struct editor *e = fresh(COLS, ROWS);

	apply(e, EVENT_ADD_BAND);
	type(e, "Login");

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	int i = find_text(&dl, editor_root(e)->first_child->first_child->box);

	assert(i >= 0);
	assert(dl.ops[i].text != NULL);
	assert(strcmp(dl.ops[i].text, "Login") == 0);

	editor_free(e);
}

static struct editor *two_text_band(void)
{
	struct editor *e = fresh(COLS, ROWS);

	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	return e;
}

static void test_selected_text_highlight_replaces_the_band_highlight(void)
{
	struct editor *e = two_text_band();

	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct node *band = editor_root(e)->first_child;
	struct node *first = band->first_child;
	struct node *second = band->last_child;

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	assert(fills_with_colour(&dl, HIGHLIGHT) == 1);
	assert(find_fill(&dl, band->box, HIGHLIGHT) == -1);
	assert(find_fill(&dl, first->box, HIGHLIGHT) >= 0);
	assert(find_fill(&dl, second->box, HIGHLIGHT) == -1);

	editor_free(e);
}

static void test_selected_text_highlight_precedes_its_own_text(void)
{
	struct editor *e = two_text_band();

	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct node *first = editor_root(e)->first_child->first_child;

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	int fill = find_fill(&dl, first->box, HIGHLIGHT);
	int text = find_text(&dl, first->box);

	assert(fill >= 0);
	assert(text >= 0);
	assert(fill < text);

	editor_free(e);
}

static void test_selected_band_fills_no_text(void)
{
	struct editor *e = two_text_band();

	apply(e, EVENT_ESCAPE);

	struct node *band = editor_root(e)->first_child;

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	assert(find_fill(&dl, band->box, HIGHLIGHT) >= 0);
	assert(find_fill(&dl, band->first_child->box, HIGHLIGHT) == -1);
	assert(find_fill(&dl, band->last_child->box, HIGHLIGHT) == -1);

	editor_free(e);
}

static void test_caret_is_on_the_last_text_in_type_mode(void)
{
	struct editor *e = two_text_band();

	assert(editor_mode(e) == MODE_TYPE);

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	const struct node *last = editor_root(e)->first_child->last_child;

	assert(dl.caret_visible == 1);
	assert(dl.caret_row == last->box.row);
	assert(dl.caret_col == last->box.col + last->data.text.cursor);

	editor_free(e);
}

static void test_no_caret_in_move_mode(void)
{
	struct editor *e = two_text_band();

	apply(e, EVENT_ESCAPE);
	assert(editor_mode(e) == MODE_MOVE);

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	assert(dl.caret_visible == 0);
	assert(dl.caret_row == 0);
	assert(dl.caret_col == 0);
	assert(find_text(&dl, editor_root(e)->first_child->last_child->box) >=
	       0);

	editor_free(e);
}

static void test_caret_moves_with_the_selection(void)
{
	struct editor *e = two_text_band();

	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ESCAPE);
	next_band(e);
	type(e, "c");
	apply(e, EVENT_SELECT_UP);

	struct display_list dl;

	render(e, COLS, ROWS, &dl);

	const struct node *last = editor_selected(e)->last_child;

	assert(dl.caret_visible == 1);
	assert(dl.caret_row == last->box.row);
	assert(dl.caret_col == last->box.col + last->data.text.cursor);

	editor_free(e);
}

static void test_clips_at_the_first_band_past_the_screen(void)
{
	struct editor *e = bands_with_text(5);
	struct display_list dl;

	render(e, COLS, 3, &dl);

	for (int i = 1; i <= 3; i++) {
		struct node *band = band_at(editor_root(e), i);

		assert(find_text(&dl, band->first_child->box) >= 0);
	}
	for (int i = 4; i <= 5; i++) {
		struct node *band = band_at(editor_root(e), i);

		assert(find_text(&dl, band->first_child->box) == -1);
		assert(find_fill(&dl, band->box, HIGHLIGHT) == -1);
	}

	assert(dl.count == 4);
	assert(dl.caret_visible == 0);

	editor_free(e);
}

static void test_clips_a_padded_band_whose_text_row_is_past_the_screen(void)
{
	struct editor *e = fresh(COLS, ROWS);

	apply(e, EVENT_ADD_BAND);
	type(e, "a");
	apply(e, EVENT_ESCAPE);
	next_band(e);
	for (int i = 0; i < 4; i++)
		apply(e, EVENT_GROW_BAND);

	struct display_list dl;

	render(e, COLS, 4, &dl);

	struct node *first = band_at(editor_root(e), 1);
	struct node *second = band_at(editor_root(e), 2);

	assert(second->box.row == 2);
	assert(second->first_child->box.row == 6);
	assert(find_text(&dl, first->first_child->box) >= 0);
	assert(find_text(&dl, second->first_child->box) == -1);
	assert(dl.count == 2);

	editor_free(e);
}

static void test_clips_the_band_below_a_tall_stacked_band(void)
{
	struct editor *e = fresh(COLS, ROWS);

	apply(e, EVENT_ADD_BAND);
	type(e, "a");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_STEP_OUT);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_STEP_OUT);
	apply(e, EVENT_SWITCH_DIRECTION);
	next_band(e);

	struct display_list dl;

	render(e, COLS, 4, &dl);

	struct node *tall = band_at(editor_root(e), 1);
	struct node *below = band_at(editor_root(e), 2);

	assert(tall->data.vertical);
	assert(tall->box.row + tall->box.rows > 4);
	assert(below->box.row > 4);
	assert(find_text(&dl, tall->first_child->box) >= 0);
	assert(count_borders(&dl) == 2);
	assert(find_text(&dl, below->first_child->box) == -1);
	assert(find_fill(&dl, below->box, HIGHLIGHT) == -1);

	editor_free(e);
}

static void test_op_count_matches_the_tree_when_it_fits(void)
{
	struct editor *e = fresh(COLS, BIG_ROWS);

	for (int i = 0; i < 8; i++) {
		next_band(e);
		apply(e, EVENT_ADD_TEXT);
	}
	apply(e, EVENT_ESCAPE);

	struct display_list dl;

	render(e, COLS, BIG_ROWS, &dl);

	/* root fill, two texts per band, one highlight on the selection */
	assert(dl.count == 1 + 2 * 8 + 1);

	editor_free(e);
}

static void test_count_never_exceeds_max_ops(void)
{
	struct editor *e = fresh(COLS, BIG_ROWS);

	for (int i = 0; i < BIG_BANDS; i++) {
		next_band(e);
		apply(e, EVENT_ADD_TEXT);
	}
	apply(e, EVENT_ESCAPE);

	struct display_list dl;

	render(e, COLS, BIG_ROWS, &dl);

	/* 1 + 2 * BIG_BANDS + 1 ops are on screen, well past the cap */
	assert(1 + 2 * BIG_BANDS + 1 > DISPLAY_MAX_OPS);
	assert(dl.count == DISPLAY_MAX_OPS);

	editor_free(e);
}

static void test_a_smaller_tree_after_a_capped_one_starts_from_scratch(void)
{
	struct editor *big = fresh(COLS, BIG_ROWS);

	for (int i = 0; i < BIG_BANDS; i++) {
		next_band(big);
		apply(big, EVENT_ADD_TEXT);
	}
	apply(big, EVENT_ESCAPE);

	struct display_list dl;

	render(big, COLS, BIG_ROWS, &dl);
	assert(dl.count == DISPLAY_MAX_OPS);

	struct editor *small = bands_with_text(2);

	render(small, COLS, ROWS, &dl);
	assert(dl.count == 3);

	editor_free(big);
	editor_free(small);
}

int main(void)
{
	test_box_emits_a_border_and_its_text_inside_it();
	test_text_children_emit_no_border();
	test_selected_box_highlight_precedes_its_border();
	test_dim_band_border_is_dim();
	test_root_fill_is_the_first_op();
	test_empty_tree_is_only_the_root_fill();
	test_band_fill_precedes_its_own_texts();
	test_highlight_is_only_on_the_selected_band();
	test_unselected_bands_have_no_fill_op_at_all();
	test_type_mode_emits_no_highlight();
	test_dim_band_texts_are_dim();
	test_toggle_dim_twice_returns_to_ink();
	test_text_op_carries_the_buffer();
	test_caret_is_on_the_last_text_in_type_mode();
	test_no_caret_in_move_mode();
	test_caret_moves_with_the_selection();
	test_selected_text_highlight_replaces_the_band_highlight();
	test_selected_text_highlight_precedes_its_own_text();
	test_selected_band_fills_no_text();
	test_clips_at_the_first_band_past_the_screen();
	test_clips_a_padded_band_whose_text_row_is_past_the_screen();
	test_clips_the_band_below_a_tall_stacked_band();
	test_op_count_matches_the_tree_when_it_fits();
	test_count_never_exceeds_max_ops();
	test_a_smaller_tree_after_a_capped_one_starts_from_scratch();
	return 0;
}
