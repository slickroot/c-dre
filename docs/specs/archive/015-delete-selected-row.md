# delete-selected-row

Doug is looking at his diagram with a few rows stacked on it. He moves his highlight onto a row he no longer wants and presses `d`. The row disappears, and the rows underneath move up one row so the diagram stays stacked with no gap.

## Acceptance Criteria
- While Doug is moving around (not typing), pressing `d` deletes the highlighted row.
- The deleted row disappears and the rows underneath it move up one row to close the gap, so the remaining rows stay stacked with no empty row between them.
- After the deletion, the highlight lands on the row that was above the deleted one.
- If the deleted row was the topmost row, the highlight lands on the row below it.
- If the deleted row was the only row, the screen is left empty with nothing highlighted.
- If nothing is highlighted, pressing `d` does nothing.
- While Doug is typing text into a row, pressing `d` types the letter "d" and does not delete the row.

## Technical Design

Still one `main.c` plus `input`, `text_buffer` and `paint`. No new module, no
new file, no change to `struct band` or `text_buffer.[ch]`. The work is: one new
parser event, one new static helper in `main.c` that owns the whole delete, and
two small additions to `paint.[ch]`. `selected` is always the deleted node,
because `d` acts on the highlighted row and nothing else.

### Rows stay stored, and are renumbered in memory

`row` remains a per-band field assigned once at creation. Deleting a band from
the middle would break the contiguity invariant (rows `1..N` with no gaps), so
the helper rewrites it: after unlinking, it walks the list from `bands` and
decrements `row` for every band whose row was below the gap.

```c
for (struct band *b = bands; b; b = b->next)
  if (b->row > old_row)
    b->row--;
```

This is a pure in-memory walk — no I/O — and it keeps `row` truthful for the
two things that read it later: `a` placing a new band at `bands->row + 1`, and
`paint_label` positioning the caret at `selected->row`. The pixels are moved by
a separate, single terminal escape (below), not by this walk.

### `delete_selected()` owns unlink, renumber, shift and re-seat

One static helper in `main.c`, called from a new `EVENT_DELETE_BAND` case. It
is self-contained for the same reason `EVENT_ADD_BAND` already is: the case
that restructures the list does its own painting, and `changed` stays `0` so the
loop's post-switch repaint never fires for `d`.

```c
static void delete_selected(void) {
  if (!selected)
    return;

  struct band *victim = selected;
  int old_row = victim->row;
  struct band *heir = victim->next ? victim->next : victim->prev;

  if (victim->prev)
    victim->prev->next = victim->next;
  if (victim->next)
    victim->next->prev = victim->prev;
  if (bands == victim)
    bands = victim->next;

  for (struct band *b = bands; b; b = b->next)
    if (b->row > old_row)
      b->row--;

  text_buffer_free(&victim->buf);
  free(victim);
  selected = heir;

  paint_delete_row(old_row);
  if (selected)
    paint_label(&selected->buf, cols, selected->row);
  else
    paint_hide_cursor();
}
```

Walking the list from `bands` via `next` visits every remaining node regardless
of direction, so the `prev` fixups only need to keep the links consistent; the
renumber loop needs no separate traversal.

### Which band is highlighted next falls out of `next`/`prev`

`bands` is the newest, bottom-most node and `next` points toward older, higher
bands; `prev` points toward newer, lower bands (established in 012). So "the
row that was above the deleted one" is `victim->next`, and "the row below" is
`victim->prev`. The single expression

```c
struct band *heir = victim->next ? victim->next : victim->prev;
```

is AC3 (prefer the band above), AC4 (topmost has `next == NULL`, so fall to the
band below) and AC5 (a lone band has both `NULL`, so `heir` is `NULL`) at once.
It is computed **before** the unlink so the neighbour pointers are still valid,
and assigned to `selected` only after the old node is freed.

`EVENT_DELETE_BAND` reuses the existing `selected == NULL` guard as the first
line of the helper, which is AC11 ("if nothing is highlighted, `d` does
nothing"): the function returns before touching anything.

### The pixels move with one `ESC[M`, not a repaint

Two additions to `paint.[ch]`, keeping every escape in `paint.c`:

```c
void paint_delete_row(int row);   /* position at (row,1), emit ESC[M */
void paint_hide_cursor(void);     /* emit ESC[?25l */
```

`ESC[M` (delete-line) removes the line under the cursor and pulls every line
below it up by one, with a blank line filled in at the bottom using the current
background — which is still the wallpaper's SGR, so the gap closes with no
visible seam. That is AC2: the bands below shift up exactly one row, and their
text is never re-emitted. Because `paint_label` leaves the hardware cursor on
the selected band, the cursor is already on `old_row` when `paint_delete_row`
positions and deletes it.

After the shift, the helper either repaints the new selection (which re-emits
that band's unchanged text and re-shows the caret via `ESC[?25h`, moving the
caret to the new `selected->row`) or, when the list is now empty, calls
`paint_hide_cursor()` so AC10's "nothing highlighted" leaves no caret on screen.
`paint_label` is untouched, and the loop's post-switch repaint line is left
exactly as it is — it cannot see a `NULL` `selected` because `d` never reaches
it.

### The parser reads `d` only in move mode

`enum key_event_type` gains `EVENT_DELETE_BAND`, and `input_parse()` gains one
branch in the `MODE_MOVE` block alongside `a`, `i`, `j` and `k`:

```c
if (byte == 'd')
  return (struct key_event){ EVENT_DELETE_BAND, 0 };
```

The `MODE_TYPE` path is unchanged, so `d` still falls through to the printable
range and yields `EVENT_CHAR` with `ch == 'd'` — AC12 ("while typing, `d` types
`d`") needs no code. This is the same shape as `a`/`i`/`j`/`k`.

### The new switch case is one line

```c
case EVENT_DELETE_BAND:
  delete_selected();
  break;
```

`changed` is left `0` and `EVENT_DELETE_BAND` is not added to the post-switch
condition, so the existing repaint line does not run a second time.

### Components

- `enum key_event_type` — gains `EVENT_DELETE_BAND`
- `input_parse()` — move mode reads `d`; signature unchanged
- `main.c` `delete_selected()` — **new** static helper: NULL guard, compute
  `heir`, unlink, renumber rows below, free, re-seat `selected`, shift and
  repaint-or-hide
- `main.c` `switch` — new `EVENT_DELETE_BAND` case calling the helper
- `paint.c` / `paint.h` — `paint_delete_row(int row)` and `paint_hide_cursor()`
- `struct band`, `text_buffer.[ch]`, `paint_label()`, `paint_wallpaper()` — untouched

### Testing

`tests/test_input.c` gains parser cases following the existing pattern:

- `d` in `MODE_MOVE` yields `EVENT_DELETE_BAND`;
- `d` in `MODE_TYPE` is still printable ASCII, so it yields `EVENT_CHAR` with
  `ch == 'd'`, same as `i` already does.

The list surgery, the `next`/`prev` landing rule and the `ESC[M` shift live in
`main.c`, which has no unit-test harness — the same decision 011 and 012 made
for the band list, and 013/014 for `paint.c`. Hand-judged end to end: add three
bands with `a`, type distinct text into each, press `Esc`, highlight the middle
band and press `d`; confirm it disappears, the band below moves up one line with
no gap, and the caret lands on the band that was above. Highlight the topmost
band, press `d`, and confirm the caret lands on the band below. Add a single
band, press `d`, and confirm the screen is empty with no caret. Press `d` with
nothing highlighted and confirm nothing happens. Press `i`, type `d`, and
confirm a literal `d` appears and no row is deleted. Quit and confirm the
terminal is restored.
