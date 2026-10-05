# one-row-bands

Sam adds a new band and it appears exactly one line below the last one. Sam drops in empty bands as spacers, nudging their centered text down row by row until it sits where they want.

## Acceptance Criteria
- Each band occupies a single line.
- A newly added band appears exactly one line below the previous band.
- An empty band shifts the next band down by one line.

## Technical Design

A two-file change that collapses the band's 3-row block down to a single line:
`main.c` renames and re-spaces the position field, and `paint.c` stops drawing
text on a middle row and stops erasing three rows. No new module, no new event,
no change to `struct band` beyond the field rename, and no signature change to
`paint_label` (only its parameter name).

### `top_row` becomes `row` and means the band's one line

The field is renamed because the `top_` prefix described a block whose text sat
on the middle row — a concept that disappears here:

```c
struct band {
  struct text_buffer buf;
  int row;
  struct band *next;
  struct band *prev;
};
```

`row` is the single screen line the band occupies. Bands stack downward from
row `1`, so creation loses the `+ 3` spacing:

```c
b->row = bands ? bands->row + 1 : 1;
```

and the two `paint_label` call sites pass `b->row` / `selected->row` instead of
`top_row` (`main.c:81`, `main.c:115`). AC2 is exactly this `+ 1`: `bands` is
the newest, bottom-most band, so each `a` lands one row under the previous one.

### Every band consumes a row, empty or not

`paint_label` is called for every band, and it always erases its row first,
then draws the text — of which there may be none. An empty band (`buf->len ==
0`) therefore still erases and occupies its line, so the next band added is one
row below it. AC3 needs no special case: it is the same `+ 1` spacing applied
to a band whose text happens to be empty. The spacers the story describes fall
straight out of the uniform layout.

### The renderer draws and erases on `row`, one row tall

`paint_label` currently positions at `top_row`, erases three rows, then offsets
text and caret to `top_row + 1`. Both the offset and the extra two rows go. The
erase sequence collapses to a single `ESC[K`, still ending with the cursor-show
that the caller relies on:

```c
static const char erases[] = "\x1b[K"
                             "\x1b[?25h";
```

Text and caret use `row` with no `+ 1`:

```c
int n = snprintf(cup, sizeof cup, "\x1b[%d;%dH", row, (cols - buf->len) / 2 + 1);
...
int c = snprintf(caret, sizeof caret, "\x1b[%d;%dH", row,
                 (cols - buf->len) / 2 + buf->cursor + 1);
```

AC1 falls out of this: the erase touches exactly one line, and the glyphs are
written on that same line.

### Interaction with 013

013 (band-no-background) also edits `paint_label` but only its SGR: it deletes
the band fill and the trailing `ESC[0m`. Its design note says the `erases`
sequence is left untouched — that note describes 013 in isolation and is
superseded on the row count by this spec. Implemented together, the final
`paint_label` erases one row, draws the text on `row` with the `#C9C9CF` ink
only, and emits no reset; the wallpaper background stays active for `ESC[K`.
The two changes touch disjoint parts of the function (colour vs. geometry) and
do not conflict.

### Components

- `struct band` — field `top_row` renamed to `row`
- `main.c` `EVENT_ADD_BAND` — `b->row = bands ? bands->row + 1 : 1`
- `main.c` `paint_label` call sites — pass `row`
- `paint.c` `paint_label()` — parameter renamed `top_row` → `row`; erase one
  row; draw text and caret at `row`
- `paint.h` — `paint_label` parameter renamed
- `paint_wallpaper()`, `text_buffer.[ch]`, `input.[ch]` — untouched

### Testing

Manual, hand-judged. The layout rule lives in `main.c`, which has no unit-test
harness, and `paint_label` writes to `STDOUT_FILENO` directly (the same reason
013 skips a paint test); extracting the `+ 1` arithmetic into a testable
function would be a refactor this story does not justify. End to end: start
`dre`, press `a` and confirm the first band's text sits on the top line; press
`a` again and confirm the new band is exactly one line below, not three; press
`a` with no typing to drop an empty spacer and confirm the next band lands one
line under it; backspace a band's text away and confirm it leaves a single
blank line; quit and confirm the terminal is restored.
