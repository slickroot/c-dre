# switch-row-between-normal-and-dim

Doug has a few rows stacked on his diagram, and one of them matters less than the others. He moves his highlight onto it and presses `-`. The row's text turns a quiet grey and fades into the background. He changes his mind, presses `-` again, and the row is back to normal. Satisfied, he moves on to the next row.

## Acceptance Criteria
- While Doug is moving around (not typing), pressing `-` on a normal row makes it dim. Its text shows in #6B6B73.
- While Doug is moving around, pressing `-` on a dim row makes it normal again, in the usual text colour.
- Only the highlighted row changes. Other rows keep their role.
- While Doug is typing in a row, pressing `-` types a "-" and doesn't change the role.
- A new row added with `a` always starts as normal, even if the row highlighted before it was dim.
- When Doug types into a dim row, the whole row stays dim, including the new text.
- If nothing is highlighted, pressing `-` does nothing.

## Technical Design

The pipeline stays as it is: `input_parse` → `editor_apply` → `layout()` →
`paint_frame`. The band owns its look in a new `struct style`, `layout` copies
that style onto each placed label, and only `paint.c` turns it into a colour. No
new files.

### `struct style` lives in `layout.h`, and the band owns one

```c
struct style {
	int dim;
};

struct placed_label {
	int row;
	int col;
	const char *text;
	int len;
	struct style style;
};
```

`struct style` is declared in `layout.h` above `struct placed_label`. `editor.c`
already sees it through `editor.h` → `layout.h`, and `paint.c` sees it through
`paint.h`. It holds semantic attributes, not colours. A zeroed `struct style`
means "normal", and later styling grows this struct instead of adding more
fields to the band.

`struct band` in `editor.c` gains the source of truth:

```c
struct band {
	struct text_buffer buf;
	struct style style;
	struct band *next;
	struct band *prev;
};
```

### New bands start normal

`insert_node()` uses `malloc`, so it sets the style explicitly:

```c
b->style = (struct style){0};
```

This covers AC5: a band added with `a` is normal no matter what the previously
highlighted band was. Style is never copied from a neighbour.

### `-` is `EVENT_TOGGLE_DIM`, in move mode only

`enum key_event_type` gains `EVENT_TOGGLE_DIM`. `input_parse()` gains one branch
in the `MODE_MOVE` block, next to `a`, `i`, `j`, `k` and `d`:

```c
if (byte == '-')
	return (struct key_event){EVENT_TOGGLE_DIM, 0};
```

The `MODE_TYPE` path is unchanged. `-` (0x2d) is printable, so it already
returns `EVENT_CHAR` with `ch == '-'`. AC4 needs no code.

### The editor flips the highlighted band's style

```c
case EVENT_TOGGLE_DIM:
	if (e->selected)
		e->selected->style.dim = !e->selected->style.dim;
	break;
```

This one case covers four criteria:

- **AC1 and AC2:** the toggle goes normal → dim → normal.
- **AC3:** only `selected` is touched.
- **AC7:** the `NULL` guard makes it a no-op when nothing is highlighted.

Mode is left unchanged.

### `layout()` copies the style onto the label

In the existing loop, next to `text` and `len`:

```c
p->style = oldest->style;
```

Style belongs to the band, not to its text. Typing or backspacing into a dim
band keeps the whole label dim, which covers AC6 without extra code.

### `paint_frame` sets the foreground per label

The single frame-wide `38;2;201;201;207` write at the top of `paint_frame` is
removed. Each label now states its own colour just before its text, so no colour
can leak from one label to the next and there is no "restore" step:

```c
static const char *fg_for(struct style s)
{
	return s.dim ? "\x1b[38;2;107;107;115m"   /* #6B6B73 */
		     : "\x1b[38;2;201;201;207m";
}
```

In the loop, after the cursor-position escape:

```c
const char *fg = fg_for(p->style);
write(STDOUT_FILENO, fg, strlen(fg));
write(STDOUT_FILENO, p->text, p->len);
```

The hex values stay in `paint.c`, next to the wallpaper colour.

### Components

- `layout.h`: new `struct style { int dim; }`. `struct placed_label` gains
  `struct style style`.
- `struct band` (`editor.c`): gains `struct style style`. `insert_node()` zeroes
  it.
- `enum key_event_type`: gains `EVENT_TOGGLE_DIM`.
- `input_parse()`: move mode reads `-`. The signature is unchanged.
- `editor_apply()`: new `EVENT_TOGGLE_DIM` case that flips
  `selected->style.dim` behind a `NULL` guard.
- `layout()`: copies `style` onto each `placed_label`.
- `paint.c`: new static `fg_for(struct style)`. `paint_frame` writes the
  foreground per label instead of once per frame.
- `text_buffer.[ch]`, `main.c` and `paint_wallpaper()` are unchanged.

### Testing

`tests/test_input.c`:

- `-` in `MODE_MOVE` returns `EVENT_TOGGLE_DIM`.
- `-` in `MODE_TYPE` returns `EVENT_CHAR` with `ch == '-'`.

`tests/test_editor.c`, asserting through `layout()`:

- A new band's label has `style.dim == 0`.
- Toggling once makes the label dim, and toggling again makes it normal (AC1
  and AC2).
- With three bands, dimming the middle one leaves the other two normal (AC3).
- Dimming a band and then pressing `a`: the new label is normal and the old one
  is still dim (AC5).
- Typing into a dim band keeps `style.dim == 1` and updates the text (AC6).
- Toggling with no bands is a no-op and doesn't crash (AC7).
- Toggling doesn't change `editor_mode()`.

`paint.c` has no unit-test harness, the same decision as in 013, 014 and 018.
Check it by hand:

1. Add three rows with text and press `Esc`.
2. Highlight the middle row and press `-`. Its text should turn #6B6B73 and the
   other rows should stay #C9C9CF.
3. Press `-` again. The row should return to normal.
4. Dim a row and press `a`. The new row should be normal.
5. Dim a row, press `i`, and type. The new text should also be dim.
6. In type mode, `-` should insert a literal "-".
