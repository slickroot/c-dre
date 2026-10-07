# dim-one-text

Doug has a row on his diagram with "Login" on the left and "Logout" on the right, and he has stepped into the row so "Login" is lit up on its own. "Login" matters less, so he presses `-` and it turns grey while "Logout" stays as it was. He changes his mind, presses `-` again, and "Login" is back to normal. Happy that he can quiet down just one text, he carries on editing.

## Acceptance Criteria

1. While a text is selected, pressing `-` flips how that text looks now. A normal text turns grey and a grey text turns normal. This applies whether the grey came from the row or from the text.
2. Only the selected text changes. Its sibling and the rest of the diagram keep their look.
3. While a whole row is selected, pressing `-` flips each text in it from what it looks like now. In a row with a normal "Login" and a grey "Logout", "Login" goes grey and "Logout" goes normal.
4. A text added with `o` always starts normal, even in a row whose other texts are grey.
5. A text keeps its look while Doug types into it, so a grey text stays grey.

## Technical Design

The text is the single source of truth for how it looks. The pipeline stays
`input_parse` → `editor_apply` → `layout()` → `display_list`. No new files, no
new events, no new structs.

### The text owns its dim

`struct node` is shared by bands and texts, so `node->data.style.dim` already
exists on every text and `node_new` zeroes it. Nothing is added to `node.h`.

The band's own `data.style.dim` stays on the struct but nothing reads or writes
it any more. We do not split band data from text data; that would be a refactor
beyond this story. There is no "clear the band flag" rule, because the band flag
is gone from the look.

### `display.c` decides the colour from the text

At `display.c:54` the colour comes from the text, not the band:

```c
text->data.style.dim ? dim : ink
```

This is the only place the colour is decided.

### `EVENT_TOGGLE_DIM` handles two cases (`editor.c:164`)

One small static helper does the flip:

```c
static void toggle_dim(struct node *text)
{
	text->data.style.dim = !text->data.style.dim;
}
```

- **Text selected** (`is_text`): `toggle_dim(e->selected)`. AC1, AC2.
- **Band selected** (`is_band`): loop `first_child` → `next` and `toggle_dim`
  each child. Each text flips from its own current state, so a normal "Login"
  and a grey "Logout" swap. AC3.
- Anything else (nothing selected): no-op.

No mode check. `input_parse` only emits `EVENT_TOGGLE_DIM` in MODE_MOVE, and
`EVENT_STEP_IN` is already limited to MODE_MOVE, so a text can only be selected
there.

### AC4 and AC5 need no code

- AC4: `new_text` calls `node_new`, which zeroes the style, and `add_text` never
  copies from the band. A text added with `o` in a grey row starts normal.
- AC5: typing only touches `data.text`, never `data.style`.

### Existing tests that change

A band's `dim` no longer affects rendering, so these break and are rewritten:

- `tests/test_node.c:209` sets `band->data.style.dim = 1` directly.
- `test_dim_band_texts_are_dim` (`tests/test_display.c:235`)
- `test_typing_into_dim_band_keeps_dim` (`tests/test_editor.c:606`)
- `test_add_band_after_dim_is_normal_and_old_stays_dim`
  (`tests/test_editor.c:586`)

Where they set the field, they go through the `-` key instead.
`test_toggle_makes_band_dim_then_normal` and
`test_toggle_dim_twice_returns_to_ink` should pass as they are.

Behaviour change to expect: a text added with `o` to a grey row used to look grey
because the band flag applied to it. It now starts normal (AC4). Any test
asserting the old behaviour must be updated.

### New tests

- Editor: `-` on a selected text greys only that text; its sibling stays normal;
  `-` again restores it (AC1, AC2).
- Editor: `-` on a selected text whose row was greyed with `-` on the band flips
  just that text (AC1, "from the row").
- Editor: `-` on a band with a normal and a grey text swaps them (AC3).
- Editor: `o` in a row of grey texts adds a normal text (AC4).
- Editor: typing into a grey text keeps it grey (AC5).
