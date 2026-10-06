# stateless-input-parser

## Refactoring Goal

`struct input_parser` holds a single `int unused`. It used to be `int seq`,
the escape-sequence state machine that grouped `ESC [ C/D` into arrow keys
for spec 008. Spec 010 made Esc mean "leave type mode" and `5db15fc` removed
the state machine, but the struct stayed behind as a placeholder (C has no
empty structs) so that `input_parser_init()` and every `&parser` call site
would still compile. Both functions now open with `(void)parser`, and every
test in `tests/test_input.c` sets up a parser only to hand it in.

The parser keeps no state, so its API shouldn't pretend it does. Remove the
struct and the init function. `input_parse()` becomes a pure function of
`(byte, mode)`.

## Technical Design

Still `main.c`, `input`, `text_buffer` and `paint`. No new module or file.
Only `input.[ch]`, `tests/test_input.c` and two lines of `main.c` change.

### `input.h`

Removed:

```c
struct input_parser {
	int unused;
};

void input_parser_init(struct input_parser *parser);
```

New signature:

```c
struct key_event input_parse(char byte, enum app_mode mode);
```

`enum key_event_type`, `enum app_mode` and `struct key_event` are unchanged.

### `input.c`

`input_parser_init()` is deleted. `input_parse()` drops the `parser`
parameter and its `(void)parser;` line. The body is otherwise byte-for-byte
the same, so every key keeps its current mapping.

### `main.c`

The two setup lines are deleted:

```c
struct input_parser parser;
input_parser_init(&parser);
```

The call becomes `input_parse(byte, mode)`.

### Known limitation, kept on purpose

Because the parser keeps no state, a multi-byte key comes in as separate
events. In type mode, ← (`ESC [ D`) is `EVENT_ESCAPE` (which switches to
move mode) and then `[` and `D`, which are parsed in move mode and happen to
be unbound. Alt+x behaves the same way. In move mode, ↓ (`ESC [ B`) does
nothing only because `B` isn't bound. **Binding any letter that ends a CSI
sequence (`A`–`D`, `H`, `F`, `~`, …) in move mode will make those keys
trigger the command.**

`test_escape_in_type_mode` keeps asserting this split so the behavior stays
documented. Grouping escape sequences is left to the first story that needs
arrow keys. That story brings the struct back with a real field (e.g.
`enum { GROUND, ESC, CSI } state`) and must decide how to tell a lone Esc
from the start of a sequence when input is read one byte at a time with no
timeout.

### Relationship to spec 018

018 is in progress and its `main.c` snippet still passes `&parser`. 018 is
not edited here. Whichever spec lands second drops `&parser` (and the
parser setup) from the new loop when it rebases.

### Components

- `input.[ch]`: `struct input_parser` and `input_parser_init()` removed;
  `input_parse(char byte, enum app_mode mode)` is pure, with no state and no
  I/O
- `main.c`: no parser variable; calls `input_parse(byte, mode)`
- `tests/test_input.c`: drops the parser setup from every test
- `Makefile`, `text_buffer.[ch]`, `paint.[ch]`: untouched

### Testing

The existing `tests/test_input.c` suite is the safety net. Each test loses
its two setup lines and calls `input_parse(byte, mode)` directly. No
assertion changes, and no tests are added or removed. `make test` and
`make lint` stay green, and `grep -rn input_parser` finds nothing.

Hand-judged end to end: `a` adds a band and types, Esc/`i` switch modes,
`j`/`k` select, `d` deletes, Ctrl-C quits and restores the terminal.
