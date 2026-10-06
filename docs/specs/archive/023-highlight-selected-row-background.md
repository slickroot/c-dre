# highlight-selected-row-background

Doug has a few rows on his diagram, and he's made one of them 5 lines tall. He presses `Esc` to move around. The row he's on lights up as a soft #1C1C20 block that covers all 5 lines, edge to edge across the screen, and there's no blinking caret. He presses `j` and the block moves to the next row, which is empty, but he can still see exactly where it is. He lands on a dim row, and its grey text sits quietly on the highlight. Happy, he presses `i` to type, and the highlight gives way to the familiar caret.

## Acceptance Criteria
- While Doug is moving around (not typing), the highlighted row has a #1C1C20 background across the full width of the screen.
- The background covers every line of the row, so a 5-line row shows a 5-line block.
- While Doug is moving around, the caret is hidden.
- Only the highlighted row has the background. The other rows sit on the canvas.
- When Doug moves with `j`/`k`, the background moves to the new row and the old row goes back to the canvas.
- A dim row's text stays #6B6B73 while it's highlighted.
- An empty row lights up like any other row.
- While Doug is typing, the screen looks the same as today: caret showing, no background.

## Technical Design

The pipeline stays as it is: `input_parse` → `editor_apply` → `layout()` →
`paint_frame`. Nothing changes on the input side, and `editor_apply` and
`struct band` are untouched. `layout()` already knows everything the highlight
needs: which band is selected, the mode, and the band's `pad`. It passes that
information along on the label, and `paint.c` turns it into a background. No new
files and no new events.

### The label carries its pad, and the highlight is a style attribute

```c
struct style {
	int dim;
	int highlight;
};

struct placed_label {
	int row;
	int col;
	const char *text;
	int len;
	int pad;
	struct style style;
};
```

A band takes up `2 * pad + 1` screen lines with its label on the middle line,
so the highlighted block is the lines from `row - pad` to `row + pad`. There's
no separate highlight rectangle in `struct layout`, because the label already
describes the band's full height.

`highlight` lives next to `dim` in `struct style`. Both are semantic
attributes, so a label can be dim and highlighted at once (AC6).

### `layout()` marks the selected band in move mode and hides the caret

In the existing loop, next to `style`:

```c
p->pad = oldest->pad;
p->style = oldest->style;
p->style.highlight = oldest == e->selected && e->mode == MODE_MOVE;
```

The caret block only runs in type mode:

```c
if (oldest == e->selected && e->mode == MODE_TYPE) {
	l.caret_visible = 1;
	...
}
```

- **AC3:** in move mode `caret_visible` stays 0.
- **AC4:** only the selected band's label gets `highlight`.
- **AC5:** `highlight` is worked out again on every `layout()` call, so when
  `j`/`k` move `selected`, the old row loses it automatically.
- **AC8:** in type mode nothing is highlighted and the caret shows, the same as
  today.

`highlight` is never stored on `struct band`. It's derived state, so the band's
own `style` keeps only `dim`, and `insert_node()` doesn't change.

### `paint_frame` fills the block, then draws the text on top

A new `bg_for(struct style)` sits beside `fg_for`:

```c
static const char *bg_for(struct style s)
{
	return s.highlight ? "\x1b[48;2;28;28;32m" /* #1C1C20 */
			   : "\x1b[48;2;10;10;11m"; /* #0A0A0B */
}
```

In the label loop, every label writes its own background, so no colour can
carry over from one label to the next. For a highlighted label, each line from
`row - pad` to `row + pad` is filled before the text is drawn: move to column 1,
then `ESC[K`. `ESC[K` clears to the end of the line in the current background,
so the fill spans the full width and `paint.c` never needs to know the screen
width (AC1, AC2). Then comes the existing cursor-position escape, `fg_for`, and
the text. The text is drawn with the highlight background still active, so it
sits on the block. A dim label keeps its #6B6B73 foreground (AC6), and an empty
label (`len == 0`) still gets its block (AC7).

After the loop, `paint_frame` writes the canvas background `\x1b[48;2;10;10;11m`
once more. This keeps the contract from 013: canvas is the active background
between frames, so the next frame's `ESC[2J` clears to canvas instead of to
#1C1C20.

The block never goes above line 1, because the first band's top line is line 1.
`layout()` only places a label when its middle line fits on screen, so the
bottom padding lines can run past the last line. The terminal clamps those
cursor moves to the last line, and since the fill happens before the text, the
label is still drawn on top.

### Components

- `layout.h`: `struct style` gains `int highlight`. `struct placed_label` gains
  `int pad`.
- `layout()` (`editor.c`): copies `pad` onto each label, and sets
  `style.highlight` for the selected band in `MODE_MOVE`. It sets the caret only
  in `MODE_TYPE`.
- `paint.c`: new static `bg_for(struct style)`. Each label writes its
  background. A highlighted label fills `row - pad` through `row + pad` with
  `ESC[K` before its text. The canvas background is restored after the loop.
- `struct band`, `editor_apply()`, `input.[ch]`, `text_buffer.[ch]`, `main.c`
  and `paint_wallpaper()` are unchanged.

### Testing

`tests/test_editor.c`, asserting through `layout()`:

- In move mode, the selected band's label has `style.highlight == 1` and
  `caret_visible == 0` (AC1, AC3).
- A band grown with `]` twice has a label with `pad == 2`, and every other label
  has its own pad (AC2).
- With three bands, only the selected label is highlighted (AC4).
- After `j`/`k`, the highlight moves to the new label and the old label goes
  back to `highlight == 0` (AC5).
- A dim selected band in move mode has `dim == 1` and `highlight == 1` (AC6).
- An empty selected band in move mode is highlighted (AC7).
- In type mode, no label is highlighted and `caret_visible == 1` with the same
  caret position as before (AC8).
- Existing caret tests that run in move mode are updated to expect
  `caret_visible == 0`.

`paint.c` still has no unit-test harness, the same decision as in 013, 018 and
020. Check it by hand:

1. Add three rows, type in two of them, grow one to 5 lines with `]`, and press
   `Esc`.
2. The highlighted row should be a #1C1C20 block, edge to edge, with no caret.
3. Press `j`/`k`. The block should follow, and the old row should go back to
   canvas.
4. Land on the 5-line row. The block should be 5 lines tall.
5. Land on the empty row. It should still show a block.
6. Dim a row with `-` and highlight it. The grey text should sit on the block.
7. Press `i`. The block should disappear and the caret should come back.
8. Quit and check that the shell's own colours come back.
