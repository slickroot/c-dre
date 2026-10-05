## Story

Doug runs `dre`. His whole terminal fills with the quiet near-black canvas and no cursor blinks anywhere. He presses `a`, and a single grey square appears in the top-left corner. He has his first mark on the canvas.

## Acceptance Criteria

1. Doug runs `dre` and presses `a` exactly once.
2. A `#3F3F46` square appears, 2 cells wide and 1 cell tall, occupying the first two cells of the top row.
3. No cursor is visible anywhere on the screen.
4. Every other cell stays `#0A0A0B`.
5. Ctrl-C returns Doug to his shell with his scrollback intact, exactly as it is today.

## Technical Design

### Cursor

`enter()` gains `ESC[?25l`, hiding the cursor, alongside the alt-screen switch
and the background paint already there. It belongs there rather than in the
paint path: the cursor must be hidden from the moment the session starts, not
from the moment something is drawn, and a canvas that is still blank is still a
canvas with no cursor on it. It rides with the other session-level modes.

### Painting

No cell buffer. The canvas stays the terminal's own memory; `paint_square()`
writes escape sequences and nothing else. A buffer earns its place when a
feature needs to know what is already on screen — an eraser, save, undo — and no
such feature is in play.

The square is position, pen, two spaces, reset:

1. `ESC[1;1H` — cursor to row 1, column 1, the top-left cell
2. `ESC[48;2;63;63;70m` — background pen to `#3F3F46` (63, 63, 70)
3. `"  "` — two spaces, printing exactly two cells
4. `ESC[0m` — reset

Step 3 is the load-bearing one. The alternatives that came up — `ESC[K` to
erase to end of line, or any countless erase — paint the entire row from the
cursor rightwards, because `K` takes no count and simply clears to the
terminal's edge. With the square at column 0 of an empty row that would look
right on screen while violating AC4 for every cell to its right. Printing two
spaces is bounded by construction: exactly the two cells AC2 names, and nothing
else on the row is touched.

Step 4 is not optional. After step 3 the terminal's current background is still
`#3F3F46`, so anything printed next would inherit grey. `ESC[0m` clears all SGR
attributes back to their defaults. Restoring the canvas background instead
(`ESC[48;2;10;10;11m`) would be equally correct for painting and would not
survive the first attribute anyone adds later, so the full reset wins.

Nothing about this depends on the terminal size, so there is still no
`TIOCGWINSZ` query.

### Output mechanism

All terminal output goes through `write(STDOUT_FILENO, ...)`, as in 001 —
unbuffered, so bytes reach the tty immediately and ordering is never in question.
`paint_square()` follows the same rule; there is no stdio path for escape
sequences anywhere in the program.

The `fflush(stdout)` at the end of `restore()` is already dead code, since
nothing writes through stdio. It is left alone here rather than churned as an
unrelated cleanup.

### Colour literals

`63;63;70` and the two-space width are written inline in `paint_square()`. The
constant is duplicated when 003 needs the same grey, and that duplication is
accepted deliberately: naming it now would create a symbol with exactly one use
until 003, and parameterising the cell position would be the cell buffer
arriving under another name. When a third caller appears, extract it then.

### Components

One `main.c`, as in 001, with a fourth function:

- `static void paint_square(void)` — the four writes above
- `enter()` — save termios, raw mode, alt screen, hide cursor, paint background
- `restore()` — leave alt screen, restore termios
- `main()` — the isatty guard, `atexit(restore)`, the read loop

`main()`'s loop stays a plain dispatcher: read one byte, `0x03` breaks, `'a'`
calls `paint_square()`, every other byte is dropped as before. Its only job is
mapping bytes to meaning. 003's window-size query and wrap arithmetic belong in
`paint_square()`, where the geometry is, rather than in the loop.

### Testing

None, continuing the decision from 001: no unit tests and no pty harness yet.
The acceptance criteria are judged by hand in a real terminal. The natural
harness — a script that allocates a pty, runs the binary on the slave, sends `a`
then `0x03`, and asserts on the byte stream — needs no test-only code in
`main.c`, so nothing here forecloses it.
