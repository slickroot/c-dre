# linked-children

## Refactoring Goal

Spec 030 gives the editor a node tree whose children are an array of
pointers: `children`, `count`, `cap`, grown by doubling `realloc`, shrunk
by `memmove`, with `node_index()` to find a node's neighbours. That array
costs more than it saves:

- `node_append` can fail (the `realloc`), so the editor carries cleanup
  rules for a half-built band in `EVENT_ADD_BAND`/`EVENT_ADD_TEXT`.
- `node_remove` shifts indices, so the delete heir must be picked first.
- `j`/`k` and the heir need `node_index()`, a linear search, just to reach
  the row above or below.
- `count` exists only for the temporary two-text cap.

With a handful of rows and at most two texts per row, the array's locality
and `children[i]` access buy nothing. This spec replaces it with linked
siblings: the classic left-child/right-sibling tree with a tail pointer,
the same shape as the DOM (`firstChild`, `lastChild`, `previousSibling`,
`nextSibling`) or BSD `TAILQ`. Stories 026–028 then move between texts
with the same `prev`/`next` they use between rows.

No visible behaviour change. `tests/test_editor.c` is the safety net and
stays untouched. `tests/test_node.c` is rewritten for the new links.

## Acceptance Criteria

1. `struct node` has `parent`, `first_child`, `last_child`, `prev`, `next`
   and `data`. It has no `children`, `count` or `cap`.
2. `node.h` exposes only `node_new`, `node_free`, `node_append` and
   `node_delete`. `node_index` and `node_remove` are gone.
3. `node_append` returns `void` and cannot fail.
4. `editor.c` uses `prev`/`next` for `j`/`k` and the delete heir, and
   `last_child` for the active text. Nothing indexes children.
5. `layout.h`, `paint`, `grid`, `term`, `input`, `text_buffer` and
   `main.c` are untouched.
6. `tests/test_editor.c` is untouched and stays green, along with all other
   suites. No leaks under ASan / `leaks --atExit`.
7. Visible behaviour is unchanged.

## Technical Design

### `node.[ch]`

```c
struct node {
	struct node_data data;
	struct node *parent;
	struct node *first_child, *last_child;   /* screen order: top/left first */
	struct node *prev, *next;                /* siblings */
};

struct node *node_new(void);
void node_free(struct node *n);
void node_append(struct node *parent, struct node *child);
void node_delete(struct node *n);
```

`struct node_data` is unchanged from 030.

- **No counter, no capacity, no index.** The tree only does new, free,
  append and delete. Neighbours are `prev`/`next`, so nothing needs a
  position. Nodes never move, so `parent` and `e->selected` never dangle.
- `node_new()` callocs a node: zeroed data (`text.data == NULL`), every
  pointer `NULL`. It returns `NULL` on failure. This is the only
  allocation in `node.c`.
- `node_append(parent, child)` links `child` after `parent->last_child`
  (and makes it `first_child` too if `parent` was empty), and sets
  `child->parent`. It runs in constant time and cannot fail.
- `node_delete(n)` replaces 030's `node_remove`. It unlinks `n` from its
  siblings, fixes the parent's `first_child`/`last_child` when `n` was at
  an end, and then calls `node_free(n)`. If `n` has no parent, it just
  frees.
- `node_free(n)` frees the subtree: each child (`first_child` → `next`)
  recursively, then `text_buffer_free(&n->data.text)`, then `n`. There is
  no children array to free. It reads `next` before freeing each child.

### `editor.c`

| event | 030 (array) | 031 (linked) |
|---|---|---|
| `EVENT_ADD_BAND` | append can fail: free the half-built band | only `node_new` can fail; free what was created before appending |
| `EVENT_ADD_TEXT` | `selected->count < 2` | `selected->first_child == selected->last_child` (one text) |
| `EVENT_SELECT_UP` (`k`) | `i = node_index(selected)`; `children[i - 1]` | `selected->prev`, if any |
| `EVENT_SELECT_DOWN` (`j`) | `children[i + 1]` | `selected->next`, if any |
| `EVENT_DELETE_BAND` | heir by index, picked before removal shifts indices | heir = `prev`, else `next`, else `NULL`; read before `node_delete` (its pointers are freed with it) |
| `active_text(band)` | `children[count - 1]` | `band->last_child` |

`prev` is the row above and `next` the row below, matching the screen.

`layout()` walks bands with `root->first_child` → `next`, and each band's
texts with `band->first_child` → `next`. Everything else in it is
unchanged.

### Components

- `node.[ch]`. Knows parent, first/last child, prev/next sibling, and the
  node's data. Does new/free/append/delete. Collaborates with
  `text_buffer` (free only) and `layout.h` (`struct style`).
- `editor.[ch]`. Unchanged role: gives the tree its meaning and runs the
  events, now through `prev`/`next`/`first_child`/`last_child`.

### Testing

`tests/test_node.c` is rewritten, in `assert.h` style:
- `node_new`: every pointer `NULL`, zeroed data.
- Append to an empty parent: `first_child == last_child == child`, no
  siblings, `parent` set.
- Append several: the same order walking `first_child` → `next` and
  `last_child` → `prev`. The ends have `NULL` `prev`/`next`.
- Delete first, middle and last: the order holds both ways and
  `first_child`/`last_child` are fixed up.
- Delete the only child: the parent has
  `first_child == last_child == NULL`.
- Delete a node with children frees the whole subtree (ASan/`leaks`).
- Free a tree with some initialised text buffers: no leaks, no crash.

`tests/test_editor.c` is **not edited**. All of it passing is the proof.

## Out of scope

- Selecting a text, stepping into a row, moving between texts (026, 027).
- Per-text dim (028).
- Removing the two-text cap.
- `pad` in the style and renaming `node_data`.
