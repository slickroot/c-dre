# display-list

## Refactoring Goal

After 031, the model is a single node tree (canvas → bands → texts), but
what `layout()` hands to paint is a **second tree with the same
vocabulary**: `struct layout` → `struct placed_band` → `texts[2]`, each
with its own `count`, `style` and `pad`. Reading `p->texts` or `p->count`
means first working out which tree you are in. Paint also does layout
work: it knows bands, `pad`, and that a highlight covers `2 * pad + 1`
rows. And the two-text cap is hardcoded twice, in `node` logic and in
`layout.h`'s `texts[2]`.

dre is headed for flexbox, with nested boxes. Browsers solve this with a
pipeline (DOM → layout → display list → raster), and Yoga/Clay do the same
in C. This spec follows that model:

1. **One tree.** The node tree is the only tree. `layout()` writes a
   computed `struct rect box` onto every node, as Yoga does. There is no
   separate box tree, because node and box are 1:1 today.
2. **A flat display list.** `display_list()` walks the tree pre-order and
   emits draw ops with final colours, plus the caret, like Clay's render
   commands.
3. **Dumb paint.** `paint_frame()` runs the ops onto the grid. It knows no
   bands, padding, styles, modes or colours.

**Tree order is paint order.** Ops are emitted pre-order (a node's own ops,
then its children's), and later ops draw over earlier ones, so a child
always paints on top of its parent at any depth. No node or op has a
z/depth field.

No visible behaviour change. Depends on **031** (linked children): every
walk is `first_child` → `next`.

## Acceptance Criteria

1. `struct node` has a computed `struct rect box`.
2. `layout(root, cols, rows)` lives in `layout.[ch]` and writes `box` on
   every node. It knows nodes and rects only: no selection, mode or
   colours.
3. `display_list(e, &dl)` lives in `display.[ch]`. It fills a caller-owned,
   fixed-capacity list of `OP_FILL`/`OP_TEXT` ops with final colours, plus
   the caret. The palette lives in `display.c` only.
4. `paint_frame(&dl, g)` takes the display list. `paint.c` has no palette
   and no knowledge of bands or padding.
5. `struct layout`, `struct placed_band`, `struct placed_text`,
   `LAYOUT_MAX_BANDS` and `place_text`'s `struct placed_text` output are
   gone. `editor.c` holds only the event table.
6. **Before** any production code changes, `tests/test_editor.c` is moved
   to screen-level assertions (grid cells and cursor) and passes against
   today's code. After the refactor, it passes with only its frame helper
   changed.
7. All suites pass, `make lint` is green, no leaks under ASan /
   `leaks --atExit`, and visible behaviour is unchanged.

## Technical Design

### Pipeline

```c
editor_apply(e, ev);
layout(editor_root(e), cols, rows);   /* where is everything?    */
display_list(e, &dl);                 /* what does it look like? */
paint_frame(&dl, g);                  /* put it on the grid      */
term_flush(g);
```

`main.c` owns one `static struct display_list dl` for the whole run.

### `rect.h` (new)

```c
struct rect {
	int row, col;     /* top-left, 1-based like the terminal */
	int rows, cols;   /* height, width */
};
```

It is its own header because both `node.h` and `display.h` use it.

### `node.[ch]`

```c
struct style {          /* moved here from layout.h */
	int dim;
};

struct node {
	struct node_data data;
	struct rect box;    /* computed by layout(), not data */
	struct node *parent;
	struct node *first_child, *last_child;
	struct node *prev, *next;
};
```

- `box` sits outside `node_data`: it's a computed result, not user data.
  `node_new` zeroes it.
- `struct style` loses `highlight`, which was never stored. It is derived
  in `display_list` from the selection and mode.
- `node.h` stops including `layout.h`.

### `layout.[ch]`: geometry

```c
void layout(struct node *root, int cols, int rows);
```

It writes boxes using today's rules, expressed as rects. A band's box
**includes its padding**, like CSS's border-box:

- **Root:** `{1, 1, rows, cols}`, the whole screen.
- **Band:** `{top, 1, 2 * pad + 1, cols}`. Bands stack from `top = 1`, and
  the next band's `top` is `top + box.rows`. All bands get a box, including
  ones below the screen; clipping is `display_list`'s job.
- **Text:** `{row, col, 1, len}`, where `mid = top + pad`:
  - one text: `row = mid`, centred: `col = (cols - len) / 2 + 1`;
  - two texts with `pad >= 1` (stacked): rows `mid - 1` and `mid + 1`,
    both centred;
  - two texts with `pad == 0` (side by side): row `mid`, first at
    `col = 3`, second at `col = cols - len - 1`.

  "Two texts" is `band->first_child != band->last_child`. The rules move
  out of `editor.c` as a static helper that returns a `struct rect`.
- If `cols <= 0`, only the root gets a box.

### `display.[ch]`: what it looks like

```c
#define DISPLAY_MAX_OPS 1024

enum op_kind { OP_FILL, OP_TEXT };

struct op {
	enum op_kind kind;
	struct rect rect;    /* TEXT: rows = 1, cols = len */
	const char *text;    /* TEXT only */
	uint32_t colour;     /* FILL: background; TEXT: foreground */
};

struct display_list {
	struct op ops[DISPLAY_MAX_OPS];
	int count;
	int caret_visible;
	int caret_row, caret_col;
};

void display_list(const struct editor *e, struct display_list *out);
```

The palette (`canvas`, `ink`, `dim`, `highlight`) moves from `paint.c` to
`display.c`. `display_list` resets `out` and then walks the tree in order:

1. **Root:** `OP_FILL` of `root->box` in `canvas`. This is always the
   first op. If `root->box.cols <= 0`, stop here.
2. **Each band** (`first_child` → `next`):
   - **Clip:** stop at the first band whose text row
     (`box.row + pad`) is past `root->box.rows`. That is today's
     `row > visible` rule. Rows past the bottom of a band that is drawn
     are clipped by the grid, as today.
   - If the band is selected in `MODE_MOVE`: `OP_FILL` of the band's box
     in `highlight`.
   - **Each text:** `OP_TEXT` at its box, `text = data.text.data`, colour
     `dim` if the band's `style.dim`, else `ink`.
3. **Caret:** if `selected` and `MODE_TYPE`, it goes on the active text
   (`selected->last_child`): `caret_row = box.row`,
   `caret_col = box.col + data.text.cursor`.

Ops past `DISPLAY_MAX_OPS` are dropped. Only on-screen bands emit ops, so
the cap is bounded by the screen, not the document. If nesting ever
outgrows it, it becomes a growable array without changing callers.

### `paint.[ch]`

```c
void paint_frame(const struct display_list *dl, struct grid *g);
```

- `grid_clear(g, 0, 0)` resets characters and the cursor. The root's fill
  paints the canvas. The foreground of blank cells is 0 instead of `ink`,
  which can't be seen because they have no glyph.
- `OP_FILL` → `grid_fill(g, r.row, r.col, r.cols, r.rows, colour)`.
- `OP_TEXT` → `grid_text(g, r.row, r.col, text, r.cols, colour)`.
- Then `grid_cursor` if `caret_visible`.

### `editor.[ch]`

```c
struct node *editor_root(const struct editor *e);
const struct node *editor_selected(const struct editor *e);
enum app_mode editor_mode(const struct editor *e);   /* exists */
```

`layout()` and `place_text` leave `editor.c`. `editor.h` stops including
`layout.h` and only declares `struct node`.

### `layout.h`

It becomes the header for `layout.c`: a forward declaration of
`struct node` and the `layout()` prototype. `struct style` moves to
`node.h`, `struct rect` to `rect.h`, and the `placed_*` structs are
deleted.

### `Makefile`

- `SRCS` gains `layout.c` and `display.c`.
- New `test_layout` (`layout.c node.c text_buffer.c`) and `test_display`
  (`display.c layout.c editor.c node.c text_buffer.c`) in `TEST_BINS` and
  `make test`.
- `test_editor` links the whole pipeline: `editor.c node.c text_buffer.c
  layout.c display.c paint.c grid.c`. `test_paint` links `paint.c grid.c`.

### Components

- `node.[ch]`. Knows the tree links, the node's data, and its computed
  `box`. Does new/free/append/delete.
- `layout.[ch]` (new). Knows the geometry rules. Writes `box` on every
  node. Collaborates with `node` only. This is where flexbox will go.
- `display.[ch]` (new). Knows the palette and how state looks
  (highlight, dim, caret). Turns the laid-out tree into ops. Collaborates
  with `editor` (root, selected, mode) and `node`.
- `paint.[ch]`. Runs ops onto the grid. Collaborates with `grid` only.
- `editor.[ch]`. The event table, plus read-only accessors.
- `grid`, `term`, `input`, `text_buffer`: untouched.

### Testing

**Slice 1 is the safety net and comes first.** Before any production code
changes, rewrite `tests/test_editor.c` so each test drives events and then
renders a frame into a `struct grid` through one helper, `frame(e)`:
today `layout(e)` → `paint_frame`, after the refactor `layout` →
`display_list` → `paint_frame`. Assertions are on what the screen shows:

- characters and foreground (`INK`/`DIM`) on text cells;
- background (`CANVAS`/`HIGHLIGHT`) on any cell, including the full
  `2 * pad + 1` rows of a highlighted band;
- `cursor_visible`, `cursor_row`, `cursor_col`;
- `editor_mode(e)`.

Nothing asserts the foreground of blank cells. Facts the screen doesn't
show directly are made visible: pad is checked through the highlight's
extent in `MODE_MOVE`, and an empty text through the caret position. All
64 tests must pass against today's code. Afterwards, only `frame()`
changes.

New and rewritten suites, in `assert.h` style:

- `test_layout`: root box is the screen. Bands stack, with
  `top += 2 * pad + 1`. Text boxes for one text, two side by side (pad 0)
  and two stacked (pad ≥ 1). Bands below the screen still get boxes.
- `test_display`:
  - root fill is the first op, in `CANVAS`;
  - pre-order: each band's fill comes before its texts;
  - highlight only on the selected band and only in `MODE_MOVE`;
  - `DIM` vs `INK`;
  - caret on the last text in `MODE_TYPE`, none in `MODE_MOVE`;
  - clipping stops at the first band past the screen;
  - `count` never exceeds `DISPLAY_MAX_OPS`.
- `test_paint` is rewritten: ops in, cells out. A fill sets only the
  background, a text sets the character and foreground, a later op draws
  over an earlier one, and the caret sets the cursor.

## Out of scope

- Flexbox properties (direction, justify, align, gap), measuring, and
  nested boxes in the model.
- `z-index` or any paint order other than tree order.
- A growable display list.
- Per-text dim (028), stepping into rows and moving between texts (026,
  027), and removing the two-text cap.
- Moving `pad` into the style, and renaming `node_data`.
