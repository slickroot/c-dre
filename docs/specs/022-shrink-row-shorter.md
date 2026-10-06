# shrink-row-shorter

Doug made one of his rows five lines tall, but now it takes up too much room. He moves his highlight onto it and presses `[`. The row shrinks to three lines tall, with its text still sitting right in the middle. The rows below it slide up to close the gap. He presses `[` again and the row is back to a single line. He presses `[` once more and nothing happens, because a row can't get any smaller than one line.

## Acceptance Criteria
- While Doug is moving around (not typing), pressing `[` makes the highlighted row 2 lines shorter: one line less above its text and one line less below.
- The row keeps an odd height (…, 5, 3, 1) and the text stays on the exact middle line.
- A row that is 1 line tall stays at 1 line when Doug presses `[`. Nothing else on screen changes.
- Rows below the shrunk row move up by 2 lines. Rows above it stay where they are.
- Only the highlighted row shrinks. Other rows keep their height.
- While Doug is typing in a row, pressing `[` types a "[" and doesn't change the height.
- `]` and `[` undo each other: growing a row and then shrinking it puts every row back where it was.
- When Doug types into a row he has shrunk, the caret sits on the middle line next to the text.
- If nothing is highlighted, pressing `[` does nothing.

## Technical Design

This builds on spec 021: `struct band` already has `int pad`, and `layout()`
already stacks bands with a running `top` and places the text at `top + pad`.
Shrinking is the inverse of growing, so there's no new state and `layout()`
doesn't change. The only new rule is that `pad` never goes below 0. No new
files.

### `[` is `EVENT_SHRINK_BAND`, in move mode only

`enum key_event_type` gains `EVENT_SHRINK_BAND`. `input_parse()` gains one
branch in the `MODE_MOVE` block, after `]`:

```c
if (byte == '[')
	return (struct key_event){EVENT_SHRINK_BAND, 0};
```

The `MODE_TYPE` path is unchanged. `[` (0x5b) is printable, so it already
returns `EVENT_CHAR` with `ch == '['`. AC6 needs no code.

`[` is not ESC (0x1b) and doesn't start an escape sequence. The parser reads
one byte at a time, so a lone `[` in move mode is safe.

### The editor shrinks the highlighted band, floored at 0

```c
case EVENT_SHRINK_BAND:
	if (e->selected && e->selected->pad > 0)
		e->selected->pad--;
	break;
```

This one case covers five criteria:

- **AC1:** each press removes one line above and one below, so the band is 2
  lines shorter.
- **AC3:** the `pad > 0` guard leaves a one-line band as it is. `pad` never
  goes negative, so `layout()` never sees a band with height below 1.
- **AC5:** only `selected` is touched.
- **AC7:** `pad--` exactly undoes 021's `pad++`.
- **AC9:** the `NULL` guard makes it a no-op when nothing is highlighted.

Mode is left unchanged.

### `layout()` is unchanged

Spec 021's stacking already covers the rest:

- **AC2:** height is still `2 * pad + 1` and the text is still at `top + pad`.
- **AC4:** every band below a shrunk band starts 2 lines higher, and bands above
  it keep their `top`.
- **AC8:** the caret uses the same `row` as the label.

Shrinking can bring bands that were clipped back on screen. This needs no extra
code, because `layout()` recomputes everything from the bands each frame.

### Components

- `enum key_event_type`: gains `EVENT_SHRINK_BAND`.
- `input_parse()`: move mode reads `[`. The signature is unchanged.
- `editor_apply()`: new `EVENT_SHRINK_BAND` case that does `selected->pad--`,
  guarded by `selected != NULL` and `pad > 0`.
- `struct band`, `insert_node()`, `layout()`, `layout.h`, `paint.[ch]`,
  `text_buffer.[ch]` and `main.c` are unchanged.

### Testing

`tests/test_input.c`:

- `[` in `MODE_MOVE` returns `EVENT_SHRINK_BAND`.
- `[` in `MODE_TYPE` returns `EVENT_CHAR` with `ch == '['`.

`tests/test_editor.c`, asserting through `layout()`:

- A band grown twice and then shrunk once has its label and caret on row 2.
  Shrunk again, it's back on row 1 (AC1, AC2, AC8).
- Shrinking a one-line band leaves its label on row 1. Pressing `]` once after
  that puts it on row 2, which shows `pad` didn't go negative (AC3).
- With three bands, growing the middle one twice and then shrinking it once
  gives rows 1, 3 and 5 (AC4, AC5).
- Growing and then shrinking leaves every label and the caret exactly as they
  were (AC7).
- After shrinking, typing updates the text and keeps the caret on the middle
  row (AC8).
- Shrinking with no bands is a no-op and doesn't crash (AC9).
- Shrinking doesn't change `editor_mode()`.
- On a short screen, a band clipped by a tall band above it is placed again
  after that band is shrunk.

`paint.c` doesn't change. Check it by hand:

1. Add three rows with text and press `Esc`.
2. Highlight the middle row and press `]` twice, then `[`. Its text should move
   up one line, and the row below should move up two.
3. Press `[` again. All three rows should be on consecutive lines.
4. Press `[` once more. Nothing should change.
5. Press `i` and type. The caret should be on the text line.
6. In type mode, `[` should insert a literal "[".
