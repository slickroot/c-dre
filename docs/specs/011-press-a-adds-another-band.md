## Story

Doug is in move mode, with a band already on screen. He presses `a`, and a new
empty band appears directly below the last one, with no gap between them. The
caret is waiting in the new band, so he starts typing straight away, and his
earlier band keeps its text, untouched. When he's done, he presses `Esc`, and
later presses `a` again to add yet another band below the last. What happens
once the bands reach the bottom of the screen is left for another day.

## Acceptance Criteria

1. While in move mode with at least one band already on screen, pressing `a`
   adds a new band directly below the last band, with no blank row between them.
2. The new band matches the existing bands: 3 rows tall, full terminal width,
   same grey background and same text colour.
3. The new band starts empty, and the visible caret sits in it, centred.
4. The next characters Doug types appear in the new band, centred; no earlier
   band changes.
5. Text already typed in earlier bands stays visible and unchanged.
6. Pressing `Esc` leaves typing mode; pressing `a` again adds another band
   directly below the last one.
7. Ctrl-C still exits the app and restores the terminal.
8. What happens when the bands fill the screen is out of scope for this story.

## Technical Design

The whole program is still one `main.c` plus the three modules 009 extracted
(`input`, `text_buffer`, `paint`). No new module, no new file. The multi-band
state is a flat linked list that lives in `main.c` alongside the loop.

### One list, newest first, and the head is the active band

Today `main()` owns a single `struct text_buffer buf` and a `band_drawn` latch.
Both go. In their place, `main.c` declares one node type:

```c
struct band {
  struct text_buffer buf;
  int top_row;
  struct band *next;
};
```

and one pointer, `static struct band *bands`, that is the newest band. The list
is singly linked, newest first, and the head is by construction the active band
for typing and for the caret. That is why there is no `active` index, no
`band_count`, and no tail pointer: `a` only ever appends, typing only ever
targets the newest band, and story 010's `i` resumes "the current band", which
is the head.

`top_row` is stored on the node rather than derived by walking the list. A new
band's first row is the previous head's first row plus three, or row 1 when the
list is empty:

```
top_row = bands ? bands->top_row + 3 : 1
```

This is the "no gap" of AC1 stated arithmetically, and it means painting a band
never has to know its position in the list — only its own `top_row`. It also
means insertion order and screen order are independent, which is what lets the
newest node be the head without any band's row moving.

### Adding a band

`a` in move mode is now unconditional: every press adds a band, first or tenth.
The parser no longer decides based on `band_drawn`, because `band_drawn` is
gone. The loop does the work when it sees `EVENT_ADD_BAND`:

```c
struct band *b = malloc(sizeof *b);
if (b) {
  b->top_row = bands ? bands->top_row + 3 : 1;
  text_buffer_init(&b->buf, cols);
  b->next = bands;
  bands = b;
  mode = MODE_TYPE;
  paint_label(&b->buf, cols, b->top_row);
}
```

A failed `malloc` is skipped: no band is added, the program stays in move mode,
and nothing is dereferenced. This mirrors the codebase's existing treatment of
`text_buffer_init`'s `malloc`, which is also unchecked.

The new band's buffer is initialized at `cols`, exactly as the single buffer was
at startup, so the full-width ceiling from 006 still holds per band. The
`a` keystroke is not stored as text, same as 006.

### Painting only the newest band, addressed by row

Only the newest band ever changes: it is created, then typed into. Older bands
are painted once and never touched again, and there is only one hardware cursor,
so painting the new band automatically takes the caret away from the one above.
That is why `paint_label()` does not need an "active" flag — every call it
receives *is* for the active band.

**Decision: `paint_label(const struct text_buffer *buf, int cols, int top_row)`.**
The body is 009's, with the two hardcoded rows made explicit:

- the band starts at `ESC[<top_row>;1H` instead of `ESC[1;1H`, then the same
  three `ESC[K` erases walk down rows `top_row` through `top_row + 2`;
- the text and caret are addressed on row `top_row + 1` instead of `2`, with the
  same `(cols - buf->len) / 2 + 1` centring and `+ buf->cursor` caret offset.

Everything else is byte-for-byte 009: the band colour `#3F3F46`, the text colour
`#C9C9CF`, `ESC[?25h`, the `ESC[0m` reset, and the `cols == 0` / `len >= cols`
early returns. The band's three rows and its text row are all the row arithmetic
this story adds.

AC5 falls out for free: an older band's bytes are never re-emitted, so nothing
can change them. AC4 is the `buf` argument being the head's buffer.

### The parser loses `band_drawn`

`input_parse()` no longer needs to know whether a band exists. In move mode `a`
always returns the add-band event; nothing else about the parser changes.

**Decision: `input_parse(struct input_parser *parser, char byte, enum app_mode mode)`.**
The fourth parameter is deleted, and `EVENT_SUMMON_BAND` is renamed
`EVENT_ADD_BAND` to say what it now does. Move mode keeps `i` →
`EVENT_ENTER_TYPE`; type mode keeps `Esc`, Backspace and printable characters
unchanged from 010.

`main.c` no longer declares `band_drawn`, and the `if (!band_drawn) continue;`
guard in the loop is replaced by a `bands == NULL` guard described next.

### Typing with no bands is ignored

Story 010 lets `i` enter typing mode before any band exists. With zero bands
there is no head to type into, so the loop guards the text events:

```c
if (bands == NULL)
  continue;
```

placed after the quit and add-band cases. This keeps 010's behaviour that `i`
enters typing mode regardless, while making the "no buffer" state safe: the loop
reads and drops characters, Backspace, Left and Right until `a` creates the
first band. Ctrl-C still exits from either mode, as before.

### Exit frees the whole list

The startup `text_buffer_init(&buf, cols)` is removed, so no buffer exists until
the first `a`. At exit the single `text_buffer_free(&buf)` becomes a walk that
frees every node and its buffer:

```c
while (bands) {
  struct band *next = bands->next;
  text_buffer_free(&bands->buf);
  free(bands);
  bands = next;
}
```

This runs after the read loop breaks on `EVENT_QUIT`, before `return 0`. The
`atexit(restore)` terminal cleanup is untouched.

### The screen-bottom edge is declined

AC8 leaves the case where the bands run past the bottom of the terminal out of
scope. The design still emits a `top_row` beyond the last row and lets the
terminal do whatever it does with it; the program never queries rows and never
counts bands, so it has no notion of "full". That is deliberate and matches the
story's boundary: a later story that wants scrolling or a row budget will have
to bring a `TIOCGWINSZ` row query with it.

### Components

Still `main.c`, `input.[ch]`, `text_buffer.[ch]`, `paint.[ch]`. Changes:

- `struct band` — **new**, declared in `main.c`
- `static struct band *bands` — **new**, replaces `struct text_buffer buf` and
  `int band_drawn`
- `paint_label()` — signature gains `int top_row`; body swaps the hardcoded rows
  for `top_row` and `top_row + 1`
- `input_parse()` — signature loses `band_drawn`; `EVENT_SUMMON_BAND` renamed
  `EVENT_ADD_BAND`
- `main()` — `EVENT_ADD_BAND` allocates, links and paints a new head; the
  `bands == NULL` guard replaces the `band_drawn` guard; typing events use
  `&bands->buf`; exit walks and frees the list

`text_buffer.[ch]` is unchanged: each band simply owns one instance of it.

### Testing

`tests/test_input.c` is updated to the three-argument `input_parse()` and the
renamed event:

- the `band_drawn`-specific cases are removed — `test_a_is_none_once_band_is_drawn`
  goes, and `test_summon_band_key` becomes a check that `a` in `MODE_MOVE`
  yields `EVENT_ADD_BAND`;
- every existing call drops its fourth argument;
- `test_a_is_char_in_type_mode`, `test_i_enters_type_mode`, the escape,
  Backspace and printable cases keep their behaviour, re-expressed against the
  new signature.

The linked list, the row arithmetic and the paint output live in `main.c` and
have no unit test, by the decision to keep the bands flat there. They are
hand-judged end to end in a real terminal, as 001–010 were: press `a`, confirm
band 1; press `Esc`; press `a`, confirm a second empty band sits directly under
the first with the caret in it; type `deploy`, confirm it lands in band 2 and
band 1 is untouched; press `Esc`, press `a` again, confirm a third band; press
Ctrl-C and confirm scrollback intact.
