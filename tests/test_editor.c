#include <assert.h>
#include <string.h>

#include "editor.h"
#include "grid.h"
#include "layout.h"
#include "paint.h"

#define COLS 40
#define ROWS 40

#define CANVAS 0x0A0A0Bu
#define INK 0xC9C9CFu
#define DIM 0x6B6B73u
#define HIGHLIGHT 0x1C1C20u

static int e_cols;
static int e_rows;

static void apply(struct editor *e, enum key_event_type type)
{
	editor_apply(e, (struct key_event){.type = type, .ch = 0});
}

static void apply_ch(struct editor *e, char ch)
{
	editor_apply(e, (struct key_event){.type = EVENT_CHAR, .ch = ch});
}

static void apply_and_layout(struct editor *e, enum key_event_type type)
{
	apply(e, type);
	layout(editor_root(e), e_cols, e_rows);
}

static void type(struct editor *e, const char *s)
{
	for (; *s; s++)
		apply_ch(e, *s);
}

static void add_bands(struct editor *e, int n)
{
	for (int i = 0; i < n; i++)
		apply(e, EVENT_ADD_BAND);
}

static struct editor *fresh(int cols, int rows)
{
	struct editor *e = editor_new(cols, rows);
	assert(e);
	e_cols = cols;
	e_rows = rows;
	return e;
}

static struct grid *frame(struct editor *e)
{
	static struct display_list dl;

	layout(editor_root(e), e_cols, e_rows);
	display_list(e, &dl);
	struct grid *g = grid_new(e_cols, e_rows);
	assert(g);
	paint_frame(&dl, g);
	return g;
}

static const struct cell *cell(struct grid *g, int row, int col)
{
	const struct cell *c = grid_at(g, row, col);
	assert(c);
	return c;
}

static void assert_blank(struct grid *g, int row, int col)
{
	assert(cell(g, row, col)->ch == ' ');
}

static void assert_text(struct grid *g, int row, int col, const char *s,
			uint32_t fg)
{
	for (int i = 0; s[i]; i++) {
		const struct cell *c = cell(g, row, col + i);
		assert(c->ch == s[i]);
		assert(c->fg == fg);
	}
}

static void assert_bg(struct grid *g, int row, int col, uint32_t bg)
{
	assert(cell(g, row, col)->bg == bg);
}

static void assert_row_blank(struct grid *g, int row)
{
	for (int col = 1; col <= g->cols; col++)
		assert_blank(g, row, col);
}

static void assert_bg_rows(struct grid *g, int first, int last, uint32_t bg)
{
	for (int row = first < 1 ? 1 : first; row <= last && row <= g->rows;
	     row++)
		for (int col = 1; col <= g->cols; col++)
			assert_bg(g, row, col, bg);
}

static void assert_all_blank(struct grid *g)
{
	for (int row = 1; row <= g->rows; row++)
		assert_row_blank(g, row);
}

static void assert_caret(struct grid *g, int row, int col)
{
	assert(g->cursor_visible == 1);
	assert(g->cursor_row == row);
	assert(g->cursor_col == col);
}

static void assert_no_caret(struct grid *g)
{
	assert(g->cursor_visible == 0);
}

static void assert_same_frame(const struct grid *a, const struct grid *b)
{
	assert(a->cursor_visible == b->cursor_visible);
	assert(a->cursor_row == b->cursor_row);
	assert(a->cursor_col == b->cursor_col);
	for (int row = 1; row <= a->rows; row++)
		for (int col = 1; col <= a->cols; col++) {
			const struct cell *x = grid_at(a, row, col);
			const struct cell *y = grid_at(b, row, col);
			assert(x->ch == y->ch);
			assert(x->bg == y->bg);
			if (x->ch != ' ')
				assert(x->fg == y->fg);
		}
}

static void test_new_editor_is_empty(void)
{
	struct editor *e = fresh(COLS, ROWS);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(g);
	editor_free(e);
}

static void test_add_band(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 1, (COLS - (int)strlen("")) / 2 + 1);
	assert(editor_mode(e) == MODE_TYPE);

	grid_free(g);
	editor_free(e);
}

static void test_new_band_text_is_normal(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "ab");

	struct grid *g = frame(e);

	assert_text(g, 1, (COLS - 2) / 2 + 1, "ab", INK);

	grid_free(g);
	editor_free(e);
}

static void test_type_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "hi");

	struct grid *g = frame(e);

	int col = (COLS - 2) / 2 + 1;
	assert_text(g, 1, col, "hi", INK);
	assert_blank(g, 1, col + 2);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 1, col + 2);

	grid_free(g);
	editor_free(e);
}

static void test_backspace(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "hi");
	apply(e, EVENT_BACKSPACE);

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 1, col, "h", INK);
	assert_blank(g, 1, col + 1);

	grid_free(g);
	editor_free(e);
}

static void test_backspace_on_empty_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_BACKSPACE);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_caret(g, 1, (COLS - (int)strlen("")) / 2 + 1);

	grid_free(g);
	editor_free(e);
}

static void test_three_bands_stack_downwards(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	type(e, "a");
	apply(e, EVENT_SELECT_UP);
	type(e, "b");
	apply(e, EVENT_SELECT_UP);
	type(e, "c");
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_SELECT_DOWN);

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 1, col, "c", INK);
	assert_text(g, 2, col, "b", INK);
	assert_text(g, 3, col, "a", INK);
	assert_caret(g, 3, col + 1);

	grid_free(g);
	editor_free(e);
}

static void test_escape_and_enter_type_change_mode(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	assert(editor_mode(e) == MODE_TYPE);

	struct grid *typing = frame(e);
	assert_caret(typing, 1, (COLS - (int)strlen("")) / 2 + 1);

	apply(e, EVENT_ESCAPE);
	assert(editor_mode(e) == MODE_MOVE);

	struct grid *moving = frame(e);
	assert_no_caret(moving);

	apply(e, EVENT_ENTER_TYPE);
	assert(editor_mode(e) == MODE_TYPE);

	struct grid *retyping = frame(e);
	assert_caret(retyping, 1, (COLS - (int)strlen("")) / 2 + 1);

	grid_free(typing);
	grid_free(moving);
	grid_free(retyping);
	editor_free(e);
}

static void test_select_up_moves_caret(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_UP);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_caret(g, 2, (COLS - (int)strlen("")) / 2 + 1);

	grid_free(g);
	editor_free(e);
}

static void test_select_up_at_top_stays(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SELECT_UP);

	struct grid *g = frame(e);

	assert_caret(g, 1, (COLS - (int)strlen("")) / 2 + 1);

	grid_free(g);
	editor_free(e);
}

static void test_select_down_at_bottom_stays(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_SELECT_DOWN);

	struct grid *g = frame(e);

	assert_caret(g, 3, (COLS - (int)strlen("")) / 2 + 1);

	grid_free(g);
	editor_free(e);
}

static void test_select_down_moves_caret(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SELECT_DOWN);

	struct grid *g = frame(e);

	assert_caret(g, 3, (COLS - (int)strlen("")) / 2 + 1);

	grid_free(g);
	editor_free(e);
}

static void test_typing_after_select_up_edits_older_band(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "new");
	apply(e, EVENT_ADD_BAND);
	type(e, "old");
	apply(e, EVENT_SELECT_UP);
	type(e, "X");

	struct grid *g = frame(e);

	int top = (COLS - (int)strlen("newX")) / 2 + 1;
	int bottom = (COLS - (int)strlen("old")) / 2 + 1;
	assert_text(g, 1, top, "newX", INK);
	assert_blank(g, 1, top + 4);
	assert_text(g, 2, bottom, "old", INK);
	assert_blank(g, 2, bottom + 3);
	assert_caret(g, 1, top + 4);

	grid_free(g);
	editor_free(e);
}

static void test_delete_middle_of_three(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	type(e, "a");
	apply(e, EVENT_SELECT_UP);
	type(e, "b");
	apply(e, EVENT_DELETE_BAND);

	struct grid *g = frame(e);

	assert_row_blank(g, 1);
	assert_text(g, 2, (COLS - 1) / 2 + 1, "a", INK);
	assert_row_blank(g, 3);
	assert_caret(g, 1, (COLS - (int)strlen("")) / 2 + 1);

	grid_free(g);
	editor_free(e);
}

static void test_delete_top_band_selects_below(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	type(e, "a");
	apply(e, EVENT_SELECT_UP);
	type(e, "b");
	apply(e, EVENT_SELECT_UP);
	type(e, "c");
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_DELETE_BAND);

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 1, col, "b", INK);
	assert_text(g, 2, col, "a", INK);
	assert_row_blank(g, 3);
	assert_caret(g, 1, col + 1);

	grid_free(g);
	editor_free(e);
}

static void test_delete_only_band(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_DELETE_BAND);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_delete_with_no_bands_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_DELETE_BAND);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_char_and_backspace_with_no_bands(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply_ch(e, 'x');
	apply(e, EVENT_BACKSPACE);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_quit_and_none_are_noops(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_QUIT);
	apply(e, EVENT_NONE);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(g);
	editor_free(e);
}

static void test_clips_to_screen_rows(void)
{
	struct editor *e = fresh(COLS, 2);
	add_bands(e, 3);
	type(e, "a");
	apply(e, EVENT_SELECT_UP);
	type(e, "b");
	apply(e, EVENT_SELECT_UP);
	type(e, "c");
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_SELECT_DOWN);

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 1, col, "c", INK);
	assert_text(g, 2, col, "b", INK);
	assert_no_caret(g);

	grid_free(g);

	apply(e, EVENT_SELECT_UP);

	struct grid *moved = frame(e);

	assert_text(moved, 2, col, "b", INK);
	assert_caret(moved, 2, col + 1);

	grid_free(moved);
	editor_free(e);
}

static void test_capacity_is_cols_minus_one(void)
{
	struct editor *e = fresh(10, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "abcdefghij");

	struct grid *g = frame(e);

	int col = (10 - (int)strlen("abcdefghi")) / 2 + 1;
	assert_text(g, 1, col, "abcdefghi", INK);
	assert_blank(g, 1, 10);
	assert_caret(g, 1, 10);

	grid_free(g);
	editor_free(e);
}

static void test_toggle_makes_band_dim_then_normal(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "ab");
	apply(e, EVENT_TOGGLE_DIM);

	struct grid *dimmed = frame(e);

	int col = (COLS - 2) / 2 + 1;
	assert_text(dimmed, 1, col, "ab", DIM);

	grid_free(dimmed);

	apply(e, EVENT_TOGGLE_DIM);

	struct grid *normal = frame(e);

	assert_text(normal, 1, col, "ab", INK);

	grid_free(normal);
	editor_free(e);
}

static void test_toggle_only_changes_highlighted_band(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	type(e, "a");
	apply(e, EVENT_SELECT_UP);
	type(e, "b");
	apply(e, EVENT_SELECT_UP);
	type(e, "c");
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_TOGGLE_DIM);

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 1, col, "c", INK);
	assert_text(g, 2, col, "b", INK);
	assert_text(g, 3, col, "a", DIM);

	grid_free(g);
	editor_free(e);
}

static void test_add_band_after_dim_is_normal_and_old_stays_dim(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 2);
	type(e, "b");
	apply(e, EVENT_TOGGLE_DIM);
	apply(e, EVENT_ADD_BAND);
	type(e, "c");

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_row_blank(g, 1);
	assert_text(g, 2, col, "b", DIM);
	assert_text(g, 3, col, "c", INK);

	grid_free(g);
	editor_free(e);
}

static void test_typing_into_dim_band_keeps_dim(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "ab");
	apply(e, EVENT_TOGGLE_DIM);
	type(e, "cd");

	struct grid *g = frame(e);

	int col = (COLS - 4) / 2 + 1;
	assert_text(g, 1, col, "abcd", DIM);
	assert_blank(g, 1, col + 4);
	assert_caret(g, 1, col + 4);

	grid_free(g);
	editor_free(e);
}

static void test_toggle_with_no_bands_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_TOGGLE_DIM);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_toggle_does_not_change_mode(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	assert(editor_mode(e) == MODE_TYPE);

	apply(e, EVENT_TOGGLE_DIM);
	assert(editor_mode(e) == MODE_TYPE);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 1, (COLS - (int)strlen("")) / 2 + 1);

	grid_free(g);
	editor_free(e);
}

static void test_grow_band_centres_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "a");

	struct grid *one = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(one, 1, col, "a", INK);
	assert_caret(one, 1, col + 1);

	grid_free(one);

	apply(e, EVENT_GROW_BAND);

	struct grid *two = frame(e);

	assert_text(two, 2, col, "a", INK);
	assert_caret(two, 2, col + 1);

	grid_free(two);

	apply(e, EVENT_GROW_BAND);

	struct grid *three = frame(e);

	assert_text(three, 3, col, "a", INK);
	assert_caret(three, 3, col + 1);

	grid_free(three);
	editor_free(e);
}

static void test_grow_middle_band_pushes_band_below(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	type(e, "a");
	apply(e, EVENT_SELECT_UP);
	type(e, "b");
	apply(e, EVENT_SELECT_UP);
	type(e, "c");
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_GROW_BAND);

	struct grid *once = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(once, 1, col, "c", INK);
	assert_text(once, 3, col, "b", INK);
	assert_text(once, 5, col, "a", INK);
	assert_caret(once, 3, col + 1);

	grid_free(once);

	apply(e, EVENT_GROW_BAND);

	struct grid *twice = frame(e);

	assert_text(twice, 1, col, "c", INK);
	assert_text(twice, 4, col, "b", INK);
	assert_text(twice, 7, col, "a", INK);
	assert_caret(twice, 4, col + 1);

	grid_free(twice);
	editor_free(e);
}

static void test_grow_band_has_no_limit(void)
{
	struct editor *e = fresh(COLS, 40);
	add_bands(e, 1);
	type(e, "a");
	for (int i = 0; i < 10; i++)
		apply(e, EVENT_GROW_BAND);

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 11, col, "a", INK);
	assert_caret(g, 11, col + 1);

	grid_free(g);
	editor_free(e);
}

static void test_add_band_after_grow_is_one_line(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "a");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ADD_BAND);
	type(e, "b");

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 2, col, "a", INK);
	assert_text(g, 4, col, "b", INK);
	assert_caret(g, 4, col + 1);

	grid_free(g);
	editor_free(e);
}

static void test_typing_into_grown_band_keeps_caret_middle(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "ab");
	apply(e, EVENT_GROW_BAND);
	type(e, "c");

	struct grid *three = frame(e);

	int col = (COLS - 3) / 2 + 1;
	assert_text(three, 2, col, "abc", INK);
	assert_caret(three, 2, col + 3);

	grid_free(three);

	apply(e, EVENT_BACKSPACE);

	struct grid *two = frame(e);

	col = (COLS - 2) / 2 + 1;
	assert_text(two, 2, col, "ab", INK);
	assert_caret(two, 2, col + 2);

	grid_free(two);
	editor_free(e);
}

static void test_grow_with_no_bands_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_GROW_BAND);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_grow_does_not_change_mode(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_ESCAPE);
	assert(editor_mode(e) == MODE_MOVE);

	apply(e, EVENT_GROW_BAND);
	assert(editor_mode(e) == MODE_MOVE);

	struct grid *g = frame(e);

	assert_bg_rows(g, 1, 3, HIGHLIGHT);
	assert_bg_rows(g, 4, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_grow_clips_band_below_bottom(void)
{
	struct editor *e = fresh(COLS, 4);
	add_bands(e, 1);
	type(e, "a");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ADD_BAND);
	type(e, "b");

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 3, col, "a", INK);
	for (int row = 1; row <= e_rows; row++)
		if (row != 3)
			assert_row_blank(g, row);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_grow_clips_band_with_only_padding_visible(void)
{
	struct editor *e = fresh(COLS, 4);
	add_bands(e, 1);
	type(e, "a");
	for (int i = 0; i < 4; i++)
		apply(e, EVENT_GROW_BAND);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, 4, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_shrink_band_centres_text_again(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "a");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SHRINK_BAND);

	struct grid *shrunk = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(shrunk, 2, col, "a", INK);
	assert_caret(shrunk, 2, col + 1);

	grid_free(shrunk);

	apply(e, EVENT_SHRINK_BAND);

	struct grid *plain = frame(e);

	assert_text(plain, 1, col, "a", INK);
	assert_caret(plain, 1, col + 1);

	grid_free(plain);
	editor_free(e);
}

static void test_shrink_one_line_band_stays_one_line(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "a");
	apply(e, EVENT_SHRINK_BAND);

	struct grid *same = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(same, 1, col, "a", INK);
	assert_caret(same, 1, col + 1);

	grid_free(same);

	apply(e, EVENT_GROW_BAND);

	struct grid *grown = frame(e);

	assert_text(grown, 2, col, "a", INK);
	assert_caret(grown, 2, col + 1);

	grid_free(grown);
	editor_free(e);
}

static void test_shrink_middle_band_pulls_band_below_up(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	type(e, "a");
	apply(e, EVENT_SELECT_UP);
	type(e, "b");
	apply(e, EVENT_SELECT_UP);
	type(e, "c");
	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SHRINK_BAND);

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 1, col, "c", INK);
	assert_text(g, 3, col, "b", INK);
	assert_text(g, 5, col, "a", INK);

	grid_free(g);
	editor_free(e);
}

static void test_grow_then_shrink_restores_layout(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	type(e, "a");
	apply(e, EVENT_SELECT_UP);
	type(e, "b");
	apply(e, EVENT_SELECT_UP);
	type(e, "c");
	apply(e, EVENT_SELECT_DOWN);

	struct grid *before = frame(e);

	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SHRINK_BAND);
	apply(e, EVENT_SHRINK_BAND);

	struct grid *after = frame(e);

	assert_same_frame(before, after);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_typing_into_shrunk_band_keeps_caret_middle(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "ab");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SHRINK_BAND);
	type(e, "c");

	struct grid *g = frame(e);

	int col = (COLS - 3) / 2 + 1;
	assert_text(g, 2, col, "abc", INK);
	assert_caret(g, 2, col + 3);

	grid_free(g);
	editor_free(e);
}

static void test_shrink_with_no_bands_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_SHRINK_BAND);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_shrink_does_not_change_mode(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_ESCAPE);
	assert(editor_mode(e) == MODE_MOVE);

	apply(e, EVENT_SHRINK_BAND);
	assert(editor_mode(e) == MODE_MOVE);

	struct grid *g = frame(e);

	assert_bg_rows(g, 1, 1, HIGHLIGHT);
	assert_bg_rows(g, 2, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_shrink_brings_clipped_band_back(void)
{
	struct editor *e = fresh(COLS, 4);
	add_bands(e, 1);
	type(e, "a");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ADD_BAND);
	type(e, "b");

	struct grid *clipped = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(clipped, 3, col, "a", INK);
	assert_row_blank(clipped, 4);
	assert_no_caret(clipped);

	grid_free(clipped);

	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SHRINK_BAND);

	struct grid *back = frame(e);

	assert_text(back, 2, col, "a", INK);
	assert_text(back, 4, col, "b", INK);
	assert_caret(back, 2, col + 1);

	grid_free(back);
	editor_free(e);
}

static void test_move_mode_highlights_selected_and_hides_caret(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_ESCAPE);

	struct grid *g = frame(e);

	assert_bg_rows(g, 3, 3, HIGHLIGHT);
	assert_bg_rows(g, 1, 2, CANVAS);
	assert_bg_rows(g, 4, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_band_carries_its_own_pad(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_GROW_BAND);

	struct grid *first = frame(e);

	assert_bg_rows(first, 1, 3, HIGHLIGHT);
	assert_bg_rows(first, 4, ROWS, CANVAS);

	grid_free(first);

	apply(e, EVENT_SELECT_DOWN);

	struct grid *second = frame(e);

	assert_bg_rows(second, 4, 8, HIGHLIGHT);
	assert_bg_rows(second, 1, 3, CANVAS);
	assert_bg_rows(second, 9, ROWS, CANVAS);

	grid_free(second);

	apply(e, EVENT_SELECT_DOWN);

	struct grid *third = frame(e);

	assert_bg_rows(third, 9, 9, HIGHLIGHT);
	assert_bg_rows(third, 1, 8, CANVAS);
	assert_bg_rows(third, 10, ROWS, CANVAS);

	grid_free(third);
	editor_free(e);
}

static void test_only_selected_band_is_highlighted(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_ESCAPE);

	struct grid *g = frame(e);

	assert_bg_rows(g, 3, 3, HIGHLIGHT);
	assert_bg_rows(g, 1, 2, CANVAS);
	assert_bg_rows(g, 4, ROWS, CANVAS);

	grid_free(g);
	editor_free(e);
}

static void test_highlight_moves_with_selection(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_ESCAPE);

	struct grid *bottom = frame(e);

	assert_bg_rows(bottom, 3, 3, HIGHLIGHT);
	assert_bg_rows(bottom, 1, 2, CANVAS);

	grid_free(bottom);

	apply(e, EVENT_SELECT_UP);

	struct grid *middle = frame(e);

	assert_bg_rows(middle, 2, 2, HIGHLIGHT);
	assert_bg_rows(middle, 1, 1, CANVAS);
	assert_bg_rows(middle, 3, ROWS, CANVAS);

	grid_free(middle);

	apply(e, EVENT_SELECT_DOWN);

	struct grid *back = frame(e);

	assert_bg_rows(back, 3, 3, HIGHLIGHT);
	assert_bg_rows(back, 1, 2, CANVAS);

	grid_free(back);
	editor_free(e);
}

static void test_dim_selected_band_is_highlighted(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "a");
	apply(e, EVENT_TOGGLE_DIM);
	apply(e, EVENT_ESCAPE);

	struct grid *g = frame(e);

	int col = (COLS - 1) / 2 + 1;
	assert_text(g, 1, col, "a", DIM);
	assert_bg_rows(g, 1, 1, HIGHLIGHT);
	assert_bg_rows(g, 2, ROWS, CANVAS);

	grid_free(g);
	editor_free(e);
}

static void test_empty_selected_band_is_highlighted(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_ESCAPE);

	struct grid *g = frame(e);

	assert_bg_rows(g, 1, 1, HIGHLIGHT);
	assert_bg_rows(g, 2, ROWS, CANVAS);
	assert_row_blank(g, 1);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_type_mode_has_no_highlight_and_visible_caret(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "ab");

	int col = (COLS - 2) / 2 + 1;

	struct grid *typed = frame(e);

	assert_bg_rows(typed, 1, ROWS, CANVAS);
	assert_text(typed, 1, col, "ab", INK);
	assert_caret(typed, 1, col + 2);

	grid_free(typed);

	apply(e, EVENT_ESCAPE);

	struct grid *moved = frame(e);

	assert_bg_rows(moved, 1, 1, HIGHLIGHT);
	assert_no_caret(moved);

	grid_free(moved);

	apply(e, EVENT_ENTER_TYPE);

	struct grid *retyped = frame(e);

	assert_bg_rows(retyped, 1, ROWS, CANVAS);
	assert_caret(retyped, 1, col + 2);

	grid_free(retyped);
	editor_free(e);
}

static void test_add_text_moves_old_text_left_and_types_on_the_right(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);

	struct grid *g = frame(e);

	int right = COLS - (int)strlen("") - 1;
	assert_text(g, 1, 3, "Login", INK);
	assert_blank(g, 1, 8);
	assert_blank(g, 1, right);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 1, right);

	grid_free(g);
	editor_free(e);
}

static void test_typed_text_on_the_right_is_right_aligned(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");

	struct grid *g = frame(e);

	int right = COLS - (int)strlen("Logout") - 1;
	assert_text(g, 1, 3, "Login", INK);
	assert_text(g, 1, right, "Logout", INK);
	assert_blank(g, 1, right + 6);
	assert_caret(g, 1, right + 6);

	grid_free(g);
	editor_free(e);
}

static void test_add_text_on_a_band_with_two_texts_is_a_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);

	assert(editor_mode(e) == MODE_MOVE);

	struct grid *g = frame(e);

	int right = COLS - (int)strlen("Logout") - 1;
	assert_text(g, 1, 3, "Login", INK);
	assert_text(g, 1, right, "Logout", INK);
	assert_bg_rows(g, 1, 1, HIGHLIGHT);
	assert_bg_rows(g, 2, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_add_text_with_no_bands_is_a_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_TEXT);

	assert(editor_mode(e) == MODE_MOVE);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_add_text_on_an_empty_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);

	int empty_right = COLS - (int)strlen("") - 1;

	struct grid *both_empty = frame(e);

	assert_all_blank(both_empty);
	assert_bg_rows(both_empty, 1, ROWS, CANVAS);
	assert_blank(both_empty, 1, 3);
	assert_blank(both_empty, 1, empty_right);
	assert_caret(both_empty, 1, empty_right);

	grid_free(both_empty);

	type(e, "Logout");

	struct grid *typed = frame(e);

	int right = COLS - (int)strlen("Logout") - 1;
	assert_blank(typed, 1, 3);
	assert_text(typed, 1, right, "Logout", INK);
	assert_caret(typed, 1, empty_right);

	grid_free(typed);
	editor_free(e);
}

static void test_escape_right_after_add_text_keeps_empty_right_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	apply(e, EVENT_ESCAPE);

	struct grid *g = frame(e);

	assert_text(g, 1, 3, "Login", INK);
	assert_blank(g, 1, 8);
	assert_blank(g, 1, COLS - (int)strlen("") - 1);
	assert_bg_rows(g, 1, 1, HIGHLIGHT);
	assert_bg_rows(g, 2, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_enter_type_on_two_texts_types_into_the_right_one(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Lo");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ENTER_TYPE);
	type(e, "gout");

	struct grid *g = frame(e);

	int right = COLS - (int)strlen("Logout") - 1;
	assert_text(g, 1, 3, "Login", INK);
	assert_text(g, 1, right, "Logout", INK);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 1, right + 6);

	grid_free(g);
	editor_free(e);
}

static void test_single_text_in_a_tall_band_stays_on_the_band_row(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ESCAPE);

	struct grid *g = frame(e);

	assert_text(g, 2, (COLS - (int)strlen("Login")) / 2 + 1, "Login", INK);
	assert_bg_rows(g, 1, 3, HIGHLIGHT);
	assert_bg_rows(g, 4, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_two_texts_on_a_one_line_band_stay_side_by_side(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");

	struct grid *g = frame(e);

	int right = COLS - (int)strlen("Logout") - 1;
	assert_text(g, 1, 3, "Login", INK);
	assert_text(g, 1, right, "Logout", INK);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 1, right + 6);

	grid_free(g);
	editor_free(e);
}

static void test_add_text_on_a_tall_band_stacks_the_texts(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);

	struct grid *g = frame(e);

	int top = (COLS - (int)strlen("Login")) / 2 + 1;
	int bottom = (COLS - (int)strlen("")) / 2 + 1;
	assert_text(g, 1, top, "Login", INK);
	assert_blank(g, 3, bottom);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 3, bottom);
	assert(editor_mode(e) == MODE_TYPE);

	grid_free(g);
	editor_free(e);
}

static void test_typed_text_on_a_tall_band_is_centred_below(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");

	struct grid *g = frame(e);

	assert_text(g, 2, (COLS - (int)strlen("Login")) / 2 + 1, "Login", INK);
	assert_text(g, 4, (COLS - (int)strlen("Logout")) / 2 + 1, "Logout",
		    INK);
	assert_caret(g, 4, (COLS - (int)strlen("Logout")) / 2 + 1 + 6);

	grid_free(g);
	editor_free(e);
}

static void
test_escape_right_after_add_text_on_a_tall_band_keeps_empty_bottom_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	apply(e, EVENT_ESCAPE);

	struct grid *g = frame(e);

	assert_text(g, 1, (COLS - (int)strlen("Login")) / 2 + 1, "Login", INK);
	assert_blank(g, 3, (COLS - (int)strlen("")) / 2 + 1);
	assert_bg_rows(g, 1, 3, HIGHLIGHT);
	assert_bg_rows(g, 4, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_enter_type_on_a_stacked_band_types_into_the_bottom_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Lo");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ENTER_TYPE);
	type(e, "gout");

	struct grid *g = frame(e);

	int bottom = (COLS - (int)strlen("Logout")) / 2 + 1;
	assert_text(g, 1, (COLS - (int)strlen("Login")) / 2 + 1, "Login", INK);
	assert_text(g, 3, bottom, "Logout", INK);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 3, bottom + 6);

	grid_free(g);
	editor_free(e);
}

static void test_growing_and_shrinking_flips_the_arrangement(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);

	int side_by_side_left = 3;
	int side_by_side_right = COLS - (int)strlen("Logout") - 1;
	int stacked = (COLS - (int)strlen("Logout")) / 2 + 1;

	struct grid *side_by_side = frame(e);

	assert_text(side_by_side, 1, side_by_side_left, "Login", INK);
	assert_text(side_by_side, 1, side_by_side_right, "Logout", INK);
	assert_bg_rows(side_by_side, 1, 1, HIGHLIGHT);
	assert_no_caret(side_by_side);

	grid_free(side_by_side);

	apply(e, EVENT_GROW_BAND);

	struct grid *stacked_frame = frame(e);

	assert_text(stacked_frame, 1, stacked, "Login", INK);
	assert_text(stacked_frame, 3, stacked, "Logout", INK);
	assert_bg_rows(stacked_frame, 1, 3, HIGHLIGHT);
	assert_no_caret(stacked_frame);

	grid_free(stacked_frame);

	apply(e, EVENT_SHRINK_BAND);

	struct grid *back = frame(e);

	assert_text(back, 1, side_by_side_left, "Login", INK);
	assert_text(back, 1, side_by_side_right, "Logout", INK);
	assert_bg_rows(back, 1, 1, HIGHLIGHT);
	assert_no_caret(back);

	grid_free(back);
	editor_free(e);
}

static void test_step_in_highlights_the_first_of_two_texts(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct grid *g = frame(e);

	int left = 3;
	int right = COLS - (int)strlen("Logout") - 1;
	assert_text(g, 1, left, "Login", INK);
	assert_text(g, 1, right, "Logout", INK);
	assert_bg(g, 1, 2, CANVAS);
	assert_bg(g, 1, left, HIGHLIGHT);
	assert_bg(g, 1, left + 4, HIGHLIGHT);
	assert_bg(g, 1, right, CANVAS);
	assert_bg(g, 1, right + 5, CANVAS);
	assert_bg_rows(g, 2, ROWS, CANVAS);
	assert_no_caret(g);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(g);
	editor_free(e);
}

static void test_step_in_highlights_a_lone_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct grid *g = frame(e);

	int col = (COLS - (int)strlen("Login")) / 2 + 1;
	assert_text(g, 1, col, "Login", INK);
	assert_bg(g, 1, col, HIGHLIGHT);
	assert_bg(g, 1, col + 4, HIGHLIGHT);
	assert_bg(g, 1, col - 1, CANVAS);
	assert_bg(g, 1, col + 5, CANVAS);
	assert_bg_rows(g, 2, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_step_in_highlight_matches_row_highlight(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);

	struct grid *row = frame(e);

	assert_bg_rows(row, 1, 1, HIGHLIGHT);
	int col = (COLS - (int)strlen("Login")) / 2 + 1;

	grid_free(row);

	apply(e, EVENT_STEP_IN);

	struct grid *g = frame(e);

	assert_bg(g, 1, col, HIGHLIGHT);
	assert_bg(g, 1, col + 4, HIGHLIGHT);
	assert_bg(g, 1, col - 1, CANVAS);
	assert_bg(g, 1, col + 5, CANVAS);

	grid_free(g);
	editor_free(e);
}

static void test_step_out_brings_back_the_row_highlight(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);
	apply(e, EVENT_STEP_OUT);

	struct grid *g = frame(e);

	assert_bg_rows(g, 1, 1, HIGHLIGHT);
	assert_bg_rows(g, 2, ROWS, CANVAS);
	assert_text(g, 1, 3, "Login", INK);
	assert_text(g, 1, COLS - (int)strlen("Logout") - 1, "Logout", INK);
	assert_no_caret(g);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(g);
	editor_free(e);
}

static void test_step_in_then_j_and_k_change_nothing(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_BAND);
	type(e, "x");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_STEP_IN);

	struct grid *before = frame(e);

	apply(e, EVENT_SELECT_DOWN);
	apply(e, EVENT_SELECT_UP);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_step_in_then_band_keys_change_nothing(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct grid *before = frame(e);

	type(e, "iod-[]a");
	apply(e, EVENT_STEP_IN);
	apply(e, EVENT_ADD_TEXT);
	apply(e, EVENT_DELETE_BAND);
	/* spec gap: - on a selected text now flips it, see
	 * docs/specs/035-dim-one-text.md AC1 */
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SHRINK_BAND);
	apply(e, EVENT_ENTER_TYPE);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_toggle_dim_on_a_text_leaves_its_sibling_alone(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	apply(e, EVENT_TOGGLE_DIM);

	struct grid *dimmed = frame(e);

	int right = COLS - (int)strlen("Logout") - 1;
	assert_text(dimmed, 1, 3, "Login", DIM);
	assert_text(dimmed, 1, right, "Logout", INK);

	grid_free(dimmed);

	apply(e, EVENT_TOGGLE_DIM);

	struct grid *back = frame(e);

	assert_text(back, 1, 3, "Login", INK);
	assert_text(back, 1, right, "Logout", INK);

	grid_free(back);
	editor_free(e);
}

static void test_toggle_dim_on_a_text_greyed_by_the_row(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_TOGGLE_DIM);
	apply(e, EVENT_STEP_IN);

	apply(e, EVENT_TOGGLE_DIM);

	struct grid *g = frame(e);

	int right = COLS - (int)strlen("Logout") - 1;
	assert_text(g, 1, 3, "Login", INK);
	assert_text(g, 1, right, "Logout", DIM);

	grid_free(g);
	editor_free(e);
}

static void test_toggle_dim_on_a_band_swaps_mixed_texts(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_TOGGLE_DIM);
	apply(e, EVENT_STEP_IN);
	apply(e, EVENT_TOGGLE_DIM);
	apply(e, EVENT_STEP_OUT);

	apply(e, EVENT_TOGGLE_DIM);

	struct grid *g = frame(e);

	int right = COLS - (int)strlen("Logout") - 1;
	assert_text(g, 1, 3, "Login", DIM);
	assert_text(g, 1, right, "Logout", INK);

	grid_free(g);
	editor_free(e);
}

static void test_step_in_with_nothing_selected_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_STEP_IN);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(g);
	editor_free(e);
}

static void test_step_out_on_a_whole_row_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);

	struct grid *before = frame(e);

	apply(e, EVENT_STEP_OUT);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_typing_and_backspace_still_delete_in_type_mode(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "ab");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);
	apply(e, EVENT_STEP_OUT);
	apply(e, EVENT_ENTER_TYPE);
	type(e, "cd");
	apply(e, EVENT_BACKSPACE);

	struct grid *g = frame(e);

	int col = (COLS - (int)strlen("abc")) / 2 + 1;
	assert_text(g, 1, col, "abc", INK);
	assert_blank(g, 1, col + 3);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 1, col + 3);
	assert(editor_mode(e) == MODE_TYPE);

	grid_free(g);
	editor_free(e);
}

static void test_step_in_in_type_mode_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");

	struct grid *before = frame(e);

	apply(e, EVENT_STEP_IN);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_TYPE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_step_in_on_an_empty_text_shows_no_highlight(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct grid *g = frame(e);

	assert_all_blank(g);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_no_caret(g);

	grid_free(g);
	editor_free(e);
}

static void test_next_text_highlights_the_right_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct grid *first = frame(e);

	int left = 3;
	int right = COLS - (int)strlen("Logout") - 1;
	assert_bg(first, 1, left, HIGHLIGHT);
	assert_bg(first, 1, right, CANVAS);

	grid_free(first);

	apply_and_layout(e, EVENT_SELECT_NEXT_TEXT);

	struct grid *g = frame(e);

	assert_text(g, 1, left, "Login", INK);
	assert_text(g, 1, right, "Logout", INK);
	assert_bg(g, 1, left, CANVAS);
	assert_bg(g, 1, left + 4, CANVAS);
	assert_bg(g, 1, right, HIGHLIGHT);
	assert_bg(g, 1, right + 5, HIGHLIGHT);
	assert_bg_rows(g, 2, ROWS, CANVAS);
	assert_no_caret(g);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(g);
	editor_free(e);
}

static void test_prev_text_highlights_the_left_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	int left = 3;
	int right = COLS - (int)strlen("Logout") - 1;

	apply_and_layout(e, EVENT_SELECT_NEXT_TEXT);

	struct grid *second = frame(e);

	assert_bg(second, 1, right, HIGHLIGHT);
	assert_bg(second, 1, left, CANVAS);

	grid_free(second);

	apply_and_layout(e, EVENT_SELECT_PREV_TEXT);

	struct grid *g = frame(e);

	assert_text(g, 1, left, "Login", INK);
	assert_text(g, 1, right, "Logout", INK);
	assert_bg(g, 1, left, HIGHLIGHT);
	assert_bg(g, 1, left + 4, HIGHLIGHT);
	assert_bg(g, 1, right, CANVAS);
	assert_bg(g, 1, right + 5, CANVAS);
	assert_bg_rows(g, 2, ROWS, CANVAS);
	assert_no_caret(g);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(g);
	editor_free(e);
}

static void test_prev_text_on_the_left_text_is_a_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct grid *before = frame(e);

	apply_and_layout(e, EVENT_SELECT_PREV_TEXT);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_next_text_on_the_right_text_is_a_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	apply_and_layout(e, EVENT_SELECT_NEXT_TEXT);

	struct grid *before = frame(e);

	apply_and_layout(e, EVENT_SELECT_NEXT_TEXT);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_prev_and_next_text_in_a_one_text_row_are_noops(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct grid *before = frame(e);

	apply_and_layout(e, EVENT_SELECT_PREV_TEXT);
	apply_and_layout(e, EVENT_SELECT_NEXT_TEXT);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_prev_and_next_text_keep_the_text_of_the_row_selected(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	int left = 3;
	int right = COLS - (int)strlen("Logout") - 1;

	apply_and_layout(e, EVENT_SELECT_NEXT_TEXT);
	apply_and_layout(e, EVENT_SELECT_PREV_TEXT);
	apply_and_layout(e, EVENT_SELECT_NEXT_TEXT);

	struct grid *text = frame(e);

	assert_bg(text, 1, right, HIGHLIGHT);
	assert_bg(text, 1, right + 5, HIGHLIGHT);
	assert_bg(text, 1, left, CANVAS);
	assert_no_caret(text);

	grid_free(text);

	apply(e, EVENT_BACKSPACE);

	struct grid *backspaced = frame(e);

	assert_bg(backspaced, 1, right, HIGHLIGHT);
	assert_bg(backspaced, 1, left, CANVAS);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(backspaced);

	apply(e, EVENT_STEP_OUT);

	struct grid *row = frame(e);

	assert_bg_rows(row, 1, 1, HIGHLIGHT);
	assert_bg_rows(row, 2, ROWS, CANVAS);

	grid_free(row);
	editor_free(e);
}

static void test_prev_and_next_text_in_a_stacked_row_are_noops(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_STEP_IN);

	struct grid *before = frame(e);

	apply_and_layout(e, EVENT_SELECT_PREV_TEXT);
	apply_and_layout(e, EVENT_SELECT_NEXT_TEXT);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_prev_and_next_text_on_a_selected_band_are_noops(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_ADD_TEXT);
	type(e, "Logout");
	apply(e, EVENT_ESCAPE);

	struct grid *before = frame(e);

	apply_and_layout(e, EVENT_SELECT_PREV_TEXT);
	apply_and_layout(e, EVENT_SELECT_NEXT_TEXT);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_MOVE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_next_text_in_type_mode_is_a_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "Login");

	struct grid *before = frame(e);

	apply(e, EVENT_SELECT_NEXT_TEXT);

	struct grid *after = frame(e);

	assert_same_frame(before, after);
	assert(editor_mode(e) == MODE_TYPE);

	grid_free(before);
	grid_free(after);
	editor_free(e);
}

static void test_typing_h_and_l_in_type_mode_inserts_characters(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply_ch(e, 'h');
	apply_ch(e, 'l');

	struct grid *g = frame(e);

	int col = (COLS - 2) / 2 + 1;
	assert_text(g, 1, col, "hl", INK);
	assert_bg_rows(g, 1, ROWS, CANVAS);
	assert_caret(g, 1, col + 2);
	assert(editor_mode(e) == MODE_TYPE);

	grid_free(g);
	editor_free(e);
}

int main(void)
{
	test_new_editor_is_empty();
	test_add_band();
	test_new_band_text_is_normal();
	test_type_text();
	test_backspace();
	test_backspace_on_empty_is_noop();
	test_three_bands_stack_downwards();
	test_escape_and_enter_type_change_mode();
	test_select_up_moves_caret();
	test_select_up_at_top_stays();
	test_select_down_at_bottom_stays();
	test_select_down_moves_caret();
	test_typing_after_select_up_edits_older_band();
	test_delete_middle_of_three();
	test_delete_top_band_selects_below();
	test_delete_only_band();
	test_delete_with_no_bands_is_noop();
	test_char_and_backspace_with_no_bands();
	test_quit_and_none_are_noops();
	test_clips_to_screen_rows();
	test_capacity_is_cols_minus_one();
	test_toggle_makes_band_dim_then_normal();
	test_toggle_only_changes_highlighted_band();
	test_add_band_after_dim_is_normal_and_old_stays_dim();
	test_typing_into_dim_band_keeps_dim();
	test_toggle_with_no_bands_is_noop();
	test_toggle_does_not_change_mode();
	test_grow_band_centres_text();
	test_grow_middle_band_pushes_band_below();
	test_grow_band_has_no_limit();
	test_add_band_after_grow_is_one_line();
	test_typing_into_grown_band_keeps_caret_middle();
	test_grow_with_no_bands_is_noop();
	test_grow_does_not_change_mode();
	test_grow_clips_band_below_bottom();
	test_grow_clips_band_with_only_padding_visible();
	test_shrink_band_centres_text_again();
	test_shrink_one_line_band_stays_one_line();
	test_shrink_middle_band_pulls_band_below_up();
	test_grow_then_shrink_restores_layout();
	test_typing_into_shrunk_band_keeps_caret_middle();
	test_shrink_with_no_bands_is_noop();
	test_shrink_does_not_change_mode();
	test_shrink_brings_clipped_band_back();
	test_move_mode_highlights_selected_and_hides_caret();
	test_band_carries_its_own_pad();
	test_only_selected_band_is_highlighted();
	test_highlight_moves_with_selection();
	test_dim_selected_band_is_highlighted();
	test_empty_selected_band_is_highlighted();
	test_type_mode_has_no_highlight_and_visible_caret();
	test_add_text_moves_old_text_left_and_types_on_the_right();
	test_typed_text_on_the_right_is_right_aligned();
	test_add_text_on_a_band_with_two_texts_is_a_noop();
	test_add_text_with_no_bands_is_a_noop();
	test_add_text_on_an_empty_text();
	test_escape_right_after_add_text_keeps_empty_right_text();
	test_enter_type_on_two_texts_types_into_the_right_one();
	test_single_text_in_a_tall_band_stays_on_the_band_row();
	test_two_texts_on_a_one_line_band_stay_side_by_side();
	test_add_text_on_a_tall_band_stacks_the_texts();
	test_typed_text_on_a_tall_band_is_centred_below();
	test_escape_right_after_add_text_on_a_tall_band_keeps_empty_bottom_text();
	test_enter_type_on_a_stacked_band_types_into_the_bottom_text();
	test_growing_and_shrinking_flips_the_arrangement();
	test_step_in_highlights_the_first_of_two_texts();
	test_step_in_highlights_a_lone_text();
	test_step_in_highlight_matches_row_highlight();
	test_step_out_brings_back_the_row_highlight();
	test_step_in_then_j_and_k_change_nothing();
	test_step_in_then_band_keys_change_nothing();
	test_toggle_dim_on_a_text_leaves_its_sibling_alone();
	test_toggle_dim_on_a_text_greyed_by_the_row();
	test_toggle_dim_on_a_band_swaps_mixed_texts();
	test_step_in_with_nothing_selected_is_noop();
	test_step_out_on_a_whole_row_is_noop();
	test_typing_and_backspace_still_delete_in_type_mode();
	test_step_in_in_type_mode_is_noop();
	test_step_in_on_an_empty_text_shows_no_highlight();
	test_next_text_highlights_the_right_text();
	test_prev_text_highlights_the_left_text();
	test_prev_text_on_the_left_text_is_a_noop();
	test_next_text_on_the_right_text_is_a_noop();
	test_prev_and_next_text_in_a_one_text_row_are_noops();
	test_prev_and_next_text_keep_the_text_of_the_row_selected();
	test_prev_and_next_text_in_a_stacked_row_are_noops();
	test_prev_and_next_text_on_a_selected_band_are_noops();
	test_next_text_in_type_mode_is_a_noop();
	test_typing_h_and_l_in_type_mode_inserts_characters();
	return 0;
}