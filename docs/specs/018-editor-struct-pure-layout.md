# editor-struct-pure-layout

## Refactoring Goal

All editor state lives in file-scope statics in `main.c` (`bands`,
`selected`, `cols`) plus a local `mode` in `main()`, and every event paints
incrementally as it goes. None of it can run without a terminal, so none of
it is tested: inserting, deleting, selection, mode changes and where things
land on screen are all hand-judged.

Pull the state into one opaque `struct editor`, move the event `switch` into
`editor_apply()`, and make "what's on screen" a pure function,
`layout(const struct editor *)`, that returns a value tests can assert on.
Paint becomes a dumb renderer of that value, redrawing the full frame every
event. The main loop becomes: read → parse → apply → layout → paint.

## Acceptance Criteria

1. `struct editor` is opaque: its fields are only visible inside `editor.c`.
   `struct band` moves into `editor.c` too.
2. `layout(const struct editor *)` is pure: one argument in, one
   `struct layout` out by value, no malloc, no I/O.
3. `struct band` no longer stores `row`; rows are derived from list position
   by `layout()`.
4. The event `switch` (minus quit) lives in `editor_apply()`; `main()` no
   longer touches bands, selection or mode directly.
5. Every event repaints the full frame with `paint_frame()`, wrapped in
   synchronized output (mode 2026).
6. `editor` is covered by black-box unit tests in `tests/test_editor.c`
   that only use the public surface.
7. Visible behavior is unchanged, except for two deliberate fixes: bands
   below the bottom of the terminal are clipped instead of overdrawing the
   last row, and a label can no longer grow to `cols` characters (see
   below).

## Technical Design

Still `main.c`, `input`, `text_buffer`, `paint`, plus one new module,
`editor.[ch]`, and one new header, `layout.h`. `input.[ch]` and
`text_buffer.[ch]` are untouched.

```
stdin byte → input_parse() → key_event → editor_apply() → layout() → paint_frame()
                 ▲                                            │
                 └──────────── editor_mode() ◀────────────────┘ (main)
```

### `layout.h` — the frame as a value

```c
#define LAYOUT_MAX_LABELS 256

struct placed_label {
  int row;            /* 1-based screen row */
  int col;            /* 1-based screen col, already centered */
  const char *text;   /* points into the band's text_buffer */
  int len;
};

struct layout {
  struct placed_label labels[LAYOUT_MAX_LABELS];
  int count;          /* labels[0..count) are valid, sorted by row */
  int caret_visible;
  int caret_row;
  int caret_col;
};
```

- Fixed size (~6 KB) so `layout()` can return it by value with no heap
  allocation. The cap is on **visible screen rows**, not on bands: the band
  list stays unbounded; only what fits on screen is placed. 256 rows is a
  bet that no terminal is taller.
- `text` borrows from the band's buffer. A layout is valid only until the
  next `editor_apply()`; it lives for exactly one paint.
- Only `labels[0..count)` are written; the rest is untouched stack memory.
- Lives in its own header so `paint.h` depends on the frame description,
  not on the editor.

### `editor.h` — opaque editor, public surface

```c
#include "input.h"
#include "layout.h"

struct editor;

struct editor *editor_new(int cols, int rows);
void editor_free(struct editor *e);
void editor_apply(struct editor *e, struct key_event ev);
enum app_mode editor_mode(const struct editor *e);
struct layout layout(const struct editor *e);
```

That is the entire public surface. Because the struct is only declared,
`main.c` and the tests get a compile error if they touch its fields. Since
outside code doesn't know its size, it can't sit on the stack:
`editor_new()` mallocs it (returns `NULL` on failure), `editor_free()`
frees every band and the editor. One allocation for the life of the
program.

### `editor.c` — state and transitions

```c
struct band {
  struct text_buffer buf;
  struct band *next;    /* older band, one row up */
  struct band *prev;    /* newer band, one row down */
};

struct editor {
  struct band *bands;   /* head = newest = bottom row */
  struct band *selected;
  enum app_mode mode;
  int cols;
  int rows;
};
```

`row` is gone from `struct band`. List order already encodes it: the tail
(oldest) is row 1, each newer band is one row lower, the head is the bottom
row.

Static helpers, carried over from spec 016 but with painting and row
bookkeeping removed:

- `insert_node(struct editor *e)` — mallocs a band,
  `text_buffer_init(&b->buf, e->cols - 1)`, links it at the head, sets
  `e->selected = b`. The capacity is `cols - 1`, not `cols`: today a buffer
  accepts a `cols`-th character that `paint_label()` then refuses to draw
  (`len >= cols` early return). Capping the model at what the view can show
  removes that state; `text_buffer_insert()` already rejects inserts past
  `cap`.
- `delete_node(struct editor *e, struct band *node)` — unlinks, picks the
  heir (`next`, else `prev`), frees, reseats `e->selected = heir`. The
  row-decrement loop is deleted — nothing stores rows anymore.

`editor_apply()` owns the switch formerly in `main()`:

| event | effect |
|---|---|
| `EVENT_ADD_BAND` | `insert_node(e)`; `mode = MODE_TYPE` |
| `EVENT_ESCAPE` | `mode = MODE_MOVE` |
| `EVENT_ENTER_TYPE` | `mode = MODE_TYPE` |
| `EVENT_SELECT_UP` | `selected = selected->next` if both exist |
| `EVENT_SELECT_DOWN` | `selected = selected->prev` if both exist |
| `EVENT_CHAR` | `text_buffer_insert(&selected->buf, ch)` if selected |
| `EVENT_BACKSPACE` | `text_buffer_backspace(&selected->buf)` if selected |
| `EVENT_DELETE_BAND` | `delete_node(e, e->selected)` |
| `EVENT_QUIT`, `EVENT_NONE` | no-op |

`EVENT_QUIT` is a process concern, not an editor concern: `main()` checks
for it before calling `editor_apply()`, so the editor never needs a quit
flag or a return value.

`layout()`:

1. Visible rows = `min(e->rows, LAYOUT_MAX_LABELS)`. If `e->cols <= 0`,
   return an empty layout (preserves `paint_label()`'s `cols == 0` guard).
2. Walk to the tail, then walk back via `prev`, assigning row 1, 2, 3, …
3. For each band with `row <= visible rows`, place a label:
   `col = (cols - len) / 2 + 1`. Stop once rows run out (**clip** — bands
   below the fold still exist, can be selected and typed into, they just
   aren't placed).
4. If the selected band was placed: `caret_visible = 1`,
   `caret_row = its row`, `caret_col = (cols - len) / 2 + cursor + 1`.
   Otherwise (`selected == NULL`, or selected is below the fold)
   `caret_visible = 0`. The caret shows in both modes, as it does today.

Scrolling to keep the selection on screen is a feature, not part of this
refactor.

### `paint.[ch]` — render one frame

```c
#include "layout.h"

void paint_wallpaper(void);
void paint_frame(const struct layout *l);
```

`paint_label()`, `paint_delete_row()` and `paint_hide_cursor()` are
removed. `paint_frame()` emits:

```
\x1b[?2026h                     begin synchronized update
\x1b[38;2;201;201;207m          ink
\x1b[2J                         clear (fills with wallpaper bg set at startup)
for each label: \x1b[row;colH + text
caret_visible ? \x1b[r;cH \x1b[?25h : \x1b[?25l
\x1b[?2026l                     end synchronized update
```

Mode 2026 makes the terminal compose the frame off-screen and swap it in
atomically, so a full clear per event doesn't flicker (WezTerm, kitty,
iTerm2, Ghostty, Alacritty, foot, Windows Terminal support it; others
ignore the unknown mode). Because the whole screen is cleared, paint needs
no `rows`. Existing per-call `write()`s are kept; batching the frame into a
single `write()` is a possible follow-up.

### `main.c` — thin loop

`enter()` reads both `ws.ws_col` and `ws.ws_row`. Then:

```c
struct editor *e = editor_new(cols, rows);
if (!e) exit(1);

while (read(STDIN_FILENO, &byte, 1) == 1) {
  struct key_event ev = input_parse(&parser, byte, editor_mode(e));
  if (ev.type == EVENT_QUIT)
    break;
  editor_apply(e, ev);
  struct layout l = layout(e);
  paint_frame(&l);
}

editor_free(e);
```

The globals `bands`, `selected`, `cols`, the local `mode`, `struct band`,
both node helpers, the `changed` flag and the `goto done` all leave
`main.c`.

### `Makefile`

- `SRCS` gains `editor.c`.
- New `$(BUILD_DIR)/test_editor` from `tests/test_editor.c editor.c
  text_buffer.c`, added to `TEST_BINS` and run by `make test`.
- If spec 017 (clang-format kernel style) has landed, its `FORMAT_FILES`
  wildcard picks up the new files automatically. Write them in kernel style
  (the snippets in this spec are illustrative, not formatted) and keep
  `make lint` green.

### Components

- `layout.h` (new) — `struct placed_label`, `struct layout`,
  `LAYOUT_MAX_LABELS`; data only.
- `editor.[ch]` (new) — opaque `struct editor`, `struct band`; knows the
  band list, selection, mode, screen size; does `editor_new/free`,
  `editor_apply`, `editor_mode`, `layout`. Collaborates with `text_buffer`;
  uses `input.h` types only.
- `paint.[ch]` — `paint_wallpaper`, `paint_frame`; knows nothing of the
  editor, only `layout.h`.
- `main.c` — terminal setup/restore, read loop, quit; wires parser → editor
  → layout → paint.
- `input.[ch]`, `text_buffer.[ch]` — untouched.

### Testing

`tests/test_editor.c`, `assert.h` style like the other suites. **Black-box
only**: build with `editor_new(cols, rows)`, drive with `editor_apply()`,
observe with `layout()` and `editor_mode()`. The opaque struct enforces
this at compile time. Small helpers in the test file (e.g. `apply(e,
EVENT_ADD_BAND)`, `type(e, "hi")`) keep tests readable.

- New editor: `count == 0`, caret hidden, mode `MODE_MOVE`.
- Add band: `count == 1`, row 1, len 0, `col == cols/2 + 1`, caret at row 1,
  mode `MODE_TYPE`.
- Type "hi": label text "hi", len 2, centered col, caret col after "i".
- Backspace removes the last char; backspace on empty is a no-op.
- Three adds: rows 1, 2, 3 in order; caret on row 3 (newest at bottom).
- Escape → `MODE_MOVE`; enter-type → `MODE_TYPE`.
- Select up from the bottom moves the caret up one row; select up at row 1
  stays; select down at the bottom stays.
- Typing after select up edits the selected band, not the newest.
- Delete middle of three: `count == 2`, rows 1 and 2 (renumbered), caret on
  the band that was above.
- Delete top band: band below becomes selected.
- Delete the only band: `count == 0`, caret hidden; delete again is a no-op.
- Char / backspace with no bands: no-op, no crash.
- Clip: `rows = 2`, add three → `count == 2`, caret hidden (selection is on
  row 3); select up → caret visible on row 2.
- Capacity: `cols = 10`, type 10 chars → `len == 9`.

`paint_frame()` and `main.c` stay hand-judged end to end: add three bands,
type in each, move with j/k, delete middle/top/last, confirm the screen
matches the old behavior with no flicker, and that quitting restores the
terminal. Run under `make test` with no leaks reported by
`leaks --atExit` / ASan for `test_editor`.

## Out of scope

- Scrolling (keeping the selection visible past the bottom row).
- Terminal resize (`SIGWINCH`) — `cols`/`rows` are still read once at
  startup.
- Single `write()` per frame.
