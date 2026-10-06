# static-insert-delete-node

## Refactoring Goal

`EVENT_DELETE_BAND` already calls a static helper, `delete_selected()`, that
owns the whole delete. `EVENT_ADD_BAND` never got the same treatment — it's
still 15 lines inline in the `switch`. One case is a static function, the
other is inline; settle on static for both so the two mirror-image
operations (grow the list, shrink the list) read the same way at the call
site.

## Technical Design

Still one `main.c` plus `input`, `text_buffer` and `paint`. No new module, no
new file, no change to `struct band`, `text_buffer.[ch]` or `paint.[ch]`. The
work is renaming/reshaping two static helpers in `main.c` and shrinking both
switch cases to one line each.

### `delete_selected()` becomes `delete_node(struct band *node)`

Same body as today, but the node to delete is a parameter instead of an
implicit read of the global `selected`. The call site passes `selected`
explicitly:

```c
case EVENT_DELETE_BAND:
  delete_node(selected);
  break;
```

Inside, every reference to `selected` (the NULL guard, computing `heir` from
`victim->next`/`victim->prev`) is renamed to `node`. The function still
reseats the global `selected = heir` itself before returning — the caller
only says *which* node to remove, not what becomes selected afterward. Mode
is untouched here: deleting never changes `mode`, so the switch case stays a
single call with no second line.

### The inline add block becomes `insert_node(void)`

Mirrors `delete_node` taking ownership of `free`: `insert_node()` takes
ownership of `malloc`. No parameters — there's no node to point to until the
function creates one. It mallocs the `struct band`, sets `row`, calls
`text_buffer_init`, links it onto the front of `bands`, and — mirroring how
`delete_node` reseats `selected` to the heir internally — sets the global
`selected` to the new node itself, then paints the label:

```c
static void insert_node(void) {
  struct band *b = malloc(sizeof *b);
  if (!b)
    return;

  b->row = bands ? bands->row + 1 : 1;
  text_buffer_init(&b->buf, cols);
  b->next = bands;
  b->prev = NULL;
  if (bands)
    bands->prev = b;
  bands = b;
  selected = b;

  paint_label(&b->buf, cols, b->row);
}
```

`insert_node()` returns `void` and never touches `mode` — that stays a
`main()` concern, same as it is today. The switch case keeps the one line
`delete_node` doesn't need:

```c
case EVENT_ADD_BAND:
  insert_node();
  mode = MODE_TYPE;
  break;
```

### Why mode stays in the switch, not the helper

`mode` is a local in `main()`, not a global like `bands`/`selected`/`cols`.
Pushing it into `insert_node()` would mean passing `enum app_mode *mode` in,
which `delete_node` has no equivalent need for (deleting never changes mode).
Keeping `mode = MODE_TYPE;` in the switch case keeps both helpers symmetric:
each owns exactly the list/buffer/paint side effects of its operation, and
`main()` keeps owning `mode`.

### Components

- `main.c` `insert_node()` — renamed/extracted from the inline
  `EVENT_ADD_BAND` block; owns `malloc`, link, `selected` reseat, paint;
  `void`, no params
- `main.c` `delete_node(struct band *node)` — renamed from
  `delete_selected()`; same body, `node` replaces reads of global `selected`
  as the thing being removed; still reseats `selected = heir` itself
- `main.c` `switch` — `EVENT_ADD_BAND` becomes `insert_node(); mode =
  MODE_TYPE;`; `EVENT_DELETE_BAND` becomes `delete_node(selected);`
- `struct band`, `text_buffer.[ch]`, `paint.[ch]`, `input.[ch]` — untouched

### Testing

No unit-test harness for `main.c` (same call 011/012/013/014/015 made for
the band list and `paint.c`). Hand-judged end to end: press `a` three times,
confirm three bands appear and typing still works (`insert_node` + mode
switch unchanged behavior). Highlight the middle band, press `d`, confirm it
disappears and the band above becomes selected (`delete_node` unchanged
behavior). Highlight the topmost band, press `d`, confirm the band below
becomes selected. Delete the last remaining band, confirm the screen is
empty with no caret. Quit and confirm the terminal is restored.
