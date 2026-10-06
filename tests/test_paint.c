#include <assert.h>
#include <string.h>

#include "grid.h"
#include "paint.h"

#define COLS 20
#define ROWS 5

#define CANVAS 0x0A0A0Bu
#define INK 0xC9C9CFu
#define DIM 0x6B6B73u
#define HIGHLIGHT 0x1C1C20u

static struct rect rect(int row, int col, int rows, int cols)
{
	struct rect r = {row, col, rows, cols};

	return r;
}

static void push_fill(struct display_list *dl, struct rect r, uint32_t colour)
{
	struct op *op = &dl->ops[dl->count++];

	op->kind = OP_FILL;
	op->rect = r;
	op->text = NULL;
	op->colour = colour;
}

static void push_text(struct display_list *dl, int row, int col,
		      const char *text, uint32_t colour)
{
	struct op *op = &dl->ops[dl->count++];

	op->kind = OP_TEXT;
	op->rect = rect(row, col, 1, (int)strlen(text));
	op->text = text;
	op->colour = colour;
}

static struct display_list empty(void)
{
	struct display_list dl;

	memset(&dl, 0, sizeof dl);
	return dl;
}

static struct grid *paint(const struct display_list *dl)
{
	struct grid *g = grid_new(COLS, ROWS);

	assert(g);
	paint_frame(dl, g);
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

static void assert_blank(const struct grid *g, int row, int col, uint32_t bg)
{
	const struct cell *c = grid_at(g, row, col);

	assert(c);
	assert(c->ch == ' ');
	assert(c->bg == bg);
}

static void test_empty_display_list_clears_the_grid(void)
{
	struct display_list dl = empty();
	struct grid *g = paint(&dl);

	for (int row = 1; row <= ROWS; row++) {
		for (int col = 1; col <= COLS; col++) {
			assert_blank(g, row, col, 0);
			assert(grid_at(g, row, col)->fg == 0);
		}
	}
	assert(g->cursor_visible == 0);
	assert(g->cursor_row == 0);
	assert(g->cursor_col == 0);

	grid_free(g);
}

static void test_root_fill_paints_blank_canvas(void)
{
	struct display_list dl = empty();

	push_fill(&dl, rect(1, 1, ROWS, COLS), CANVAS);
	struct grid *g = paint(&dl);

	for (int row = 1; row <= ROWS; row++)
		for (int col = 1; col <= COLS; col++)
			assert_blank(g, row, col, CANVAS);

	assert(g->cursor_visible == 0);

	grid_free(g);
}

static void test_fill_sets_only_the_background(void)
{
	struct display_list dl = empty();

	push_fill(&dl, rect(2, 1, 1, COLS), HIGHLIGHT);
	struct grid *g = paint(&dl);

	/* inside the fill: bg set, no glyph */
	assert_blank(g, 2, 1, HIGHLIGHT);
	assert_blank(g, 2, 10, HIGHLIGHT);
	assert_blank(g, 2, COLS, HIGHLIGHT);
	assert(grid_at(g, 2, 10)->fg == 0);

	/* outside the fill: untouched */
	assert_blank(g, 1, 1, 0);
	assert_blank(g, 3, 1, 0);
	assert_blank(g, ROWS, COLS, 0);

	grid_free(g);
}

static void test_fill_does_not_disturb_neighbour_rows(void)
{
	struct display_list dl = empty();

	push_fill(&dl, rect(2, 1, 1, COLS), HIGHLIGHT);
	push_fill(&dl, rect(4, 1, 1, COLS), HIGHLIGHT);
	struct grid *g = paint(&dl);

	assert_blank(g, 2, 5, HIGHLIGHT);
	assert_blank(g, 3, 5, 0);
	assert_blank(g, 4, 5, HIGHLIGHT);
	assert_blank(g, 5, 5, 0);

	grid_free(g);
}

static void test_text_sets_character_and_foreground_only(void)
{
	struct display_list dl = empty();

	push_text(&dl, 2, 4, "hi", INK);
	struct grid *g = paint(&dl);

	assert_cell(g, 2, 4, 'h', INK, 0);
	assert_cell(g, 2, 5, 'i', INK, 0);

	/* neighbours keep the cleared background and no glyph */
	assert_blank(g, 2, 3, 0);
	assert_blank(g, 2, 6, 0);
	assert_blank(g, 1, 4, 0);
	assert_blank(g, 3, 4, 0);

	grid_free(g);
}

static void test_text_foreground_is_the_op_colour(void)
{
	struct display_list dl = empty();

	push_text(&dl, 1, 1, "d", DIM);
	struct grid *g = paint(&dl);

	assert_cell(g, 1, 1, 'd', DIM, 0);
	assert_blank(g, 1, 2, 0);

	grid_free(g);
}

static void test_text_after_fill_keeps_the_fill_background(void)
{
	struct display_list dl = empty();

	push_fill(&dl, rect(2, 1, 1, COLS), HIGHLIGHT);
	push_text(&dl, 2, 4, "hi", INK);
	struct grid *g = paint(&dl);

	assert_cell(g, 2, 4, 'h', INK, HIGHLIGHT);
	assert_cell(g, 2, 5, 'i', INK, HIGHLIGHT);
	assert_blank(g, 2, 1, HIGHLIGHT);
	assert_blank(g, 3, 4, 0);

	grid_free(g);
}

static void test_fill_after_text_overwrites_only_the_background(void)
{
	struct display_list dl = empty();

	push_text(&dl, 2, 4, "hi", INK);
	push_fill(&dl, rect(2, 1, 1, COLS), HIGHLIGHT);
	struct grid *g = paint(&dl);

	/* grid_fill only writes bg, so the glyph and its fg survive */
	assert_cell(g, 2, 4, 'h', INK, HIGHLIGHT);
	assert_cell(g, 2, 5, 'i', INK, HIGHLIGHT);
	assert_blank(g, 2, 6, HIGHLIGHT);

	grid_free(g);
}

static void test_later_text_overwrites_an_earlier_one(void)
{
	struct display_list dl = empty();

	push_text(&dl, 2, 4, "ab", INK);
	push_text(&dl, 2, 4, "cd", DIM);
	struct grid *g = paint(&dl);

	assert_cell(g, 2, 4, 'c', DIM, 0);
	assert_cell(g, 2, 5, 'd', DIM, 0);

	grid_free(g);
}

static void test_two_texts_side_by_side(void)
{
	struct display_list dl = empty();

	push_text(&dl, 2, 2, "ab", INK);
	push_text(&dl, 2, 8, "cd", DIM);
	struct grid *g = paint(&dl);

	assert_cell(g, 2, 2, 'a', INK, 0);
	assert_cell(g, 2, 3, 'b', INK, 0);
	assert_cell(g, 2, 8, 'c', DIM, 0);
	assert_cell(g, 2, 9, 'd', DIM, 0);
	assert_blank(g, 2, 4, 0);
	assert_blank(g, 2, 10, 0);

	grid_free(g);
}

static void test_caret_visible_places_the_cursor(void)
{
	struct display_list dl = empty();

	push_text(&dl, 2, 5, "ab", INK);
	dl.caret_visible = 1;
	dl.caret_row = 2;
	dl.caret_col = 6;

	struct grid *g = paint(&dl);

	assert(g->cursor_visible == 1);
	assert(g->cursor_row == 2);
	assert(g->cursor_col == 6);
	assert_cell(g, 2, 5, 'a', INK, 0);
	assert_cell(g, 2, 6, 'b', INK, 0);

	grid_free(g);
}

static void test_caret_hidden_when_not_visible(void)
{
	struct display_list dl = empty();

	push_text(&dl, 3, 2, "x", INK);
	dl.caret_visible = 0;
	dl.caret_row = 3;
	dl.caret_col = 7;

	struct grid *g = paint(&dl);

	assert(g->cursor_visible == 0);
	assert(g->cursor_row == 0);
	assert(g->cursor_col == 0);
	assert_cell(g, 3, 2, 'x', INK, 0);

	grid_free(g);
}

static void test_paint_clears_a_cursor_left_by_an_earlier_frame(void)
{
	struct display_list dl = empty();

	push_text(&dl, 1, 1, "a", INK);
	dl.caret_visible = 1;
	dl.caret_row = 1;
	dl.caret_col = 2;

	struct grid *g = paint(&dl);
	assert(g->cursor_visible == 1);

	struct display_list next = empty();

	struct grid *g2 = paint(&next);

	assert(g2->cursor_visible == 0);
	assert(g2->cursor_row == 0);
	assert(g2->cursor_col == 0);

	/* the first grid is untouched by the second frame */
	assert(g->cursor_visible == 1);
	assert_cell(g, 1, 1, 'a', INK, 0);

	grid_free(g);
	grid_free(g2);
}

static void test_fill_past_the_edges_is_clipped_by_the_grid(void)
{
	struct display_list dl = empty();

	push_fill(&dl, rect(ROWS - 1, COLS - 2, 4, 5), HIGHLIGHT);
	struct grid *g = paint(&dl);

	assert_blank(g, ROWS - 1, COLS - 2, HIGHLIGHT);
	assert_blank(g, ROWS, COLS, HIGHLIGHT);
	assert_blank(g, ROWS - 2, 1, 0);
	assert_blank(g, 1, 1, 0);

	grid_free(g);
}

static void test_text_past_the_edges_is_clipped_by_the_grid(void)
{
	struct display_list dl = empty();

	push_text(&dl, ROWS, COLS - 1, "xyz", INK);
	struct grid *g = paint(&dl);

	assert_cell(g, ROWS, COLS - 1, 'x', INK, 0);
	assert_cell(g, ROWS, COLS, 'y', INK, 0);
	assert_blank(g, ROWS - 1, COLS - 1, 0);
	assert_blank(g, ROWS, COLS - 2, 0);

	grid_free(g);
}

int main(void)
{
	test_empty_display_list_clears_the_grid();
	test_root_fill_paints_blank_canvas();
	test_fill_sets_only_the_background();
	test_fill_does_not_disturb_neighbour_rows();
	test_text_sets_character_and_foreground_only();
	test_text_foreground_is_the_op_colour();
	test_text_after_fill_keeps_the_fill_background();
	test_fill_after_text_overwrites_only_the_background();
	test_later_text_overwrites_an_earlier_one();
	test_two_texts_side_by_side();
	test_caret_visible_places_the_cursor();
	test_caret_hidden_when_not_visible();
	test_paint_clears_a_cursor_left_by_an_earlier_frame();
	test_fill_past_the_edges_is_clipped_by_the_grid();
	test_text_past_the_edges_is_clipped_by_the_grid();
	return 0;
}