# grid-layer

## Refactoring Goal

`paint_frame()` writes escape codes straight to the terminal, one small
`write()` at a time, and decides on its own what the screen looks like:
it turns `dim`/`highlight` flags into RGB, works out a band's height from
`pad`, fills full width with `ESC[K`, and relies on `ESC[2J` running while
the canvas colour is active. None of that is tested. Only positions are,
through `layout()`.

Add a grid: `cols × rows` cells of `{ ch, fg, bg }` plus a cursor, which
is exactly what the terminal shows. `paint_frame()` becomes a pure function
that draws a `struct layout` into a grid, and a new `term_flush()` is the
only code that turns a grid into escape codes. Tests pin today's screen
cell by cell. That gives us a safety net for the refactors that follow:
nodes instead of bands (canvas → bands → texts), and possibly `layout()`
returning boxes. It comes before stories 026–028.

No visible behaviour change.

## Acceptance Criteria

1. A new `grid.[ch]` module holds the cells and the cursor. It does no I/O.
2. `paint_frame(const struct layout *, struct grid *)` is pure: no I/O, no
   allocation. It owns the palette.
3. A new `term.[ch]` module owns every escape code. `term_flush()` writes a
   whole grid to the terminal.
4. `main.c` creates one grid at startup and runs
   read → parse → apply → layout → paint → flush.
5. `ESC[2J` is no longer used per frame, and the "keep canvas bg active"
   workaround in `paint.c` is gone.
6. `tests/test_grid.c` and `tests/test_paint.c` exist and run in
   `make test`. `tests/test_editor.c` is untouched and stays green.
7. Visible behaviour is unchanged: same colours, same positions, same
   cursor, no flicker.

## Technical Design

```
stdin byte → input_parse() → editor_apply() → layout() → paint_frame() → term_flush()
                                                │             │              │
                                          struct layout   struct grid    escape codes
                                            (tested)       (tested)      (hand-judged)
```

### `grid.[ch]`: what the terminal shows

```c
struct cell {
	char ch;       /* ' ' when empty */
	uint32_t fg;   /* 0xRRGGBB */
	uint32_t bg;   /* 0xRRGGBB */
};

struct grid {
	int cols, rows;
	struct cell *cells;   /* rows * cols, row-major */
	int cursor_visible;
	int cursor_row, cursor_col;
};

struct grid *grid_new(int cols, int rows);
void grid_free(struct grid *g);
void grid_clear(struct grid *g, uint32_t fg, uint32_t bg);
void grid_fill(struct grid *g, int row, int col, int w, int h, uint32_t bg);
void grid_text(struct grid *g, int row, int col, const char *s, int len,
	       uint32_t fg);
void grid_cursor(struct grid *g, int row, int col);
const struct cell *grid_at(const struct grid *g, int row, int col);
```

- **Coordinates are 1-based**, the same as `layout()` and `ESC[row;colH`.
  `grid_at(g, 1, 1)` is the top-left cell. The only `- 1` is inside
  `grid.c`'s index calculation: `cells[(row - 1) * cols + (col - 1)]`.
- **Allocated once, at the terminal's real size.** `grid_new` mallocs the
  struct and `cols * rows` cells, and returns `NULL` on failure. `grid_free`
  releases both. The grid isn't a fixed-max array, because a fixed-max grid
  is too big for the stack and adds a cap for no benefit.
- **Colours are RGB, not flags.** The grid knows nothing about dim,
  highlight, bands or texts.
- `grid_clear` sets every cell to `{ ' ', fg, bg }` and hides the cursor.
- `grid_fill` sets **only `bg`** in the rectangle. `ch` and `fg` are kept.
- `grid_text` sets **only `ch` and `fg`**. `bg` is kept, so a text drawn on
  a highlighted band takes the highlight and a text on the canvas takes
  the canvas. That's where layering comes from.
- `grid_cursor` sets `cursor_row`/`cursor_col` and makes the cursor visible.
- **Everything clips.** Cells outside `1..rows × 1..cols` are skipped, and
  a rectangle or text that runs off any edge keeps only its on-screen
  part. `grid_at` outside the grid returns `NULL`.

### `paint.[ch]`: a layout drawn into a grid

```c
void paint_frame(const struct layout *l, struct grid *g);
```

Palette, as constants private to `paint.c`:

| name | colour | was |
|---|---|---|
| canvas | `0x0A0A0B` | `bg_for` with no highlight, wallpaper |
| ink | `0xC9C9CF` | `fg_for` normal |
| dim | `0x6B6B73` | `fg_for` dim |
| highlight | `0x1C1C20` | `bg_for` with highlight |

Steps:

1. `grid_clear(g, ink, canvas)`.
2. For each band with `style.highlight`:
   `grid_fill(g, row - pad, 1, g->cols, 2 * pad + 1, highlight)`. This
   replaces the per-line `ESC[K`.
3. For each band, for each text:
   `grid_text(g, t.row, t.col, t.text, t.len, style.dim ? dim : ink)`.
4. If `caret_visible`: `grid_cursor(g, caret_row, caret_col)`.

`fg_for`, `bg_for` and every `write()` leave `paint.c`, and
`paint_wallpaper` moves to `term.c`. `paint.h` no longer needs `unistd.h`.

### `term.[ch]`: a grid to the terminal

```c
void term_enter(void);                      /* was paint_wallpaper */
void term_flush(const struct grid *g);
void term_leave(void);                      /* escape half of restore() */
```

- `term_enter`: alternate screen, hide cursor, canvas bg, `ESC[2J`, home,
  the same as `paint_wallpaper` today. `ESC[2J` stays here **once at
  startup**, so the screen isn't garbage before the first key.
- `term_flush` is a **full redraw**, with no diffing:
  1. `ESC[?2026h` (synchronized update on).
  2. For each row: `ESC[row;1H`, then each cell. It emits
     `ESC[38;2;r;g;bm` / `ESC[48;2;r;g;bm` only when `fg`/`bg` differs from
     the previous emitted cell, then `ch`.
  3. The cursor: `ESC[r;cH ESC[?25h` if visible, otherwise `ESC[?25l`.
  4. `ESC[?2026l`.

  Output goes through a fixed 64 KB static buffer in `term.c` that is
  `write()`n when full and once at the end. That's usually one or a few
  `write`s per frame instead of ~100, with no allocation.
- Writing the bottom-right cell leaves the terminal in "pending wrap"
  without scrolling (deferred wrap, standard in xterm-compatible
  terminals). The next escape is a cursor move, which clears it.
- `term_leave`: `ESC[0m`, leave alternate screen. These are the escape writes
  currently in `main.c`'s `restore()`. The tty attributes stay in `main.c`.

### `main.c`

```c
term_enter();                       /* inside enter(), after raw mode */
struct grid *g = grid_new(cols, rows);
if (!g)
	exit(1);
...
while (read(STDIN_FILENO, &byte, 1) == 1) {
	struct key_event ev = input_parse(byte, editor_mode(e));
	if (ev.type == EVENT_QUIT)
		break;
	editor_apply(e, ev);
	struct layout l = layout(e);
	paint_frame(&l, g);
	term_flush(g);
}
grid_free(g);
editor_free(e);
```

`restore()` calls `term_leave()` and then restores the tty.

### `Makefile`

- `SRCS` gains `grid.c` and `term.c`.
- `$(BUILD_DIR)/test_grid` builds from `tests/test_grid.c grid.c`.
- `$(BUILD_DIR)/test_paint` builds from `tests/test_paint.c paint.c grid.c`.
- Both are added to `TEST_BINS` and run by `make test`. New files follow
  kernel style, and `make lint` stays green.

### Components

- `grid.[ch]` (new). Knows the cells, size and cursor. Does
  new/free/clear/fill/text/cursor/at, clipped. No collaborators.
- `paint.[ch]`. Knows the palette and how a `struct layout` looks.
  Does `paint_frame`. Collaborates with `layout.h` and `grid`.
- `term.[ch]` (new). Knows escape codes. Does `term_enter`,
  `term_flush`, `term_leave`. Collaborates with `grid` (read only).
- `main.c`. Knows the tty and the loop. Wires
  editor → layout → paint → term.
- `editor`, `layout.h`, `input`, `text_buffer`: untouched.

### Testing

`assert.h` style, like the other suites. Tests use small grids
(e.g. 20×5). `test_paint` builds `struct layout` values by hand. It's a
plain struct, so no editor is needed.

`tests/test_grid.c`:
- A new grid has the requested `cols`/`rows`.
- After clear, every cell is `{ ' ', fg, bg }` and the cursor is hidden.
- Fill sets bg in exactly the rectangle and leaves `ch`/`fg` alone.
- Text sets `ch`/`fg` and leaves `bg` alone. Text over a fill keeps the
  fill's bg.
- Clipping on each edge: fill and text that start before row/col 1 or run
  past `rows`/`cols` write only their on-screen cells and touch nothing
  else. A fully off-screen call is a no-op.
- `grid_at` outside the grid returns `NULL`.
- `grid_cursor` makes it visible at the given cell, and clear hides it again.

`tests/test_paint.c` pins today's screen:
- Empty layout: every cell is canvas bg with `' '`, and the cursor is hidden.
- One plain band "hi": `h`/`i` at their placed cells with ink fg and canvas
  bg. Every other cell is untouched canvas.
- Highlighted band, pad 0: every cell of that row (cols 1..cols) has
  highlight bg, the text cells keep ink fg, and other rows are canvas.
- Highlighted band, pad 1: rows `row - 1 .. row + 1` are highlight, full
  width, and the rows around them are canvas.
- Dim band: text fg is `0x6B6B73`. Dim and highlighted: fg dim, bg
  highlight.
- Two texts (side by side, and stacked on a tall row): both drawn at their
  placed cells.
- A band whose pad runs past the bottom: clipped, no crash.
- `caret_visible`: the cursor is visible at `caret_row`/`caret_col`.
  Otherwise it's hidden.

`term.c` and `main.c` stay hand-judged: add rows, type, dim, grow and
shrink, move with `j`/`k`, delete, and quit. The screen should match
today's (canvas, highlight, dim, caret) with no flicker, and quitting
should restore the terminal. No leaks under ASan / `leaks --atExit` for
`test_grid` and `test_paint`.

## Out of scope

- Diffing (only emitting changed cells).
- The node tree (canvas → bands → texts, no `kind`). That's the next
  refactoring spec.
- `layout()` returning boxes instead of `struct layout`.
- Terminal resize (`SIGWINCH`), and UTF-8 / wide characters (`ch` stays a
  `char`).
