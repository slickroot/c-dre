# step-into-row-and-back-out

Doug has a row on his diagram with "Login" on the left and "Logout" on the right, and he has selected that row. He presses `Enter`. The row's highlight goes away and "Login" lights up on its own, so he can see he's on one text and not on the row. He presses `Backspace`, and the highlight goes back to the whole row. Happy that he always knows what he has selected, he moves on to the next row.

## Acceptance Criteria

1. While a row is selected, pressing `Enter` selects the row's first text. The row's highlight goes away and only that text is highlighted.
2. The text highlight is the same colour as the row highlight. It covers just the letters, with no padding.
3. `Enter` works the same way on a row with one text: that text lights up on its own.
4. While a text is selected, pressing `Backspace` moves the highlight back to the whole row.
5. While a text is selected, `j` and `k` do nothing. Doug presses `Backspace` before moving to another row.
6. While a text is selected, `i`, `o`, `d`, `-`, `[`, `]`, `a` and `Enter` do nothing. `Backspace` is the only key that does anything.
7. `Enter` does nothing when no row is selected, and `Backspace` does nothing while a whole row is selected.
8. `Backspace` still deletes characters while Doug is typing, as it does today.

## Technical Design

**Depends on 032 (display list), implemented first.** This design uses
`layout()` → `display_list()` → `paint_frame()`, the `struct node` `box`, and
`OP_FILL`/`OP_TEXT` ops.

### Selection model

`e->selected` can point at **any node**: a band (its parent is the root) or a
text (its parent is a band). There is no second field. Stepping in goes to a
child and stepping out goes to the parent, which also fits the nested boxes
and text-to-text movement of 026 and 027.

```c
static int is_band(const struct editor *e, const struct node *n)
{
	return n && n->parent == e->root;
}
```

### `input.[ch]`

Two new events, parsed only in `MODE_MOVE`:

| Byte             | Event             |
|------------------|-------------------|
| `0x0d`, `0x0a`   | `EVENT_STEP_IN`   |
| `0x7f`, `0x08`   | `EVENT_STEP_OUT`  |

- `EVENT_BACKSPACE` stays type-mode only, so AC8 holds by construction.
- `Enter` stays `EVENT_NONE` in type mode.
- The parser stays a stateless byte-to-event map by mode. The editor decides
  whether a step is legal.

### `editor.c`

- `EVENT_STEP_IN`: if `mode == MODE_MOVE`, `is_band(selected)` and
  `selected->first_child`, then `selected = selected->first_child`.
  The mode check is what keeps the invariant "a selected text is always in
  `MODE_MOVE`" true even when `editor_apply` is called directly. The
  `first_child` check cannot fail today because `add_band` always appends a
  text, and it stays as a guard.
- `EVENT_STEP_OUT`: if `selected` is not NULL and `!is_band(selected)`, then
  `selected = selected->parent`. No mode check is needed, because of the
  `STEP_IN` guard.
- Every band-level handler (`SELECT_UP`, `SELECT_DOWN`, `ADD_TEXT`,
  `ENTER_TYPE`, `DELETE_BAND`, `TOGGLE_DIM`, `GROW_BAND`, `SHRINK_BAND`,
  `CHAR`, `BACKSPACE`) guards on `is_band(e, e->selected)`. That gives AC5,
  AC6 and AC7, and it stops `active_text()` from ever receiving a text node.
  `ADD_BAND` is not guarded, and it moves the selection to the new band.
- `ESCAPE` is unchanged. It sets `MODE_MOVE`, which is already true when a
  text is selected.

### `display.c`

Inside the band's text loop, before the text's `OP_TEXT`:

```c
if (t == selected && mode == MODE_MOVE)
	emit_fill(out, t->box, highlight);
```

- It uses the text's `box` as is: no padding, and the same `highlight` colour
  as the band.
- The band's own fill is emitted only when `band == selected`, so a selected
  text lights up alone and the row highlight goes away.
- The caret code is unchanged. It only runs in `MODE_TYPE`, when `selected`
  is always a band.
- `paint.c` and `layout.c` are untouched.

### Known limitation (out of scope)

An **empty selected text shows no highlight**. Its `box.cols` is 0, so its
fill is zero-width, and `grid_fill` draws nothing for it. The row highlight is
already gone, so nothing is visible. `Backspace` steps out and restores the row
highlight, so Doug cannot get stuck. Widening an empty text to one cell is a
separate visual decision.

### Components

- `input.[ch]`: knows the byte-to-event map by mode. Gains the two events.
- `editor.c`: knows `selected` as any node, plus `is_band`. Gains the step
  events and the band guard.
- `display.c`: knows what a selection looks like. Gains the text highlight.
- `node`, `layout`, `paint`, `grid`, `main`: untouched.

### Testing

Slices, each built with TDD:

1. **`test_input`:** `0x0d` and `0x0a` in `MODE_MOVE` give `EVENT_STEP_IN`.
   `0x7f` and `0x08` in `MODE_MOVE` give `EVENT_STEP_OUT` (these replace the
   two assertions at `tests/test_input.c:181-186` that expected
   `EVENT_NONE`). `0x0d` in `MODE_TYPE` gives `EVENT_NONE`, and `0x7f` in
   `MODE_TYPE` still gives `EVENT_BACKSPACE`.
2. **`test_display`:** with a text selected in `MODE_MOVE`, there is no band
   fill, and there is an `OP_FILL` with `highlight` over the text's `box`
   before its `OP_TEXT`. The other text has no fill. In `MODE_TYPE`, or with a
   band selected, there is no text fill.
3. **`test_editor`**, at screen level through `frame(e)`:
   - AC1, AC3: `Enter` on a row clears the row highlight and highlights only
     the first text's letters, with `CANVAS` around them. This holds for one
     text and for two.
   - AC2: the highlight has the row's colour and no padding. Check the cells
     just outside the letters.
   - AC4: `Backspace` brings back the row highlight.
   - AC5: `j` and `k` leave the selection unchanged.
   - AC6: each of `i o d - [ ] a Enter` leaves the screen and mode unchanged.
   - AC7: `Enter` with nothing selected does nothing, and `Backspace` on a row
     does nothing.
   - AC8: typing and `Backspace` in type mode still delete characters.
   - Direct `editor_apply(STEP_IN)` in `MODE_TYPE` does nothing.
   - Known limitation: an empty selected text has no highlighted cell.
