# add-box-inside-selected-row

Doug has a diagram with one row that says "Login", and he has selected that row. He presses `a`. A new empty box with a border appears to the right of "Login" on the same line, and it is now the selected one. "Login" stays where it is. He selects the row again and presses `a` once more, and another empty box appears after the first. Happy that he can build up a row of boxes, he carries on with his diagram.

## Acceptance Criteria

1. While a row is selected, pressing `a` adds an empty box with a border to the right of the row's existing content, on the same line. The new box becomes the selected one.
2. Existing content in the row, such as "Login", keeps its column and is not pushed aside by the new box. The row grows to fit the box, so the text may move down with it.
3. If Doug selects the row again and presses `a`, the new box goes after the row's existing children. Each new box becomes the selected one.
4. While a box or a text is selected, `a` does nothing.
5. When nothing is selected, as in an empty diagram, `a` still adds a new row, as it does today.

## Technical Design

### Model

- A box is a child of a band with `style.border = 1`. It owns a text buffer, so `node_append`, `node_delete`, selection and dim work unchanged.
- `is_box(n)`: band child with `style.border`. `is_text(n)`: band child that is not a box.

### Editor

- `EVENT_ADD_BAND` keeps its name (renaming touches 75 references) and gets a comment: it is the `a` key. It dispatches on the selection:
  - nothing selected: `add_band` (AC5)
  - band selected: `add_box` (AC1, AC3)
  - box or text selected: no-op (AC4)
- `add_box` appends a bordered node to the selected band, with no child cap, selects it and leaves the mode alone (`MODE_MOVE`).
- `add_text` (`o`) is unchanged, including its 2-child cap and tests.
- `EVENT_TOGGLE_DIM` on a band still dims every child, boxes included.

### Layout

- `count` and `index` in `text_box` cover non-box children only, so a lone "Login" stays centered and keeps its column (AC2). The existing two-text placement is untouched.
- Boxes are placed left to right after the right edge of the last non-box child, with a 1-column gap between children. A box is `len + 2` columns by 3 rows.
- `band->box.rows = max(2 * pad + 1, tallest child)`. Children share the band's middle row, `mid = top + (rows - 1) / 2`, so text and box centers line up. The next band's `top` still advances by `band->box.rows`.

### Painting

- A box paints its border around its text buffer. The detailed ops are decided in the slice that draws it.

### Known limitations

- Once a band is selected, `a` adds a box, so no key adds a second band until a follow-up spec adds one. This is intended, not a bug.
- A selected box can't move to a neighbour with `h`/`l`, because those use `is_text`. Step-out (backspace) is the way back to the band.
- Typing into a box does nothing yet, because `EVENT_CHAR` is gated on a selected band and `i` is not extended to boxes.
- Adding a box moves "Login" down one row when the band grows, because the band's `top` is fixed.

### Slices

1. `a` on a selected band appends a bordered child and selects it. `a` on a box or text does nothing. `a` with nothing selected still adds a band.
2. Layout: boxes placed after the last text with a gap, a lone text stays centered, and band height grows to the tallest child.
3. Paint the border.
