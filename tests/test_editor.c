#include <assert.h>
#include <string.h>

#include "editor.h"

#define COLS 40
#define ROWS 40

static void apply(struct editor *e, enum key_event_type type)
{
	editor_apply(e, (struct key_event){.type = type, .ch = 0});
}

static void apply_ch(struct editor *e, char ch)
{
	editor_apply(e, (struct key_event){.type = EVENT_CHAR, .ch = ch});
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
	return e;
}

static void test_new_editor_is_empty(void)
{
	struct editor *e = fresh(COLS, ROWS);

	struct layout l = layout(e);
	assert(l.count == 0);
	assert(l.caret_visible == 0);
	assert(editor_mode(e) == MODE_MOVE);

	editor_free(e);
}

static void test_add_band(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.labels[0].row == 1);
	assert(l.labels[0].len == 0);
	assert(l.labels[0].col == COLS / 2 + 1);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 1);
	assert(l.caret_col == COLS / 2 + 1);
	assert(editor_mode(e) == MODE_TYPE);

	editor_free(e);
}

static void test_new_band_label_is_normal(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);

	struct layout l = layout(e);
	assert(l.labels[0].style.dim == 0);

	editor_free(e);
}

static void test_type_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "hi");

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.labels[0].len == 2);
	assert(strcmp(l.labels[0].text, "hi") == 0);
	assert(l.labels[0].col == (COLS - 2) / 2 + 1);
	assert(l.caret_col == l.labels[0].col + 2);

	editor_free(e);
}

static void test_backspace(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "hi");
	apply(e, EVENT_BACKSPACE);

	struct layout l = layout(e);
	assert(l.labels[0].len == 1);
	assert(strcmp(l.labels[0].text, "h") == 0);

	editor_free(e);
}

static void test_backspace_on_empty_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_BACKSPACE);

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.labels[0].len == 0);
	assert(l.caret_col == l.labels[0].col);

	editor_free(e);
}

static void test_three_bands_stack_downwards(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);

	struct layout l = layout(e);
	assert(l.count == 3);
	assert(l.labels[0].row == 1);
	assert(l.labels[1].row == 2);
	assert(l.labels[2].row == 3);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 3);

	editor_free(e);
}

static void test_escape_and_enter_type_change_mode(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	assert(editor_mode(e) == MODE_TYPE);

	apply(e, EVENT_ESCAPE);
	assert(editor_mode(e) == MODE_MOVE);

	apply(e, EVENT_ENTER_TYPE);
	assert(editor_mode(e) == MODE_TYPE);

	editor_free(e);
}

static void test_select_up_moves_caret(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_UP);

	struct layout l = layout(e);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 2);

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

	struct layout l = layout(e);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 1);

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

	struct layout l = layout(e);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 3);

	editor_free(e);
}

static void test_select_down_moves_caret(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SELECT_DOWN);

	struct layout l = layout(e);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 3);

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

	struct layout l = layout(e);
	assert(l.count == 2);
	assert(l.labels[0].len == 4);
	assert(strcmp(l.labels[0].text, "newX") == 0);
	assert(l.labels[1].len == 3);
	assert(strcmp(l.labels[1].text, "old") == 0);
	assert(l.caret_row == 1);

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

	struct layout l = layout(e);
	assert(l.count == 2);
	assert(l.labels[0].row == 1);
	assert(l.labels[1].row == 2);
	assert(strcmp(l.labels[0].text, "") == 0);
	assert(strcmp(l.labels[1].text, "a") == 0);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 1);

	editor_free(e);
}

static void test_delete_top_band_selects_below(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_DELETE_BAND);

	struct layout l = layout(e);
	assert(l.count == 2);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 1);

	editor_free(e);
}

static void test_delete_only_band(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_DELETE_BAND);

	struct layout l = layout(e);
	assert(l.count == 0);
	assert(l.caret_visible == 0);

	editor_free(e);
}

static void test_delete_with_no_bands_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_DELETE_BAND);

	struct layout l = layout(e);
	assert(l.count == 0);
	assert(l.caret_visible == 0);

	editor_free(e);
}

static void test_char_and_backspace_with_no_bands(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply_ch(e, 'x');
	apply(e, EVENT_BACKSPACE);

	struct layout l = layout(e);
	assert(l.count == 0);
	assert(l.caret_visible == 0);

	editor_free(e);
}

static void test_quit_and_none_are_noops(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_QUIT);
	apply(e, EVENT_NONE);

	struct layout l = layout(e);
	assert(l.count == 0);
	assert(editor_mode(e) == MODE_MOVE);

	editor_free(e);
}

static void test_clips_to_screen_rows(void)
{
	struct editor *e = fresh(COLS, 2);
	add_bands(e, 3);

	struct layout l = layout(e);
	assert(l.count == 2);
	assert(l.labels[0].row == 1);
	assert(l.labels[1].row == 2);
	assert(l.caret_visible == 0);

	apply(e, EVENT_SELECT_UP);
	l = layout(e);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 2);

	editor_free(e);
}

static void test_capacity_is_cols_minus_one(void)
{
	struct editor *e = fresh(10, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "abcdefghij");

	struct layout l = layout(e);
	assert(l.labels[0].len == 9);
	assert(strcmp(l.labels[0].text, "abcdefghi") == 0);

	editor_free(e);
}

static void test_toggle_makes_band_dim_then_normal(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_TOGGLE_DIM);

	struct layout l = layout(e);
	assert(l.labels[0].style.dim == 1);

	apply(e, EVENT_TOGGLE_DIM);
	l = layout(e);
	assert(l.labels[0].style.dim == 0);

	editor_free(e);
}

static void test_toggle_only_changes_highlighted_band(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_TOGGLE_DIM);

	struct layout l = layout(e);
	assert(l.labels[2].style.dim == 1);
	assert(l.labels[0].style.dim == 0);
	assert(l.labels[1].style.dim == 0);

	editor_free(e);
}

static void test_add_band_after_dim_is_normal_and_old_stays_dim(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 2);
	apply(e, EVENT_TOGGLE_DIM);
	apply(e, EVENT_ADD_BAND);

	struct layout l = layout(e);
	assert(l.labels[2].style.dim == 0);
	assert(l.labels[1].style.dim == 1);

	editor_free(e);
}

static void test_typing_into_dim_band_keeps_dim(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "ab");
	apply(e, EVENT_TOGGLE_DIM);
	type(e, "cd");

	struct layout l = layout(e);
	assert(l.labels[0].style.dim == 1);
	assert(l.labels[0].len == 4);
	assert(strcmp(l.labels[0].text, "abcd") == 0);

	editor_free(e);
}

static void test_toggle_with_no_bands_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_TOGGLE_DIM);

	struct layout l = layout(e);
	assert(l.count == 0);

	editor_free(e);
}

static void test_toggle_does_not_change_mode(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	assert(editor_mode(e) == MODE_TYPE);

	apply(e, EVENT_TOGGLE_DIM);
	assert(editor_mode(e) == MODE_TYPE);

	editor_free(e);
}

int main(void)
{
	test_new_editor_is_empty();
	test_add_band();
	test_new_band_label_is_normal();
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
	return 0;
}