## Refactoring Goal

Improve code readability, testability, and modularity by breaking apart the oversized monolithic `main()` loop and separating concerns into three decoupled components following a unidirectional data flow:
```
raw input bytes -> input parser -> text_buffer -> paint
```

## Acceptance Criteria

1. Background painting is encapsulated in `paint_wallpaper()` and called on startup.
2. The label rendering logic `paint_label()` accepts state explicitly (`const struct text_buffer *buf, int cols`) without relying on global mutable state.
3. Text buffer manipulation (`insert`, `backspace`, `left`, `right`) is isolated into a dedicated `struct text_buffer` module with discrete mutator functions and self-contained boundary checking.
4. Terminal escape sequence handling and input decoding are encapsulated in an `input` parser component that maps raw bytes to discrete key events.
5. `text_buffer` and `input` are covered by automated unit tests using standard `assert.h`.
6. Existing end-user behavior is 100% preserved (Ctrl-C exits cleanly, 'a' summons band, text inserts at cursor, left/right moves cursor, backspace deletes left of cursor, word remains centered, colors and escape sequences remain identical).

## Technical Design

### 1. Data Flow Architecture

The application pipeline is strictly unidirectional:
- **Input (`input.h` / `input.c`)**: Receives raw bytes from stdin, manages escape sequence parsing state, and outputs high-level key events.
- **State (`text_buffer.h` / `text_buffer.c`)**: Holds the active text and cursor position. Mutated by discrete functions based on input events.
- **Renderer (`paint.h` / `paint.c`)**: Pure terminal rendering. Reads `text_buffer` and terminal dimensions to output ANSI escape sequences to stdout.
- **Orchestrator (`main.c`)**: Sets up the terminal, instantiates the buffer and parser, runs the event loop, and restores the terminal on exit.

### 2. Component Specifications

#### A. Terminal Painting (`paint.h`, `paint.c`)
- **`void paint_wallpaper(void)`**:
  - Encapsulates alternate screen buffer switch (`\x1b[?1049h`), hiding cursor (`\x1b[?25l`), background color set (`\x1b[48;2;10;10;11m`), clear screen (`\x1b[2J`), and cursor home (`\x1b[1;1H`).
  - Called once during initial setup in `enter()`.
- **`void paint_label(const struct text_buffer *buf, int cols)`**:
  - Guards against `cols == 0` and `buf->len >= cols`.
  - Emits the 3-row band prefix (`\x1b[48;2;63;63;70m\x1b[38;2;201;201;207m\x1b[1;1H\x1b[K\r\n\x1b[K\r\n\x1b[K\x1b[?25h`).
  - Emits cursor positioning to center: `\x1b[2;%dH` with `(cols - buf->len) / 2 + 1`.
  - Emits `buf->data` (length `buf->len`) followed by color reset `\x1b[0m`.
  - Positions caret at `(cols - buf->len) / 2 + buf->cursor + 1`.

#### B. Text Buffer (`text_buffer.h`, `text_buffer.c`)
- **State**:
  ```c
  struct text_buffer {
    char *data;
    int len;
    int cap;
    int cursor;
  };
  ```
- **Operations**:
  - `void text_buffer_init(struct text_buffer *buf, int cap)`: Allocates `cap + 1` bytes, sets `len = 0`, `cursor = 0`, and null-terminates.
  - `void text_buffer_free(struct text_buffer *buf)`: Frees allocated memory.
  - `int text_buffer_insert(struct text_buffer *buf, char c)`:
    - If `buf->len >= buf->cap`, returns 0.
    - Shifts characters from `buf->cursor` right by 1 using `memmove`.
    - Inserts `c` at `buf->cursor`, increments `len` and `cursor`, sets `buf->data[buf->len] = 0`.
    - Returns 1.
  - `int text_buffer_backspace(struct text_buffer *buf)`:
    - If `buf->cursor <= 0`, returns 0.
    - Shifts characters from `buf->cursor` left by 1 using `memmove`.
    - Decrements `len` and `cursor`, sets `buf->data[buf->len] = 0`.
    - Returns 1.
  - `int text_buffer_left(struct text_buffer *buf)`:
    - If `buf->cursor > 0`, decrements `cursor` and returns 1.
    - Otherwise returns 0.
  - `int text_buffer_right(struct text_buffer *buf)`:
    - If `buf->cursor < buf->len`, increments `cursor` and returns 1.
    - Otherwise returns 0.

#### C. Input Decoding (`input.h`, `input.c`)
- **Types**:
  ```c
  enum key_event_type {
    EVENT_NONE = 0,
    EVENT_QUIT,
    EVENT_SUMMON_BAND,
    EVENT_CHAR,
    EVENT_LEFT,
    EVENT_RIGHT,
    EVENT_BACKSPACE
  };

  struct key_event {
    enum key_event_type type;
    char ch;
  };

  struct input_parser {
    int seq;
  };
  ```
- **Operations**:
  - `void input_parser_init(struct input_parser *parser)`: Initializes `seq = 0`.
  - `struct key_event input_parse(struct input_parser *parser, char byte, int band_drawn)`:
    - If `byte == 0x03`: returns `{ EVENT_QUIT, 0 }`.
    - Implements sequence state machine:
      - `seq == 0 && byte == 0x1b`: `seq = 1`, returns `EVENT_NONE`.
      - `seq == 1 && byte == 0x5b`: `seq = 2`, returns `EVENT_NONE`.
      - `seq == 1 && byte != 0x5b`: `seq = 0`, returns `EVENT_NONE`.
      - `seq == 2 && byte < 0x40`: if `byte == 0x1b` `seq = 1`; returns `EVENT_NONE`.
      - `seq == 2 && byte >= 0x40`:
        - `seq = 0`
        - if `byte == 'D'`: returns `{ EVENT_LEFT, 0 }`.
        - if `byte == 'C'`: returns `{ EVENT_RIGHT, 0 }`.
        - otherwise: returns `EVENT_NONE`.
      - Normal keys (`seq == 0`):
        - if `!band_drawn && byte == 'a'`: returns `{ EVENT_SUMMON_BAND, 0 }`.
        - if `byte == 0x7f || byte == 0x08`: returns `{ EVENT_BACKSPACE, 0 }`.
        - if `byte >= 0x20 && byte <= 0x7e`: returns `{ EVENT_CHAR, byte }`.
    - Default: returns `EVENT_NONE`.

#### D. Streamlined Event Loop (`main.c`)
```c
char byte;
while (read(STDIN_FILENO, &byte, 1) == 1) {
  struct key_event ev = input_parse(&parser, byte, band_drawn);
  if (ev.type == EVENT_QUIT)
    break;
  if (ev.type == EVENT_SUMMON_BAND) {
    band_drawn = 1;
    paint_label(&buf, cols);
    continue;
  }
  if (!band_drawn)
    continue;

  int changed = 0;
  switch (ev.type) {
  case EVENT_CHAR:
    changed = text_buffer_insert(&buf, ev.ch);
    break;
  case EVENT_BACKSPACE:
    changed = text_buffer_backspace(&buf);
    break;
  case EVENT_LEFT:
    changed = text_buffer_left(&buf);
    break;
  case EVENT_RIGHT:
    changed = text_buffer_right(&buf);
    break;
  default:
    break;
  }
  if (changed)
    paint_label(&buf, cols);
}
```

### 3. Testing Strategy (TDD)

A test binary compiled with standard `assert.h` verifies components without spawning the terminal UI:
- `tests/test_text_buffer.c`:
  - Initialization state (`len == 0`, `cursor == 0`).
  - Insertion at end, beginning, and middle of buffer.
  - Capacity boundary enforcement (`len == cap`).
  - Backspace at start (noop), middle, and end.
  - Left / Right movement and boundary clamping (`0` and `len`).
- `tests/test_input.c`:
  - Quit signal (`0x03`).
  - Normal printable chars and summon band key `a`.
  - ANSI arrow key sequences (`\x1b[D`, `\x1b[C`).
  - Discarding unsupported escape parameters/keys (`\x1b[3~`, `\x1b[H`).


