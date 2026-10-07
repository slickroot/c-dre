# move-between-side-by-side-texts

Doug has a row on his diagram with "Login" on the left and "Logout" on the right, and he has stepped into the row with `Enter`, so "Login" is lit up on its own. He presses `l` and the highlight moves to "Logout". He presses `h` and it goes back to "Login". Happy that he can reach either text without leaving the row, he carries on editing his diagram.

## Acceptance Criteria

1. While the left text of a side-by-side row is selected, pressing `l` moves the highlight to the right text.
2. While the right text is selected, pressing `h` moves the highlight back to the left text.
3. On the left text `h` does nothing, and on the right text `l` does nothing. The highlight does not wrap around.
4. In a row with one text, `h` and `l` do nothing.
5. `h` and `l` never leave the row.
6. When the two texts are stacked in a tall row, `h` and `l` do nothing.

## Technical Design

**Builds on 033 (step into row and back out).** `e->selected` can already
point at a text node, and `display.c` already highlights it. This spec adds
only horizontal movement between sibling texts.

### Neighbour rule

Two texts are side by side when they are siblings and `layout()` put them on
the same `box.row`. The editor does not look at `pad` or the text count, so
`layout.c` stays the only place that decides whether texts are stacked.

- `prev` and `next` are linear: NULL at the ends of a band, never circular
  (`node_append`, `node_delete`). Nothing wraps, and they never leave the band.
- A stacked row (`pad >= 1`) gives its two texts different `box.row` values,
  so the move does nothing there. No special case is needed.
- The editor reads `box.row` as of the last `layout()`. `main` runs
  `editor_apply` and then `layout` on every key, so the values are current
  when the next key arrives.

### `input.[ch]`

Two new events, parsed only in `MODE_MOVE`:

| Byte | Event                    |
|------|--------------------------|
| `h`  | `EVENT_SELECT_PREV_TEXT` |
| `l`  | `EVENT_SELECT_NEXT_TEXT` |

- In `MODE_TYPE`, `h` and `l` stay `EVENT_CHAR`, so typing still works.
- The parser stays a stateless byte-to-event map by mode. The editor decides
  whether a move is legal.
- The names avoid `MOVE_LEFT`/`MOVE_RIGHT`, which would clash with `MODE_MOVE`.

### `editor.c`

New helper, used only by the new handlers:

```c
static int is_text(const struct editor *e, const struct node *n)
{
	return n && is_band(e, n->parent);
}
```

```c
case EVENT_SELECT_NEXT_TEXT:
	if (e->mode == MODE_MOVE && is_text(e, e->selected) &&
	    e->selected->next &&
	    e->selected->next->box.row == e->selected->box.row)
		e->selected = e->selected->next;
	break;
```

`EVENT_SELECT_PREV_TEXT` mirrors it with `prev`.

- Only `e->selected` changes. There is no new field, and the mode does not change.
- The `MODE_MOVE` guard is defensive, since a selected text already implies
  `MODE_MOVE`. It matters for direct `editor_apply` calls.
- On a selected band, `is_text` is false, so `h` and `l` do nothing.
- The existing band-level handlers already guard on `is_band`, so they are
  untouched.
- `STEP_OUT` keeps its current `!is_band` check. Replacing it with `is_text`
  is a separate refactor and is out of scope here.

### Components

- `input.[ch]`: gains the two events.
- `editor.c`: gains `is_text` and the two handlers.
- `display`, `layout`, `paint`, `node`, `grid`, `main`: untouched. The 033
  text highlight already follows `selected`.

### Testing

Slices, each built with TDD:

1. **`test_input`:** `h` and `l` in `MODE_MOVE` give `EVENT_SELECT_PREV_TEXT`
   and `EVENT_SELECT_NEXT_TEXT`. In `MODE_TYPE` they give `EVENT_CHAR` with
   the byte.
2. **`test_editor`**, at screen level through `frame(e)`. Every test calls
   `layout` after each `editor_apply`, through a small helper, so `box.row` is
   current:
   - AC1: with the left text selected, `l` highlights only the right text.
   - AC2: with the right text selected, `h` highlights only the left text.
   - AC3: `h` on the left text and `l` on the right text leave the screen
     unchanged, with no wrap in either direction.
   - AC4: in a one-text row, `h` and `l` leave the screen unchanged.
   - AC5: after `h` and `l` the selection is still a text of the same row, and
     `Backspace` returns to that row's highlight.
   - AC6: in a tall row (`pad >= 1`) with two stacked texts, `h` and `l`
     leave the screen unchanged.
   - `h` and `l` on a selected band do nothing.
   - A direct `editor_apply(SELECT_NEXT_TEXT)` in `MODE_TYPE` does nothing.
   - Typing `h` and `l` in type mode still inserts characters.
