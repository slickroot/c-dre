## Story

Doug runs `./dre`, his terminal fills with the quiet near-black canvas. He
presses `a` and a single band of lighter grey appears — three rows tall,
reaching from the left edge of his terminal all the way to the right. He has his
first mark on the canvas. Happy, he presses Ctrl-C and gets his terminal back
exactly as it was.

## Acceptance Criteria

1. Doug runs `./dre` and presses `a` exactly once.
2. A band in `#3F3F46` appears that is exactly 3 rows tall.
3. Every cell of those 3 rows is painted, from the first column of the terminal
   to the last — no cell in them is left as canvas colour.
4. The band starts at the top row of the terminal.
5. Every cell outside those 3 rows stays `#0A0A0B`.
6. No cursor is visible anywhere on the screen.
7. Ctrl-C returns Doug to his shell with his scrollback intact, exactly as it
   is today.

## Technical Design

### The band reaches the edge by erasing, not by writing

AC3 requires every cell of the three rows to be painted, all the way to the
last column. Printing `cols` spaces per row would satisfy it, but 003 settled
the opposite design — no `TIOCGWINSZ`, no geometry state, position belongs to
the terminal. Those two cannot both hold, because writing N cells requires
knowing N, and `ws_col` may come back 0 on an unusual pty.

So the band is painted by erase-to-end-of-line. `ESC[K` fills from the cursor
to the terminal's own right margin, whatever that is, with the currently
selected background. It reaches the edge by definition rather than by
arithmetic, and it reintroduces no size query. 003's core decision survives
this story intact, and the cost is that `paint_band()` must place the cursor at
the left margin before each erase — the erase covers the rest of the row.

A per-row `ESC[K` is three writes where one `ESC[K` after a `CUP` would do, and
the redundancy is not noise: it is what makes the function state-free. Each
erase begins from a cursor position it establishes itself, so no argument and
no static has to remember where the previous one finished.

### Rows 1-3 always, every press

AC4 anchors the band at the top row and AC1 says exactly one press, which
leaves the second press undefined. Decision: `a` is idempotent. Every press
paints rows 1 through 3 and leaves the band exactly where the last one did.
There is no band stack and no second band.

This retires `n_a`. Under 003 it counted presses so the terminal's cursor
would walk right; with a fixed band there is no walk and nothing to count. The
counters and the arithmetic that positioned `A` both go, and with them
`paint_at()`, `cols`, `n_A`, `n_a` and `<sys/ioctl.h>`. `main.c` becomes a
program with exactly two keys.

`A` does nothing. It is not moved, not repositioned, not given new behaviour
relative to the band — the `else if (byte == 'A')` branch is deleted outright
and every byte that is not `0x03` or `a` is read and dropped. There is no
uppercase feature to reconcile with a top-anchored band, and inventing one
would be a story of its own.

### `paint_band()` replaces `paint_square()`

One function, self-contained, replacing the one 002 introduced and 003 kept
unchanged. Same shape, new body: it states its own colour, does its own
positioning, and resets the pen before returning. It emits, in order:

1. `ESC[48;2;63;63;70m` — the pen. `#3F3F46` decomposes to 63, 63, 70, and as
   in 001 the colour is out of every xterm palette index, so it can only be
   sent exactly. Same value 002 and 003 used for the square.
2. `ESC[1;1H` — the cursor to the top-left cell. The alt screen arrives with
   the cursor wherever it was left, so this is not inherited state.
3. `ESC[K` — rows 1 through 3's first row.
4. `\r\n` — down one row, left margin.
5. `ESC[K`
6. `\r\n`
7. `ESC[K`
8. `ESC[0m` — the pen reset.

Relative `\r\n` was chosen over `ESC[2;1H` / `ESC[3;1H`. Three fewer bytes and
one fewer assumption about where the cursor is. The cost is that the cursor
ends on row 4, which is unobservable: `enter()` hid it with `ESC[?25l` and
nothing in this story prints text.

The `ESC[0m` stays for 003's reason, unchanged: the first feature that prints
text must not come out grey. Banding three rows instead of one square does not
move that argument, and dropping the reset to save a byte would make the colour
ambient state owned elsewhere.

The canvas needs no work for AC5. `enter()` sets `#0A0A0B` as the default
background and erases with `ESC[2J`, and `paint_band()` touches only the three
rows it moves through — rows 4 downward are never addressed, so they keep the
canvas colour they arrived with.

### The short-terminal edge is declined

On a terminal with fewer than four rows the second `\r\n` scrolls rather than
moving down, and the painted row 1 shifts off the top of the screen. AC3 and
AC4 then fail. This is the same class of terminal-owned behaviour 003 recorded
and declined to fix for odd `cols`: repairing it needs the row count that this
design has committed to not asking for. Declined, same as 003, and it is the
edge a later story will have to argue past.

### Components

One `main.c`, as in 001 through 003, and now genuinely smaller than it was:

- `enter()` — unchanged from 001
- `restore()` — unchanged from 001
- `paint_band()` — the three erases above
- `main()` — `isatty` guard, `atexit(restore)`, `enter()`, read loop; `0x03`
  breaks, `a` calls `paint_band()`, everything else is dropped

`paint_square()` and `paint_at()` are deleted rather than kept as dead code.
No headers, no module seams, no new state. `enter()` still writes
`\x1b[48;2;10;10;11m` and `\x1b[2J` and no longer needs the `cols` it used to
query in the same breath.

### Testing

None, continuing from 001 through 003. Hand-judged in a real terminal: press
`a`, confirm three grey rows from the left edge to the right and nothing else
changed, press Ctrl-C, confirm scrollback intact.

The pty harness the earlier specs left open is still available and would assert
cleanly on this story's byte stream, since `paint_band()` is fixed and short.
Declined here for the same reason as before: the criteria are cheap to check by
eye, and no test-only seam is needed in `main.c` for a harness to exist later.
