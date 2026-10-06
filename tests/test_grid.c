#include <assert.h>
#include <string.h>

#include "grid.h"

#define COLS 20
#define ROWS 5

#define CANVAS 0x0A0A0Bu
#define INK 0xC9C9CFu
#define DIM 0x6B6B73u
#define HIGHLIGHT 0x1C1C20u

static struct grid *fresh(void)
{
	struct grid *g = grid_new(COLS, ROWS);
	assert(g);
	grid_clear(g, INK, CANVAS);
	return g;
}

static void assert_cell(const struct grid *g, int row, int col, char ch,
			uint32_t fg, uint32_t bg)
{
	const struct cell *c = grid_at(g, row, col);
	assert(c);
	assert(c->ch == ch);
	assert(c->fg == fg);
	assert(c->bg == bg);
}

static void test_new_grid_has_requested_size(void)
{
	struct grid *g = grid_new(COLS, ROWS);
	assert(g);
	assert(g->cols == COLS);
	assert(g->rows == ROWS);

	grid_free(g);
}

static void test_clear_sets_every_cell_and_hides_cursor(void)
{
	struct grid *g = grid_new(COLS, ROWS);
	grid_cursor(g, 3, 5);
	assert(g->cursor_visible == 1);

	grid_clear(g, INK, CANVAS);

	for (int row = 1; row <= ROWS; row++)
		for (int col = 1; col <= COLS; col++)
			assert_cell(g, row, col, ' ', INK, CANVAS);
	assert(g->cursor_visible == 0);

	grid_free(g);
}

static void test_fill_sets_bg_only_inside_rectangle(void)
{
	struct grid *g = fresh();
	grid_text(g, 2, 3, "hello", 5, DIM);
	grid_fill(g, 2, 4, 3, 1, HIGHLIGHT);

	assert_cell(g, 2, 2, ' ', INK, CANVAS);
	assert_cell(g, 2, 3, 'h', DIM, CANVAS);
	assert_cell(g, 2, 4, 'e', DIM, HIGHLIGHT);
	assert_cell(g, 2, 5, 'l', DIM, HIGHLIGHT);
	assert_cell(g, 2, 6, 'l', DIM, HIGHLIGHT);
	assert_cell(g, 2, 7, 'o', DIM, CANVAS);

	grid_fill(g, 3, 1, COLS, 2, HIGHLIGHT);
	for (int col = 1; col <= COLS; col++) {
		assert_cell(g, 3, col, ' ', INK, HIGHLIGHT);
		assert_cell(g, 4, col, ' ', INK, HIGHLIGHT);
	}
	assert_cell(g, 2, 7, 'o', DIM, CANVAS);
	assert_cell(g, 5, 1, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_text_sets_ch_and_fg_and_keeps_bg(void)
{
	struct grid *g = fresh();
	grid_fill(g, 1, 1, COLS, ROWS, HIGHLIGHT);
	grid_text(g, 3, 10, "hi", 2, DIM);

	assert_cell(g, 3, 9, ' ', INK, HIGHLIGHT);
	assert_cell(g, 3, 10, 'h', DIM, HIGHLIGHT);
	assert_cell(g, 3, 11, 'i', DIM, HIGHLIGHT);
	assert_cell(g, 3, 12, ' ', INK, HIGHLIGHT);

	grid_free(g);
}

static void test_text_over_fill_keeps_the_fill_bg(void)
{
	struct grid *g = fresh();
	grid_fill(g, 2, 1, COLS, 1, HIGHLIGHT);
	grid_text(g, 2, 1, "ab", 2, INK);
	grid_text(g, 1, 1, "cd", 2, INK);

	assert_cell(g, 2, 1, 'a', INK, HIGHLIGHT);
	assert_cell(g, 2, 2, 'b', INK, HIGHLIGHT);
	assert_cell(g, 1, 1, 'c', INK, CANVAS);
	assert_cell(g, 1, 2, 'd', INK, CANVAS);

	grid_free(g);
}

static void test_fill_clips_at_every_edge(void)
{
	struct grid *g = fresh();

	grid_fill(g, 0, 0, 3, 3, HIGHLIGHT);
	assert_cell(g, 1, 1, ' ', INK, HIGHLIGHT);
	assert_cell(g, 2, 2, ' ', INK, HIGHLIGHT);
	assert_cell(g, 1, 3, ' ', INK, CANVAS);
	assert_cell(g, 3, 1, ' ', INK, CANVAS);

	grid_clear(g, INK, CANVAS);
	grid_fill(g, ROWS - 1, COLS - 1, 4, 4, HIGHLIGHT);
	assert_cell(g, ROWS - 1, COLS - 1, ' ', INK, HIGHLIGHT);
	assert_cell(g, ROWS, COLS, ' ', INK, HIGHLIGHT);
	assert_cell(g, ROWS - 2, COLS - 1, ' ', INK, CANVAS);
	assert_cell(g, ROWS - 1, COLS - 2, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_text_clips_at_every_edge(void)
{
	struct grid *g = fresh();

	grid_text(g, 2, 0, "abc", 3, DIM);
	assert_cell(g, 2, 1, 'b', DIM, CANVAS);
	assert_cell(g, 2, 2, 'c', DIM, CANVAS);
	assert_cell(g, 2, 3, ' ', INK, CANVAS);

	grid_clear(g, INK, CANVAS);
	grid_text(g, 2, COLS - 1, "xyz", 3, DIM);
	assert_cell(g, 2, COLS - 1, 'x', DIM, CANVAS);
	assert_cell(g, 2, COLS, 'y', DIM, CANVAS);
	assert_cell(g, 2, COLS - 2, ' ', INK, CANVAS);

	grid_clear(g, INK, CANVAS);
	grid_text(g, ROWS, 1, "abc", 3, DIM);
	assert_cell(g, ROWS, 1, 'a', DIM, CANVAS);
	assert_cell(g, ROWS, 3, 'c', DIM, CANVAS);
	assert_cell(g, ROWS - 1, 1, ' ', INK, CANVAS);

	grid_clear(g, INK, CANVAS);
	grid_text(g, 0, 1, "ab", 2, DIM);
	assert_cell(g, 1, 1, ' ', INK, CANVAS);
	assert_cell(g, 1, 2, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_off_screen_calls_are_noops(void)
{
	struct grid *g = fresh();
	grid_text(g, 1, 1, "hi", 2, INK);

	grid_fill(g, ROWS + 1, 1, 3, 3, HIGHLIGHT);
	grid_fill(g, 1, COLS + 1, 3, 3, HIGHLIGHT);
	grid_fill(g, -3, -3, 2, 2, HIGHLIGHT);
	grid_fill(g, ROWS + 10, COLS + 10, 3, 3, HIGHLIGHT);
	grid_text(g, ROWS + 1, 1, "ab", 2, DIM);
	grid_text(g, 1, COLS + 1, "ab", 2, DIM);
	grid_text(g, ROWS + 10, COLS + 10, "ab", 2, DIM);

	assert_cell(g, 1, 1, 'h', INK, CANVAS);
	assert_cell(g, 1, 2, 'i', INK, CANVAS);
	assert_cell(g, ROWS, COLS, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_grid_at_outside_the_grid_is_null(void)
{
	struct grid *g = fresh();

	assert(grid_at(g, 1, 1) != NULL);
	assert(grid_at(g, ROWS, COLS) != NULL);
	assert(grid_at(g, 0, 1) == NULL);
	assert(grid_at(g, 1, 0) == NULL);
	assert(grid_at(g, ROWS + 1, 1) == NULL);
	assert(grid_at(g, 1, COLS + 1) == NULL);
	assert(grid_at(g, 0, 0) == NULL);
	assert(grid_at(g, -1, -1) == NULL);

	grid_free(g);
}

static void test_cursor_is_visible_at_the_cell_and_clear_hides_it(void)
{
	struct grid *g = grid_new(COLS, ROWS);
	assert(g->cursor_visible == 0);

	grid_cursor(g, 3, 12);
	assert(g->cursor_visible == 1);
	assert(g->cursor_row == 3);
	assert(g->cursor_col == 12);

	grid_cursor(g, 5, 1);
	assert(g->cursor_visible == 1);
	assert(g->cursor_row == 5);
	assert(g->cursor_col == 1);

	grid_clear(g, INK, CANVAS);
	assert(g->cursor_visible == 0);

	grid_free(g);
}

int main(void)
{
	test_new_grid_has_requested_size();
	test_clear_sets_every_cell_and_hides_cursor();
	test_fill_sets_bg_only_inside_rectangle();
	test_text_sets_ch_and_fg_and_keeps_bg();
	test_text_over_fill_keeps_the_fill_bg();
	test_fill_clips_at_every_edge();
	test_text_clips_at_every_edge();
	test_off_screen_calls_are_noops();
	test_grid_at_outside_the_grid_is_null();
	test_cursor_is_visible_at_the_cell_and_clear_hides_it();
	return 0;
}