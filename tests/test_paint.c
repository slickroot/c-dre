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

static struct placed_text placed(int row, int col, const char *text)
{
	struct placed_text t;

	t.row = row;
	t.col = col;
	t.text = text;
	t.len = (int)strlen(text);
	return t;
}

static void add_band(struct layout *l, int row, int pad, struct style style,
		     int count, struct placed_text texts[2])
{
	struct placed_band *b = &l->bands[l->count++];

	b->row = row;
	b->pad = pad;
	b->style = style;
	b->count = count;
	for (int t = 0; t < count; t++)
		b->texts[t] = texts[t];
}

static struct grid *paint(const struct layout *l)
{
	struct grid *g = grid_new(COLS, ROWS);
	assert(g);
	paint_frame(l, g);
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

static void assert_canvas(const struct grid *g)
{
	for (int row = 1; row <= ROWS; row++)
		for (int col = 1; col <= COLS; col++)
			assert_cell(g, row, col, ' ', INK, CANVAS);
}

static void test_empty_layout_is_blank_canvas(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct grid *g = paint(&l);

	assert_canvas(g);
	assert(g->cursor_visible == 0);

	grid_free(g);
}

static void test_one_plain_band(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[1] = {placed(2, 4, "hi")};
	add_band(&l, 2, 0, (struct style){0, 0}, 1, texts);

	struct grid *g = paint(&l);

	assert_cell(g, 2, 4, 'h', INK, CANVAS);
	assert_cell(g, 2, 5, 'i', INK, CANVAS);
	assert_cell(g, 2, 3, ' ', INK, CANVAS);
	assert_cell(g, 2, 6, ' ', INK, CANVAS);
	assert_cell(g, 1, 1, ' ', INK, CANVAS);
	assert_cell(g, ROWS, COLS, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_highlighted_band_pad_zero(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[1] = {placed(3, 6, "ok")};
	add_band(&l, 3, 0, (struct style){0, 1}, 1, texts);

	struct grid *g = paint(&l);

	for (int col = 1; col <= COLS; col++)
		assert(grid_at(g, 3, col)->bg == HIGHLIGHT);

	assert_cell(g, 3, 6, 'o', INK, HIGHLIGHT);
	assert_cell(g, 3, 7, 'k', INK, HIGHLIGHT);
	assert_cell(g, 3, 5, ' ', INK, HIGHLIGHT);
	assert_cell(g, 3, 8, ' ', INK, HIGHLIGHT);

	assert_cell(g, 2, 1, ' ', INK, CANVAS);
	assert_cell(g, 4, 1, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_highlighted_band_pad_one(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[1] = {placed(3, 2, "x")};
	add_band(&l, 3, 1, (struct style){0, 1}, 1, texts);

	struct grid *g = paint(&l);

	for (int row = 2; row <= 4; row++)
		for (int col = 1; col <= COLS; col++)
			assert(grid_at(g, row, col)->bg == HIGHLIGHT);

	assert_cell(g, 3, 2, 'x', INK, HIGHLIGHT);
	assert_cell(g, 1, 1, ' ', INK, CANVAS);
	assert_cell(g, 5, 1, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_dim_band(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[1] = {placed(1, 1, "d")};
	add_band(&l, 1, 0, (struct style){1, 0}, 1, texts);

	struct grid *g = paint(&l);

	assert_cell(g, 1, 1, 'd', DIM, CANVAS);
	assert_cell(g, 1, 2, ' ', INK, CANVAS);
	assert_cell(g, 2, 1, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_dim_highlighted_band(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[1] = {placed(2, 3, "dg")};
	add_band(&l, 2, 0, (struct style){1, 1}, 1, texts);

	struct grid *g = paint(&l);

	assert_cell(g, 2, 3, 'd', DIM, HIGHLIGHT);
	assert_cell(g, 2, 4, 'g', DIM, HIGHLIGHT);
	assert_cell(g, 2, 5, ' ', INK, HIGHLIGHT);
	assert_cell(g, 1, 1, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_two_texts_side_by_side(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[2] = {placed(2, 2, "ab"), placed(2, 8, "cd")};
	add_band(&l, 2, 0, (struct style){0, 0}, 2, texts);

	struct grid *g = paint(&l);

	assert_cell(g, 2, 2, 'a', INK, CANVAS);
	assert_cell(g, 2, 3, 'b', INK, CANVAS);
	assert_cell(g, 2, 8, 'c', INK, CANVAS);
	assert_cell(g, 2, 9, 'd', INK, CANVAS);
	assert_cell(g, 2, 4, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_two_texts_stacked_on_a_tall_row(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[2] = {placed(2, 1, "ab"), placed(4, 4, "cd")};
	add_band(&l, 3, 1, (struct style){0, 0}, 2, texts);

	struct grid *g = paint(&l);

	assert_cell(g, 2, 1, 'a', INK, CANVAS);
	assert_cell(g, 2, 2, 'b', INK, CANVAS);
	assert_cell(g, 4, 4, 'c', INK, CANVAS);
	assert_cell(g, 4, 5, 'd', INK, CANVAS);
	assert_cell(g, 3, 1, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_pad_past_the_bottom_is_clipped(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[1] = {placed(5, 1, "z")};
	add_band(&l, ROWS, 3, (struct style){0, 1}, 1, texts);

	struct grid *g = paint(&l);

	for (int row = 2; row <= ROWS; row++)
		for (int col = 1; col <= COLS; col++)
			assert(grid_at(g, row, col)->bg == HIGHLIGHT);

	assert_cell(g, ROWS, 1, 'z', INK, HIGHLIGHT);
	assert_cell(g, ROWS, 2, ' ', INK, HIGHLIGHT);
	assert_cell(g, 1, 1, ' ', INK, CANVAS);

	grid_free(g);
}

static void test_caret_visible_places_the_cursor(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[1] = {placed(2, 5, "ab")};
	add_band(&l, 2, 0, (struct style){0, 0}, 1, texts);
	l.caret_visible = 1;
	l.caret_row = 2;
	l.caret_col = 6;

	struct grid *g = paint(&l);

	assert(g->cursor_visible == 1);
	assert(g->cursor_row == 2);
	assert(g->cursor_col == 6);
	assert_cell(g, 2, 5, 'a', INK, CANVAS);
	assert_cell(g, 2, 6, 'b', INK, CANVAS);

	grid_free(g);
}

static void test_caret_hidden_when_not_visible(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	l.caret_visible = 0;
	l.caret_row = 3;
	l.caret_col = 7;

	struct grid *g = paint(&l);

	assert(g->cursor_visible == 0);

	grid_free(g);
}

static void test_two_bands_layer_highlight_under_text(void)
{
	struct layout l;

	memset(&l, 0, sizeof l);
	struct placed_text texts[1] = {placed(3, 2, "hi")};
	add_band(&l, 3, 0, (struct style){0, 1}, 1, texts);
	struct placed_text texts2[1] = {placed(4, 6, "yo")};
	add_band(&l, 4, 0, (struct style){0, 0}, 1, texts2);

	struct grid *g = paint(&l);

	assert_cell(g, 3, 2, 'h', INK, HIGHLIGHT);
	assert_cell(g, 3, 3, 'i', INK, HIGHLIGHT);
	assert_cell(g, 4, 6, 'y', INK, CANVAS);
	assert_cell(g, 4, 7, 'o', INK, CANVAS);
	assert_cell(g, 4, 1, ' ', INK, CANVAS);
	assert_cell(g, 2, 1, ' ', INK, CANVAS);

	grid_free(g);
}

int main(void)
{
	test_empty_layout_is_blank_canvas();
	test_one_plain_band();
	test_highlighted_band_pad_zero();
	test_highlighted_band_pad_one();
	test_dim_band();
	test_dim_highlighted_band();
	test_two_texts_side_by_side();
	test_two_texts_stacked_on_a_tall_row();
	test_pad_past_the_bottom_is_clipped();
	test_caret_visible_places_the_cursor();
	test_caret_hidden_when_not_visible();
	test_two_bands_layer_highlight_under_text();
	return 0;
}