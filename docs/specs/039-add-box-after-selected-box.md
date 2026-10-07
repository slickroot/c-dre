# add-box-after-selected-box

Doug has a row with a bordered box on it, and he has selected that box. He presses `o`. A new empty box with a border appears right after it, in the same row, and it is now the selected one. He presses `o` once more and another box follows. Happy that he can build up a row of boxes, he carries on with his diagram.

## Acceptance Criteria

1. While a box is selected, pressing `o` adds an empty box with a border right after it, in the same row. The new box becomes the selected one, and Doug stays in move mode.
2. If the selected box has boxes after it, the new box goes between them. With boxes A, B and C and B selected, the row reads A, B, new, C. C moves one box to the right, and nothing is replaced or lost.
3. While a text or nothing is selected, `o` does nothing. No box or text is added.
4. While a row is selected, `o` still adds a second text, as it does today.

## Technical Design

### Node

- New `void node_insert_after(struct node *n, struct node *new_node)` in `node.c`, next to `node_append`.
- It takes no `parent` argument. It uses `n->parent`, so a caller can't pass a parent that disagrees with `n`. It assumes `n` has a parent, as `node_append` does. The editor never inserts after the root.
- It sets `new_node->parent`, `prev` and `next`, then points `n->next` at `new_node`.
- If `n->next` exists, that node's `prev` becomes `new_node`. Otherwise `parent->last_child` becomes `new_node`. `first_child` never changes.

### Editor

- Pull the shared lines of `add_box` into `new_box(e)`: `new_text`, then `border = 1`. It returns the bordered node or NULL.
- `add_box` becomes `new_box` + `node_append(e->selected, box)` + select the box. Behaviour is unchanged.
- New `add_box_after` is `new_box` + `node_insert_after(e->selected, box)` + select the box. It does not touch `e->mode`, so Doug stays in move mode (AC1). This differs from `add_text`, which sets `MODE_TYPE`.
- On allocation failure `add_box_after` returns with nothing changed, as `add_box` does.
- Dispatch in `editor_apply`:

```c
case EVENT_ADD_TEXT:
	if (is_band(e, e->selected))
		add_text(e);
	else if (is_box(e, e->selected))
		add_box_after(e);
	break;
```

- `is_box` is false for a text and for a NULL selection, so AC3 needs no extra code. The band branch is unchanged, so AC4 holds.
- `input.c` is unchanged. `o` already maps to `EVENT_ADD_TEXT` in move mode.

### Layout and display

- No change. Layout places children in child order, in a row and in a stacked band (038).
- "Right after" in the criteria means the next child in the band. In a stacked band the new box therefore goes below the selected one.

### Tests

- `test_node.c`: insert after the last child (`last_child` updates), insert in the middle (A, B, new, C, with all four links checked both ways), insert after a lone child.
- `test_editor.c`:
  - AC1: `o` on a lone box adds a bordered, empty, selected box after it, and the mode is still move.
  - AC1: pressing `o` twice builds a row of three boxes.
  - AC2: with A, B, C and B selected, the order is A, B, new, C, and nothing is lost.
  - AC2: `o` on the last box makes the new box `last_child`.
  - AC3: `o` on a text and with nothing selected adds no node.
  - AC4: existing band tests already cover it.

### Slices

1. `node_insert_after` with its `test_node.c` tests.
2. `new_box` refactor with no behaviour change, then `o` on a selected box via `add_box_after` (AC1, AC2, AC3). AC4 stays covered by the existing tests.
