# node-tree

## Refactoring Goal

The editor stores a diagram as a flat, backwards linked list of
`struct band`. `e->bands` is the **newest** band, at the **bottom** of the
screen. `next` points to the older band *above* it, so `k` follows `next`
and `layout()` walks to the tail and back via `prev`. Each band holds its
texts in a fixed `texts[2]` + `count`.

Stories 026–028 need a selection that can sit on a row *or* on a text,
movement between sibling texts like between sibling rows, and later a
style on each text. All three are the same thing one level down, so the
model should be a tree:

```
canvas (root)
├── band
│   ├── text
│   └── text
└── band
    └── text
```

A node is what its position says: the root is the canvas, the root's
children are bands, and a band's children are texts. There is no `kind`
field.

No visible behaviour change. `tests/test_editor.c` (black-box, through
`layout()`) plus `test_grid`/`test_paint` from 029 are the safety net, and
they stay untouched.

## Acceptance Criteria

1. A new `node.[ch]` module holds a generic tree: a uniform `struct node`
   with a parent and an array of children in screen order. It knows nothing
   about bands, texts, selection or the screen.
2. `struct band` is gone. `struct editor` holds `root` (the canvas node) and
   `selected` (a band node or `NULL`).
3. Bands are the root's children, top row first. Texts are a band's
   children, left/top text first.
4. Only text nodes have an initialised text buffer.
5. `layout.h`, `paint`, `grid`, `term`, `input` and `main.c` are untouched.
6. `tests/test_node.c` exists and runs in `make test`. `tests/test_editor.c`
   is untouched and stays green, along with all other suites.
7. Visible behaviour is unchanged.

## Technical Design

### `node.[ch]`: a generic tree

```c
#include "layout.h"
#include "text_buffer.h"

struct node_data {
	struct text_buffer text;
	struct style style;   /* only dim is used; highlight is never stored */
	int pad;
};

struct node {
	struct node_data data;
	struct node *parent;
	struct node **children;   /* screen order: top/left first */
	int count;
	int cap;
};

struct node *node_new(void);
void node_free(struct node *n);
struct node *node_append(struct node *parent, struct node *child);
void node_remove(struct node *n);
int node_index(const struct node *n);
```

- **Children are an array of pointers**, not values. Growing it with
  `realloc` moves only the pointer array, and the nodes stay put, so
  `parent` and `e->selected` never dangle.
- `node_new()` callocs a node: empty data, so `text.data == NULL` and
  `cap == 0`, `style` zero, `pad` 0, no parent, no children. It returns
  `NULL` on failure.
- `node_append(parent, child)` grows `parent->children` when
  `count == cap` (doubling, starting at 4), puts `child` at the end and
  sets `child->parent`. It returns `child`, or `NULL` if the `realloc`
  fails (the tree is then unchanged and the caller still owns `child`).
- `node_remove(n)` takes `n` out of its parent's array (`memmove` the
  pointers after it, `count--`) and then calls `node_free(n)`. If `n` has
  no parent, it just frees.
- `node_free(n)` frees the subtree: each child recursively, then
  `text_buffer_free(&n->data.text)` (a safe `free(NULL)` on non-text
  nodes), the children array, and `n`. `node.c` never asks what kind of
  node it holds.
- `node_index(n)` is `n`'s position in `n->parent->children`, found by a
  linear search. n is the number of rows, so this is trivial. It returns -1
  for the root.

Deferred on purpose: `pad` moving into the style, and renaming the
`node_data` wrapper (the characters are at `n->data.text.data`).

### `editor.c`: meaning on top of the tree

```c
struct editor {
	struct node *root;       /* the canvas */
	struct node *selected;   /* a band, or NULL */
	enum app_mode mode;
	int cols;
	int rows;
};
```

- `editor_new` creates `root` with `node_new()`, and frees and returns
  `NULL` if that fails. `editor_free` is `node_free(e->root)` plus the
  editor.
- `new_text(e)` (static): `node_new()` + `text_buffer_init(&t->data.text,
  e->cols - 1)`. This is the only place a buffer gets initialised.
- `active_text(band)` (static) is `band->children[band->count - 1]`, the
  same rule as today's `texts[count - 1]`.

| event | today | with the tree |
|---|---|---|
| `EVENT_ADD_BAND` | prepend band at head, select, `MODE_TYPE` | `node_append(root, band)`, `node_append(band, new_text(e))`, select band, `MODE_TYPE` |
| `EVENT_ADD_TEXT` | if `count < 2`: init `texts[1]`, `MODE_TYPE` | if `selected && selected->count < 2`: `node_append(selected, new_text(e))`, `MODE_TYPE` |
| `EVENT_SELECT_UP` (`k`) | `selected = selected->next` | `i = node_index(selected)`; if `i > 0`, select `root->children[i - 1]` |
| `EVENT_SELECT_DOWN` (`j`) | `selected = selected->prev` | if `i < root->count - 1`, select `root->children[i + 1]` |
| `EVENT_CHAR` / `EVENT_BACKSPACE` | edit `active_text` | edit `active_text(selected)`'s buffer |
| `EVENT_DELETE_BAND` | heir = `next` (above) else `prev` (below) | heir = `children[i - 1]` (above) if `i > 0`, else `children[i + 1]` (below) if it exists, else `NULL`. Then `node_remove(selected)` and select the heir. |
| `EVENT_TOGGLE_DIM` | `selected->style.dim` flips | `selected->data.style.dim` flips (still on the band) |
| `EVENT_GROW_BAND` / `EVENT_SHRINK_BAND` | `selected->pad` ± | `selected->data.pad` ±, floor 0 |
| `EVENT_ESCAPE`, `EVENT_ENTER_TYPE`, `EVENT_QUIT`, `EVENT_NONE` | unchanged | unchanged |

The heir is picked **before** `node_remove`, because removal shifts the
indices. If an allocation fails in `EVENT_ADD_BAND`/`EVENT_ADD_TEXT`, the
event is a no-op and the nodes created so far are freed. Mode and selection
stay as they were.

`layout()` walks `root->children[0 .. count)`, top row first, with no
detour to the tail. Everything else is today's logic with the tree's field
names: `row = top + pad`, clip at `visible`, copy `style.dim`, derive
`highlight` (`selected && MODE_MOVE`), place each of the band's
`count` text children with `place_text`, and put the caret on
`active_text` when the band is selected in `MODE_TYPE`.

### `Makefile`

- `SRCS` gains `node.c`.
- `test_editor` now also links `node.c`.
- New `$(BUILD_DIR)/test_node` from `tests/test_node.c node.c text_buffer.c`,
  added to `TEST_BINS` and run by `make test`. New files follow kernel style,
  and `make lint` stays green.

### Components

- `node.[ch]` (new). Knows parent, children (screen order), and the node's
  data. Does new/free/append/remove/index. Collaborates with `text_buffer`
  (free only) and `layout.h` (`struct style`).
- `editor.[ch]`. Knows `root`, `selected`, mode and screen size. Gives the
  tree its meaning (canvas → bands → texts), runs the event table, and
  produces `layout()`. Collaborates with `node` and `text_buffer`.
- `layout.h`, `paint`, `grid`, `term`, `input`, `text_buffer`, `main.c`:
  untouched.

### Testing

`tests/test_node.c`, in `assert.h` style:
- `node_new`: no parent, no children, zeroed data (`text.data == NULL`).
- Append sets `parent` and keeps order. Appending past the initial `cap`
  (e.g. 10 children) keeps every child, in order.
- `node_index` is right for first, middle and last, and -1 for a root.
- Remove first, middle and last: the remaining children keep their order,
  `count` drops, and the indices shift.
- Remove a node with children frees the whole subtree (checked with
  ASan/`leaks`).
- Free a tree with initialised text buffers and some without: no leaks, no
  crash.

`tests/test_editor.c` is **not edited**. All 64 tests passing is the proof
that the tree reproduces today's behaviour: order, selection with `j`/`k`,
the delete heir, the two-text cap, pad, dim, highlight, caret, clipping.
`test_grid`, `test_paint`, `test_input` and `test_text_buffer` stay green.
No leaks under ASan / `leaks --atExit`.

## Out of scope

- Selecting a text, stepping into a row, and moving between texts (026, 027).
- Per-text dim (028).
- `pad` in the style, and renaming `node_data` / flattening the node.
- Removing the two-text cap.
