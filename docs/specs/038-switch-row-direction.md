# switch-row-direction

Doug has a row with "Login" and two boxes, side by side. He selects the row and presses `r`. The children now stack top to bottom, in the same order. The row grows taller to hold them, and the rows below move down. He presses `r` again and they go back side by side. The row shrinks back to fit its tallest child. Happy that he can lay out a row both ways, he carries on with his diagram.

## Acceptance Criteria

1. While a row is selected, pressing `r` switches its children from side by side to stacked top to bottom. Pressing `r` again switches them back.
2. Texts like "Login" are children too. They keep their place in the row's order, and are stacked or placed side by side along with the boxes.
3. When stacked, the row grows tall enough to hold all its children, and the rows below move down to make room. When switched back, the row shrinks to fit the tallest child, as in 037.
4. The row stays selected after `r`. Pressing `r` again switches back, and pressing `a` adds another box.
5. In the stacked layout, a new box from `a` goes after the last child, at the bottom.
6. While a box, a text or nothing is selected, `r` does nothing.

## Technical Design

### Model

- `node_data` gets `int vertical;` next to `pad`. It is read on bands only. `node_new` zeroes it and `test_node.c` asserts that, as it does for `pad`.
- It is an int flag, like `style.dim` and `style.border`. Two states with a toggle don't need an enum.
- Geometry stays computed by `layout()`, never stored.

### Input and editor

- `r` maps to `EVENT_SWITCH_DIRECTION` in the `MODE_MOVE` branch of `input_parse`. In type mode it stays a plain character.
- `editor_apply` flips `vertical` only when `is_band(e, e->selected)`. Every other case falls through (AC6). Selection and mode do not change (AC4).
- `add_box` is unchanged. It appends after the last child, so the column layout puts the new box at the bottom (AC5).

### Layout

- The band loop in `layout()` picks a helper by `band->data.vertical`. Each helper takes the band and its `top` and returns the band's height.
  - `layout_row`: today's code, untouched.
  - `layout_column`: new.
- `layout_column` makes a single pass in child order, so texts and boxes keep their place (AC2). It does not rely on the "texts first, then boxes" order of the row layout.
- A text is 1 row and a box is `BOX_ROWS`. Children are separated by `CHILD_GAP` blank rows.
- Band height is `max(2 * pad + 1, stack height)`, so `pad` acts as a minimum height when stacked. The next band's `top` advances by that height (AC3).
- The stack is centered vertically: the first child starts at `top + (band_rows - stack_height) / 2`. A lone child in a padded band stays on the mid row, so `r` does not move it.
- Every child, boxes included, is centered horizontally, the way `text_box` already centers stacked texts. A centered "Login" does not jump sideways when toggled.
- The two-text above/below rule driven by `pad` applies to the row layout only.

### Unchanged

- `j`/`k` and `h`/`l` decide "stacked" or "beside" by comparing `box.row`, so they follow the new layout.
- The clip check in `display.c` uses `band->box.row + pad`, so a tall stacked band is clipped by the existing rect logic.

### Known limitations

- A stacked band with a text that has a stacked neighbour still navigates with `j`/`k` for texts only. Boxes are not reachable with `j`/`k`, as in 037.

### Tests

- `test_input.c`: `r` gives `EVENT_SWITCH_DIRECTION` in move mode and `EVENT_CHAR` in type mode.
- `test_editor.c`: `r` toggles on a selected band and toggles back, the band stays selected, `a` then adds a box, and `r` does nothing with a box, a text or nothing selected.
- `test_layout.c`: stacked in child order (texts and boxes mixed), band height with the gap, the `pad` minimum, a lone padded child staying on the mid row, the next band's `box.row` moving down, and switching back restoring the row geometry.
- `test_display.c`: one tall stacked band clipped at the bottom of the screen.

### Slices

1. `vertical` flag, `r` key and editor toggle, with the `r` no-op cases (AC1 flag, AC4, AC6).
2. Layout: split into `layout_row` and `layout_column` with no behaviour change, then stack children in child order with the gap, centered, and grow the band so the rows below move down (AC1, AC2, AC3, AC5).
3. Display test for a tall stacked band at the screen edge.
