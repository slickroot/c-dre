## Story

Doug has a few bands on screen, having added them earlier with `a`. In move mode, he presses `k`, and the caret steps up into the band above; he presses `j` a couple of times, and the caret steps back down, band by band, stopping once it reaches the bottommost band. Each band remembers its own caret position, so when he lands back on one he'd typed in before, the caret sits right where he left it. He presses `i` and starts typing — the text lands in the band the caret was resting in, not necessarily the newest one. He presses `Esc`, moves the selection elsewhere with `j`/`k`, and presses `i` again to keep typing in a different band. When he presses `a`, a new band appears below the others and becomes selected, same as before.

## Acceptance Criteria

1. In move mode, pressing `k` moves the caret to the band directly above the currently selected one; pressing `j` moves it to the band directly below.
2. Pressing `k` while on the topmost band does nothing; pressing `j` while on the bottommost band does nothing.
3. The caret is visible only in the selected band, at that band's own last cursor position.
4. Pressing `i` enters typing mode and inserts characters into whichever band is currently selected.
5. Adding a band with `a` makes the new band the selected one.
6. `Ctrl-C` still exits the app and restores the terminal, from move or typing mode.

## Technical Design

Still one `main.c` plus `input`, `text_buffer` and `paint`. No new module, no
new file. The band list gains a `prev` pointer and `main.c` gains a second
static pointer; everything else reuses machinery already in place since 011.

### The list becomes doubly linked, and selection is a second pointer

`bands` is still the list head: the newest band, which by construction is also
the bottom-most band on screen (`top_row` grows by 3 with each `a`). It keeps
meaning "where does the next new band go" and "where does freeing start" — it
is never read for typing or selection anymore.

```c
struct band {
  struct text_buffer buf;
  int top_row;
  struct band *next;
  struct band *prev;
};

static struct band *bands;
static struct band *selected;
```

`next` points from a band toward older bands (higher up on screen); `prev`
points toward newer bands (lower down, back toward `bands`). `selected` is the
band the caret is in and the one typing events target. The two pointers are
deliberately independent: `bands` answers "what's newest", `selected` answers
"what's active", and `EVENT_ADD_BAND` is the only place that moves both.

### Adding a band links both pointers and selects the new node

```c
case EVENT_ADD_BAND: {
  struct band *b = malloc(sizeof *b);
  if (b) {
    b->top_row = bands ? bands->top_row + 3 : 1;
    text_buffer_init(&b->buf, cols);
    b->next = bands;
    b->prev = NULL;
    if (bands)
      bands->prev = b;
    bands = b;
    selected = b;
    mode = MODE_TYPE;
    paint_label(&b->buf, cols, b->top_row);
  }
  break;
}
```

This is 011's code plus the two lines that wire `prev` and the one line that
sets `selected = b`. AC5 falls out of that last line.

### `j`/`k` walk the list and reuse `paint_label` to move the caret

Two new events, parsed in move mode exactly where `a` and `i` are read today:

```c
if (byte == 'k')
  return (struct key_event){ EVENT_SELECT_UP, 0 };
if (byte == 'j')
  return (struct key_event){ EVENT_SELECT_DOWN, 0 };
```

`EVENT_SELECT_UP` (`k`, toward older/higher bands) follows `next`;
`EVENT_SELECT_DOWN` (`j`, toward newer/lower bands) follows `prev`. Both are
no-ops at the respective end of the list, which is AC2 stated as a guard:

```c
case EVENT_SELECT_UP:
  if (selected && selected->next)
    selected = selected->next;
  break;
case EVENT_SELECT_DOWN:
  if (selected && selected->prev)
    selected = selected->prev;
  break;
```

Neither case repaints. Instead, after the switch, the loop's existing
"repaint if something changed" step is extended to also repaint whenever
selection moved:

```c
if (changed || ev.type == EVENT_SELECT_UP || ev.type == EVENT_SELECT_DOWN)
  paint_label(&selected->buf, cols, selected->top_row);
```

`paint_label` already ends by positioning the hardware cursor at the band's
own `buf.cursor`, and a terminal has only one cursor — so repainting the newly
selected band is sufficient to both leave its text untouched (same bytes
re-emitted) and move the caret there, away from whichever band held it before.
This is the same trick 011 used when a new band takes the caret; no "active"
flag, no per-band visibility state, nothing added to `paint_label`'s signature.
AC1 and AC3 fall out of this one line.

### Typing targets `selected`, not `bands`

`EVENT_CHAR` and `EVENT_BACKSPACE` swap their buffer:

```c
case EVENT_CHAR:
  if (selected == NULL)
    continue;
  changed = text_buffer_insert(&selected->buf, ev.ch);
  break;
case EVENT_BACKSPACE:
  if (selected == NULL)
    continue;
  changed = text_buffer_backspace(&selected->buf);
  break;
```

The `bands == NULL` guard from 011 becomes `selected == NULL` — equivalent
today since the two pointers are only ever both-NULL or both-set, but it says
what the code actually depends on. This is AC4: text lands wherever `selected`
points, which after a `j`/`k` is not necessarily `bands`.

### Freeing is unchanged

The exit walk already follows `next` from `bands` down to `NULL`, which
visits every node regardless of `prev`; it needs no change. `selected` is not
freed separately — it always points at a node already owned by the `bands`
list.

### Components

- `struct band` — gains `struct band *prev`
- `static struct band *selected` — **new**, alongside the existing
  `static struct band *bands`
- `enum key_event_type` — gains `EVENT_SELECT_UP`, `EVENT_SELECT_DOWN`
- `input_parse()` — move mode reads `j`/`k`; signature unchanged
- `main()` — `EVENT_ADD_BAND` links `prev` and sets `selected`; two new
  `switch` cases walk `selected`; `EVENT_CHAR`/`EVENT_BACKSPACE` target
  `selected->buf`; the post-switch repaint also fires on selection change

`paint.[ch]` and `text_buffer.[ch]` are untouched.

### Testing

`tests/test_input.c` gets new cases for the parser, following the existing
pattern:

- `k` and `j` in `MODE_MOVE` yield `EVENT_SELECT_UP` / `EVENT_SELECT_DOWN`;
- `k` and `j` in `MODE_TYPE` are still printable ASCII, so they yield
  `EVENT_CHAR` with `ch == 'k'` / `ch == 'j'`, same as `i` already does.

The doubly linked list, the `bands`/`selected` split and the paint-driven
caret move live in `main.c` and have no unit test, by the same decision 011
made for the band list. They are hand-judged end to end: add three bands with
`a`, type into each in turn to tell them apart, press `Esc`, press `k` twice
and confirm the caret steps up through the second then the first band and
stops there on a third `k`; press `j` twice and confirm it steps back down and
stops at the bottom on a third `j`; press `i` on a middle band and type,
confirming the text lands there and the other bands are untouched; press `a`
and confirm the caret jumps to the new bottom band; Ctrl-C at any point and
confirm scrollback intact.
