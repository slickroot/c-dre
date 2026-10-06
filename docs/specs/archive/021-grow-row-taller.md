# grow-row-taller

Doug has a few rows stacked on his diagram, and one of them deserves more room. He moves his highlight onto it and presses `]`. The row grows to three lines tall, with its text sitting right in the middle. The rows below it slide down to make room. He presses `]` again and the row is now five lines tall, still with its text centred. Happy with how roomy it looks, he presses `i` and keeps typing on the middle line.

## Acceptance Criteria
- While Doug is moving around (not typing), pressing `]` makes the highlighted row 2 lines taller: one more line above its text and one more line below.
- Rows always have an odd height (1, 3, 5, 7, …) and the text sits on the exact middle line.
- There's no limit. Doug can keep pressing `]` and the row keeps growing.
- Rows below the grown row are pushed down by 2 lines. Rows above it stay where they are.
- Only the highlighted row grows. Other rows keep their height.
- While Doug is typing in a row, pressing `]` types a "]" and doesn't change the height.
- A new row added with `a` always starts at 1 line, even if the row highlighted before it was taller.
- When Doug types into a tall row, the caret sits on the middle line next to the text.
- If nothing is highlighted, pressing `]` does nothing.

## Technical Design

The pipeline stays as it is: `input_parse` → `editor_apply` → `layout()` →
`paint_frame`. The band owns its height as a `pad` count. `layout()` turns that
into screen rows by stacking bands with a running `top`. `paint.c` is untouched,
because bands have no background (013) and a taller row is just a label placed
lower, with empty lines around it. No new files.

### The band owns `pad`, not `height`

`struct band` in `editor.c` gains:

```c
struct band {
	struct text_buffer buf;
	struct style style;
	int pad;
	struct band *next;
	struct band *prev;
};
```

`pad` is the number of empty lines above the text, which is also the number of
empty lines below it. The band's height is `2 * pad + 1`. Storing `pad` instead
of `height` makes AC2 true by construction: every height is odd, and the text
line is always `top + pad`, the exact middle. There is no "is it odd?" check
anywhere.

`pad` lives on the band and not in `struct style`. Height is geometry that
`layout()` uses, not an attribute `paint.c` interprets.

### New bands start at one line

`insert_node()` sets it explicitly, next to the style:

```c
b->pad = 0;
```

This covers AC7: a band added with `a` is one line tall whatever the previously
highlighted band was. `pad` is never copied from a neighbour.

### `]` is `EVENT_GROW_BAND`, in move mode only

`enum key_event_type` gains `EVENT_GROW_BAND`. `input_parse()` gains one branch
in the `MODE_MOVE` block, after `-`:

```c
if (byte == ']')
	return (struct key_event){EVENT_GROW_BAND, 0};
```

The `MODE_TYPE` path is unchanged. `]` (0x5d) is printable, so it already
returns `EVENT_CHAR` with `ch == ']'`. AC6 needs no code.

### The editor grows the highlighted band

```c
case EVENT_GROW_BAND:
	if (e->selected)
		e->selected->pad++;
	break;
```

This one case covers four criteria:

- **AC1:** each press adds one line above and one below, so the band is 2 lines
  taller.
- **AC3:** there is no upper bound. In practice `int` overflow is out of reach.
- **AC5:** only `selected` is touched.
- **AC9:** the `NULL` guard makes it a no-op when nothing is highlighted.

Mode is left unchanged.

### `layout()` stacks bands with a running `top`

The loop's `row++` counter is replaced by `top`, the first screen line of the
current band. The label and caret are placed on the band's middle line:

```c
int top = 1;
for (; oldest; oldest = oldest->prev) {
	int row = top + oldest->pad;
	if (row > visible)
		break;

	/* place label at `row`, and the caret at `row` if selected, as today */

	top += 2 * oldest->pad + 1;
}
```

- **AC4:** every band below a grown band is pushed down by 2 per `]`, and bands
  above it keep their `top`.
- **AC8:** the caret uses the same `row` as the label, so it sits on the middle
  line next to the text.

**Clipping rule:** a band is drawn only if its text line fits on screen. If a
band's top padding is visible but its middle line is past the bottom edge, that
band is skipped, and so is everything after it. The bottom padding of the last
drawn band may run past the edge. Nothing is drawn there, so it doesn't matter.
`LAYOUT_MAX_LABELS` is still respected because labels never outnumber
`visible` rows.

`struct placed_label` is unchanged. Its `row` is now the band's middle line.
`top` and `height` are not exported, because no consumer needs them yet.

### Components

- `struct band` (`editor.c`): gains `int pad`. `insert_node()` sets it to 0.
- `enum key_event_type`: gains `EVENT_GROW_BAND`.
- `input_parse()`: move mode reads `]`. The signature is unchanged.
- `editor_apply()`: new `EVENT_GROW_BAND` case that does `selected->pad++`
  behind a `NULL` guard.
- `layout()`: stacks bands with a running `top`, places the label and caret at
  `top + pad`, and clips on the text line.
- `layout.h`, `paint.[ch]`, `text_buffer.[ch]` and `main.c` are unchanged.

### Testing

`tests/test_input.c`:

- `]` in `MODE_MOVE` returns `EVENT_GROW_BAND`.
- `]` in `MODE_TYPE` returns `EVENT_CHAR` with `ch == ']'`.

`tests/test_editor.c`, asserting through `layout()`:

- A single band grown once has its label and caret on row 2 (the middle of rows
  1–3). Grown twice, it's on row 3, the middle of rows 1–5 (AC1, AC2, AC8).
- With three bands, growing the middle one leaves the first on row 1, puts the
  middle on row 3 and the last on row 5. Growing the middle again gives rows 1,
  4 and 7 (AC4, AC5).
- Growing many times keeps adding 2 lines with no cap. For example, 10 presses
  put a lone band's label on row 11 when there are enough screen rows (AC3).
- Growing a band and then pressing `a`: the new band is one line, directly below
  the grown band's bottom padding (AC7).
- After growing, typing and backspacing update the text and keep the caret on
  the middle row (AC8).
- Growing with no bands is a no-op and doesn't crash (AC9).
- Growing doesn't change `editor_mode()`.
- Clipping: on a 4-row screen, if one band is grown twice so its text is on row
  3, a second band whose middle line would be row 6 is not placed.
  `l.count == 1`.
- Clipping: when a grown band's text line is past the bottom edge but its top
  padding is visible, the band is not placed and its caret is not visible.

`paint.c` doesn't change, so there's nothing new to check there. Check it by
hand:

1. Add three rows with text and press `Esc`.
2. Highlight the middle row and press `]`. Its text should move down one line,
   and the row below should move down two.
3. Press `]` again. The text should move down one more line, and the row below
   should move down two more.
4. Press `i` and type. The caret should be on the text line.
5. Press `Esc`, then `a`. The new row should be one line tall.
6. In type mode, `]` should insert a literal "]".
