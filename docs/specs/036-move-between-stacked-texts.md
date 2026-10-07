# move-between-stacked-texts

Doug has a tall row on his diagram with "Login" on top and "Logout" underneath, and he has stepped into the row with `Enter`, so "Login" is lit up on its own. He presses `j` and the highlight moves down to "Logout". He presses `k` and it goes back up to "Login". Happy that he can reach either text without leaving the row, he carries on editing his diagram.

## Acceptance Criteria

1. While the top text of a stacked row is selected, pressing `j` moves the highlight to the bottom text.
2. While the bottom text is selected, pressing `k` moves the highlight back to the top text.
3. On the top text `k` does nothing, and on the bottom text `j` does nothing. The highlight does not wrap around, and the selection stays on the same text.
4. In a row with one text, `j` and `k` do nothing.
5. When the two texts sit side by side in a one-line row, `j` and `k` do nothing. Doug uses `h` and `l` there.
6. While a whole row is selected, `j` and `k` keep moving between rows as they do today.
7. While Doug is typing into a text, `j` and `k` still insert the letters.

## Technical Design

**Builds on 033 and 034.** `e->selected` can already point at a text node,
`display.c` already highlights it, and 034 moves between side-by-side texts.
This spec adds vertical movement between stacked texts, and refactors 034's
row test into shared helpers.

### Reuse the existing events

`j` and `k` already parse to `EVENT_SELECT_DOWN` and `EVENT_SELECT_UP` in
`MODE_MOVE`, and to `EVENT_CHAR` in `MODE_TYPE`.

- `input.[ch]` does not change. The parser stays a stateless byte-to-event map.
- The editor decides by the type of the selected node, as `EVENT_TOGGLE_DIM`
  already does. A band moves between bands (AC6, unchanged). A text moves
  between its sibling texts.
- AC7 holds with no work: in `MODE_TYPE` `j` and `k` are still `EVENT_CHAR`.

### Stacked or side by side

`layout.c` is the only place that decides the arrangement:

- One text: no sibling.
- Two texts, `pad >= 1`: stacked, with different `box.row` values (`row-1` and
  `row+1`).
- Two texts, `pad == 0`: side by side, with equal `box.row` values.

The editor reads `box.row` as of the last `layout()`. `main` runs `layout`
after every `editor_apply`, so the values are current on the next key.

### `editor.c`

Two named helpers over one predicate replace the inline `box.row ==` checks:

```c
static int same_row(const struct node *a, const struct node *b)
{
	return a->box.row == b->box.row;
}

/* Sibling shown beside the text, in the same row; NULL if none. */
static struct node *beside(const struct node *text, struct node *sibling)
{
	return sibling && same_row(text, sibling) ? sibling : NULL;
}

/* Sibling shown above or below the text, in another row; NULL if none. */
static struct node *stacked_with(const struct node *text, struct node *sibling)
{
	return sibling && !same_row(text, sibling) ? sibling : NULL;
}
```

- The helpers dereference `box`, so every handler checks `is_text` first.
- `prev` and `next` are linear, so nothing wraps and the selection never leaves
  the row (AC3, AC4).

034 handlers, behaviour unchanged, `MODE_MOVE` guard kept:

```c
case EVENT_SELECT_PREV_TEXT:
	if (e->mode == MODE_MOVE && is_text(e, e->selected) &&
	    beside(e->selected, e->selected->prev))
		e->selected = e->selected->prev;
	break;
```

`EVENT_SELECT_NEXT_TEXT` mirrors it with `next`.

New branches (AC1, AC2, AC5):

```c
case EVENT_SELECT_UP:
	if (is_band(e, e->selected) && e->selected->prev)
		e->selected = e->selected->prev;
	else if (is_text(e, e->selected) &&
		 stacked_with(e->selected, e->selected->prev))
		e->selected = e->selected->prev;
	break;
```

`EVENT_SELECT_DOWN` mirrors it with `next`.

- Only `e->selected` changes. There is no new field and the mode does not
  change.
- The new branches have no `MODE_MOVE` guard, like the band branches beside
  them. A text can only be selected after `STEP_IN`, which requires
  `MODE_MOVE`, and `ENTER_TYPE` requires a band.

### Components

- `editor.c`: gains `same_row`, `beside` and `stacked_with`, and the two text
  branches in `SELECT_UP` and `SELECT_DOWN`. 034's handlers use the helpers.
- `input.[ch]`, `display`, `layout`, `paint`, `node`, `grid`, `main`:
  untouched.

### Testing

Slices, each built with TDD. Do the refactor first, then the feature, as
separate commits.

1. **Refactor (no new behaviour):** switch the 034 handlers to `beside`. The
   existing 034 tests in `test_editor.c` stay green.
2. **`test_editor`**, at screen level through `frame(e)`, calling `layout`
   after each `editor_apply` as the 034 tests do:
   - AC1: in a stacked row with the top text selected, `j` highlights only the
     bottom text.
   - AC2: with the bottom text selected, `k` highlights only the top text.
   - AC3: `k` on the top text and `j` on the bottom text leave the screen
     unchanged, with no wrap.
   - AC4: in a one-text row, `j` and `k` leave the screen unchanged.
   - AC5: in a side-by-side row (`pad == 0`, two texts), `j` and `k` leave the
     screen unchanged.
   - AC6: the existing band-level `j` and `k` tests stay green.
   - After `j` and `k` on texts, `Backspace` returns to the highlight of the
     same row.
3. **AC7:** `test_input.c` already pins `j` and `k` as `EVENT_CHAR` in
   `MODE_TYPE`. Add a `j` and `k` assertion there only if it is missing. Add
   one `test_editor` case that types `j` and `k` into a text of a stacked row.
