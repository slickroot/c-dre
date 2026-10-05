## Story

Doug opens the app, presses `a` to summon a label, and types some text. He presses `Esc` to leave typing mode. Curious, he presses a few other keys, but nothing is added to the label. Satisfied, he presses `i` to return to typing mode and continues typing right where he left off.

## Acceptance Criteria

1. Pressing `Esc` while typing exits typing mode.
2. While out of typing mode, pressing keys does not modify the text in the label.
3. Pressing `i` returns to typing mode, resuming text entry at the current cursor position.
4. Pressing `Ctrl-C` exits the application while in or out of typing mode.

## Technical Design

### 1. Mode State

Introduce an explicit application mode enum defined in `input.h` (or shared header) representing the user interaction state:

```c
enum app_mode {
  MODE_MOVE,
  MODE_TYPE
};
```

- The application starts in `MODE_MOVE`.
- `main.c` owns the active `mode` variable alongside `band_drawn`.
- Summoning the band with `a` sets `band_drawn = 1` and switches `mode` to `MODE_TYPE`.
- Receiving `EVENT_ESCAPE` transitions `mode` to `MODE_MOVE`.
- Receiving `EVENT_ENTER_TYPE` transitions `mode` to `MODE_TYPE`.

### 2. Input Parser Changes

To simplify key handling and avoid timing or buffering ambiguities around standalone `Esc`, arrow escape sequence parsing (`EVENT_LEFT` / `EVENT_RIGHT`) is removed for this story. Byte `0x1b` (`Esc`) is treated as an immediate event.

`enum key_event_type` is updated to include:
- `EVENT_ESCAPE`
- `EVENT_ENTER_TYPE`

`input_parse()` signature receives the current mode:

```c
struct key_event input_parse(struct input_parser *parser, char byte, enum app_mode mode, int band_drawn);
```

#### Event Dispatch Rules:
- **Universal**:
  - `0x03` (`Ctrl-C`) returns `EVENT_QUIT` regardless of mode.
- **In `MODE_MOVE`**:
  - `'a'` returns `EVENT_SUMMON_BAND` if `!band_drawn`.
  - `'i'` returns `EVENT_ENTER_TYPE`.
  - All other keys return `EVENT_NONE`.
- **In `MODE_TYPE`**:
  - `0x1b` (`Esc`) returns `EVENT_ESCAPE`.
  - `0x7f` or `0x08` returns `EVENT_BACKSPACE`.
  - Printable ASCII bytes (`0x20` to `0x7e`, including `'i'`) return `EVENT_CHAR`.
  - All other keys return `EVENT_NONE`.

### 3. Orchestration in `main.c`

The event loop in `main()` dispatches on the parsed events:
- `EVENT_QUIT`: exits the loop.
- `EVENT_SUMMON_BAND`: sets `band_drawn = 1`, `mode = MODE_TYPE`, paints the initial label.
- `EVENT_ESCAPE`: sets `mode = MODE_MOVE`.
- `EVENT_ENTER_TYPE`: sets `mode = MODE_TYPE`.
- `EVENT_CHAR`: inserts character into `buf` and repaints.
- `EVENT_BACKSPACE`: backspaces in `buf` and repaints.
- `EVENT_NONE`: does nothing.

### 4. Unit Testing

Automated tests in `tests/test_input.c` will verify:
- `Esc` (`0x1b`) in `MODE_TYPE` produces `EVENT_ESCAPE`.
- In `MODE_MOVE`, letters like `'x'`, `'b'`, space, etc. produce `EVENT_NONE`.
- `'i'` in `MODE_MOVE` produces `EVENT_ENTER_TYPE`.
- `'i'` in `MODE_TYPE` produces `EVENT_CHAR` with `ch == 'i'`.
- `Ctrl-C` (`0x03`) produces `EVENT_QUIT` in both `MODE_MOVE` and `MODE_TYPE`.
