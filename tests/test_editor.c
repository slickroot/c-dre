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
	assert(l.bands[0].row == 1);
	assert(l.bands[0].texts[0].len == 0);
	assert(l.bands[0].texts[0].col == COLS / 2 + 1);
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
	assert(l.bands[0].style.dim == 0);

	editor_free(e);
}

static void test_type_text(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "hi");

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].texts[0].len == 2);
	assert(strcmp(l.bands[0].texts[0].text, "hi") == 0);
	assert(l.bands[0].texts[0].col == (COLS - 2) / 2 + 1);
	assert(l.caret_col == l.bands[0].texts[0].col + 2);

	editor_free(e);
}

static void test_backspace(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "hi");
	apply(e, EVENT_BACKSPACE);

	struct layout l = layout(e);
	assert(l.bands[0].texts[0].len == 1);
	assert(strcmp(l.bands[0].texts[0].text, "h") == 0);

	editor_free(e);
}

static void test_backspace_on_empty_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	apply(e, EVENT_BACKSPACE);

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].texts[0].len == 0);
	assert(l.caret_col == l.bands[0].texts[0].col);

	editor_free(e);
}

static void test_three_bands_stack_downwards(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);

	struct layout l = layout(e);
	assert(l.count == 3);
	assert(l.bands[0].row == 1);
	assert(l.bands[1].row == 2);
	assert(l.bands[2].row == 3);
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
	assert(l.bands[0].texts[0].len == 4);
	assert(strcmp(l.bands[0].texts[0].text, "newX") == 0);
	assert(l.bands[1].texts[0].len == 3);
	assert(strcmp(l.bands[1].texts[0].text, "old") == 0);
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
	assert(l.bands[0].row == 1);
	assert(l.bands[1].row == 2);
	assert(strcmp(l.bands[0].texts[0].text, "") == 0);
	assert(strcmp(l.bands[1].texts[0].text, "a") == 0);
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
	assert(l.bands[0].row == 1);
	assert(l.bands[1].row == 2);
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
	assert(l.bands[0].texts[0].len == 9);
	assert(strcmp(l.bands[0].texts[0].text, "abcdefghi") == 0);

	editor_free(e);
}

static void test_toggle_makes_band_dim_then_normal(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_TOGGLE_DIM);

	struct layout l = layout(e);
	assert(l.bands[0].style.dim == 1);

	apply(e, EVENT_TOGGLE_DIM);
	l = layout(e);
	assert(l.bands[0].style.dim == 0);

	editor_free(e);
}

static void test_toggle_only_changes_highlighted_band(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_TOGGLE_DIM);

	struct layout l = layout(e);
	assert(l.bands[2].style.dim == 1);
	assert(l.bands[0].style.dim == 0);
	assert(l.bands[1].style.dim == 0);

	editor_free(e);
}

static void test_add_band_after_dim_is_normal_and_old_stays_dim(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 2);
	apply(e, EVENT_TOGGLE_DIM);
	apply(e, EVENT_ADD_BAND);

	struct layout l = layout(e);
	assert(l.bands[2].style.dim == 0);
	assert(l.bands[1].style.dim == 1);

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
	assert(l.bands[0].style.dim == 1);
	assert(l.bands[0].texts[0].len == 4);
	assert(strcmp(l.bands[0].texts[0].text, "abcd") == 0);

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

static void test_grow_band_centres_label(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_GROW_BAND);

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].row == 2);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 2);

	apply(e, EVENT_GROW_BAND);
	l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].row == 3);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 3);

	editor_free(e);
}

static void test_grow_middle_band_pushes_band_below(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_GROW_BAND);

	struct layout l = layout(e);
	assert(l.count == 3);
	assert(l.bands[0].row == 1);
	assert(l.bands[1].row == 3);
	assert(l.bands[2].row == 5);

	apply(e, EVENT_GROW_BAND);
	l = layout(e);
	assert(l.count == 3);
	assert(l.bands[0].row == 1);
	assert(l.bands[1].row == 4);
	assert(l.bands[2].row == 7);

	editor_free(e);
}

static void test_grow_band_has_no_limit(void)
{
	struct editor *e = fresh(COLS, 40);
	add_bands(e, 1);
	for (int i = 0; i < 10; i++)
		apply(e, EVENT_GROW_BAND);

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].row == 11);
	assert(l.caret_row == 11);

	editor_free(e);
}

static void test_add_band_after_grow_is_one_line(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ADD_BAND);

	struct layout l = layout(e);
	assert(l.count == 2);
	assert(l.bands[0].row == 2);
	assert(l.bands[1].row == 4);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 4);

	editor_free(e);
}

static void test_typing_into_grown_band_keeps_caret_middle(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	type(e, "ab");
	apply(e, EVENT_GROW_BAND);
	type(e, "c");

	struct layout l = layout(e);
	assert(l.bands[0].texts[0].len == 3);
	assert(strcmp(l.bands[0].texts[0].text, "abc") == 0);
	assert(l.bands[0].row == 2);
	assert(l.caret_row == 2);
	assert(l.caret_col == l.bands[0].texts[0].col + 3);

	apply(e, EVENT_BACKSPACE);
	l = layout(e);
	assert(l.bands[0].texts[0].len == 2);
	assert(strcmp(l.bands[0].texts[0].text, "ab") == 0);
	assert(l.caret_row == 2);
	assert(l.caret_col == l.bands[0].texts[0].col + 2);

	editor_free(e);
}

static void test_grow_with_no_bands_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_GROW_BAND);

	struct layout l = layout(e);
	assert(l.count == 0);
	assert(l.caret_visible == 0);

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

	editor_free(e);
}

static void test_grow_clips_band_below_bottom(void)
{
	struct editor *e = fresh(COLS, 4);
	add_bands(e, 1);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ADD_BAND);

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].row == 3);
	assert(l.caret_visible == 0);

	editor_free(e);
}

static void test_grow_clips_band_with_only_padding_visible(void)
{
	struct editor *e = fresh(COLS, 4);
	add_bands(e, 1);
	for (int i = 0; i < 4; i++)
		apply(e, EVENT_GROW_BAND);

	struct layout l = layout(e);
	assert(l.count == 0);
	assert(l.caret_visible == 0);

	editor_free(e);
}

static void test_shrink_band_centres_label_again(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SHRINK_BAND);

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].row == 2);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 2);

	apply(e, EVENT_SHRINK_BAND);
	l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].row == 1);
	assert(l.caret_visible == 1);
	assert(l.caret_row == 1);

	editor_free(e);
}

static void test_shrink_one_line_band_stays_one_line(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_SHRINK_BAND);

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].row == 1);
	assert(l.caret_row == 1);

	apply(e, EVENT_GROW_BAND);
	l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].row == 2);
	assert(l.caret_row == 2);

	editor_free(e);
}

static void test_shrink_middle_band_pulls_band_below_up(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SHRINK_BAND);

	struct layout l = layout(e);
	assert(l.count == 3);
	assert(l.bands[0].row == 1);
	assert(l.bands[1].row == 3);
	assert(l.bands[2].row == 5);

	editor_free(e);
}

static void test_grow_then_shrink_restores_layout(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_SELECT_UP);
	struct layout before = layout(e);

	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SHRINK_BAND);
	apply(e, EVENT_SHRINK_BAND);

	struct layout l = layout(e);
	assert(l.count == before.count);
	for (int i = 0; i < before.count; i++) {
		assert(l.bands[i].row == before.bands[i].row);
		assert(l.bands[i].texts[0].col == before.bands[i].texts[0].col);
		assert(strcmp(l.bands[i].texts[0].text,
			      before.bands[i].texts[0].text) == 0);
	}
	assert(l.caret_visible == before.caret_visible);
	assert(l.caret_row == before.caret_row);
	assert(l.caret_col == before.caret_col);

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

	struct layout l = layout(e);
	assert(l.bands[0].texts[0].len == 3);
	assert(strcmp(l.bands[0].texts[0].text, "abc") == 0);
	assert(l.bands[0].row == 2);
	assert(l.caret_row == 2);
	assert(l.caret_col == l.bands[0].texts[0].col + 3);

	editor_free(e);
}

static void test_shrink_with_no_bands_is_noop(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_SHRINK_BAND);

	struct layout l = layout(e);
	assert(l.count == 0);
	assert(l.caret_visible == 0);

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

	editor_free(e);
}

static void test_shrink_brings_clipped_band_back(void)
{
	struct editor *e = fresh(COLS, 4);
	add_bands(e, 1);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_ADD_BAND);

	struct layout l = layout(e);
	assert(l.count == 1);
	assert(l.bands[0].row == 3);

	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_SHRINK_BAND);
	l = layout(e);
	assert(l.count == 2);
	assert(l.bands[0].row == 2);
	assert(l.bands[1].row == 4);

	editor_free(e);
}

static void test_move_mode_highlights_selected_and_hides_caret(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_ESCAPE);

	struct layout l = layout(e);
	assert(l.count == 3);
	assert(l.bands[2].style.highlight == 1);
	assert(l.caret_visible == 0);

	editor_free(e);
}

static void test_label_carries_its_own_pad(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_GROW_BAND);
	apply(e, EVENT_SELECT_UP);
	apply(e, EVENT_GROW_BAND);

	struct layout l = layout(e);
	assert(l.count == 3);
	assert(l.bands[0].pad == 1);
	assert(l.bands[1].pad == 2);
	assert(l.bands[2].pad == 0);

	editor_free(e);
}

static void test_only_selected_label_is_highlighted(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_ESCAPE);

	struct layout l = layout(e);
	assert(l.bands[0].style.highlight == 0);
	assert(l.bands[1].style.highlight == 0);
	assert(l.bands[2].style.highlight == 1);

	editor_free(e);
}

static void test_highlight_moves_with_selection(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 3);
	apply(e, EVENT_ESCAPE);
	apply(e, EVENT_SELECT_UP);

	struct layout l = layout(e);
	assert(l.bands[2].style.highlight == 0);
	assert(l.bands[1].style.highlight == 1);

	apply(e, EVENT_SELECT_DOWN);
	l = layout(e);
	assert(l.bands[1].style.highlight == 0);
	assert(l.bands[2].style.highlight == 1);

	editor_free(e);
}

static void test_dim_selected_band_is_highlighted(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_TOGGLE_DIM);
	apply(e, EVENT_ESCAPE);

	struct layout l = layout(e);
	assert(l.bands[0].style.dim == 1);
	assert(l.bands[0].style.highlight == 1);

	editor_free(e);
}

static void test_empty_selected_band_is_highlighted(void)
{
	struct editor *e = fresh(COLS, ROWS);
	add_bands(e, 1);
	apply(e, EVENT_ESCAPE);

	struct layout l = layout(e);
	assert(l.bands[0].texts[0].len == 0);
	assert(l.bands[0].style.highlight == 1);

	editor_free(e);
}

static void test_type_mode_has_no_highlight_and_visible_caret(void)
{
	struct editor *e = fresh(COLS, ROWS);
	apply(e, EVENT_ADD_BAND);
	type(e, "ab");

	struct layout typed = layout(e);
	assert(typed.count == 1);
	assert(typed.bands[0].style.highlight == 0);
	assert(typed.caret_visible == 1);
	assert(typed.caret_row == 1);
	assert(typed.caret_col == typed.bands[0].texts[0].col + 2);

	apply(e, EVENT_ESCAPE);
	struct layout moved = layout(e);
	assert(moved.bands[0].style.highlight == 1);
	assert(moved.caret_visible == 0);

	apply(e, EVENT_ENTER_TYPE);
	struct layout retyped = layout(e);
	assert(retyped.bands[0].style.highlight == 0);
	assert(retyped.caret_visible == 1);
	assert(retyped.caret_row == typed.caret_row);
	assert(retyped.caret_col == typed.caret_col);

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
	test_grow_band_centres_label();
	test_grow_middle_band_pushes_band_below();
	test_grow_band_has_no_limit();
	test_add_band_after_grow_is_one_line();
	test_typing_into_grown_band_keeps_caret_middle();
	test_grow_with_no_bands_is_noop();
	test_grow_does_not_change_mode();
	test_grow_clips_band_below_bottom();
	test_grow_clips_band_with_only_padding_visible();
	test_shrink_band_centres_label_again();
	test_shrink_one_line_band_stays_one_line();
	test_shrink_middle_band_pulls_band_below_up();
	test_grow_then_shrink_restores_layout();
	test_typing_into_shrunk_band_keeps_caret_middle();
	test_shrink_with_no_bands_is_noop();
	test_shrink_does_not_change_mode();
	test_shrink_brings_clipped_band_back();
	test_move_mode_highlights_selected_and_hides_caret();
	test_label_carries_its_own_pad();
	test_only_selected_label_is_highlighted();
	test_highlight_moves_with_selection();
	test_dim_selected_band_is_highlighted();
	test_empty_selected_band_is_highlighted();
	test_type_mode_has_no_highlight_and_visible_caret();
	return 0;
}