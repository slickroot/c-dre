## Story

Doug keeps pressing `a`. Each press adds another `#3F3F46` square directly to the right of the last, and when the row runs out the squares carry on at the left of the row below. He paints his way across the whole screen, top to bottom, and when he finally fills the last row, pressing `a` again leaves his picture exactly as it is. He exits with Ctrl-C, scrollback intact.

## Acceptance Criteria

1. Doug presses `a` any number of times, starting from one square already on the screen.
2. Each press adds one `#3F3F46` square, 2 cells wide and 1 cell tall, directly to the right of the previous square, with no gap between them.
3. Squares on one row form a solid unbroken run of `#3F3F46` starting at the first cell of the row.
4. When the current row cannot fit another square, the next square is drawn at the first cell of the row directly below, and filling continues left to right again.
5. No cursor is visible anywhere on the screen at any point.
6. Every cell Doug has not drawn on stays `#0A0A0B`.
7. Ctrl-C returns Doug to his shell with his scrollback intact, exactly as it is today.

AC7 from the original draft — pressing `a` again once every row is filled
leaves the screen unchanged — was struck during the technical design meeting.
It is unreachable under the decision below and is recorded there rather than
implemented as a fudge.

## Technical Design

### Position belongs to the terminal

`paint_square()` emits no position. It stays exactly what 002 left it —
`ESC[48;2;63;63;70m`, `"  "`, `ESC[0m` — and advances by nothing. Every
square lands "directly to the right of the last, with no gap" because the
terminal's own cursor is already sitting two cells past the previous square
when the previous write returned. AC2 and AC3 are satisfied by the terminal's
cursor advance, not by arithmetic on our side.

Row advance is likewise the terminal's: at the right margin, writing past the
last cell wraps to column 1 of the row below and the left-to-right run starts
again. AC4 needs nothing from us either.

This is a deliberate reversal of the plan 002 deferred. 002 said 003's
window-size query and wrap arithmetic would land in `paint_square()`; both are
declined here. The program does not know how wide the terminal is, and it does
not know where it has drawn.

### What was given up, and why

**Struck AC7.** With no size query there is no way to know the screen is full.
Printing two more spaces at the bottom-right cell is not a no-op — most
terminals treat it as a scroll, shifting the alt screen up a row. Under this
design that behaviour belongs to the terminal, not to `dre`. The alternative
considered and declined was a one-shot `TIOCGWINSZ` in `enter()` keeping only
`cols * rows` as a draw counter, still with the terminal owning coordinates;
it would have kept positions resize-tolerant while giving us a stop condition.
It was not worth reintroducing the program's only dependence on a query that
can fail — `ws_col` may come back 0 on an unusual pty, which would make every
press look full. So the acceptance criterion was removed rather than satisfied
by a guess.

**Odd-column raggedness.** If `cols` is odd, a row holds `cols / 2` clean
squares and one leftover cell. The next press paints that cell alone; the press
after it wraps. So with `cols = 81`, row 1 ends in a half-square and row 2
starts one cell late, against AC3 and AC4. This is inherent to delegating
position. Repairing it needs the column number we declined to track, so it is
documented rather than fixed. A terminal reporting an even `cols` never shows
it.

### Painting

Unchanged from 002, deliberately. The pen is still set per square rather than
once in `enter()`, so `paint_square()` keeps being able to state its own
colour and remains stateless. Setting `#3F3F46` for the session and dropping
`ESC[0m` would be one write per press instead of three, and would model this
story better — a grey run on a grey canvas — but it makes the colour ambient
state owned elsewhere, and it gives up 002's settled argument for the reset.
Nothing prints after the squares in this story, so the reset buys nothing yet;
it buys the first feature that prints text, which would otherwise silently come
out grey. Three writes and the reset stay.

The `ESC[0m` also continues to keep the background correct for anything later
in the session; restoring `#0A0A0B` instead would work for painting but not
survive the first attribute anyone adds.

### Components

Still one `main.c`, still four functions, still no headers. Nothing in 002's
component list changes: `paint_square()` keeps its name because it is still
self-contained, `enter()` and `restore()` are untouched, and `main()`'s loop
remains a plain byte dispatcher — `0x03` breaks, `'a'` paints, everything else
is dropped. The story needed no new seam, only the decision not to add one.

No `TIOCGWINSZ`, no `<sys/ioctl.h>`, no geometry state anywhere. That absence
is the design, and it is the thing a later story will have to argue past.

### Testing

None, continuing from 001 and 002. The acceptance criteria are judged by hand
in a real terminal: press `a` enough times to cross a row boundary and confirm
the run continues at the left of the row below.

Two of the things worth testing are now untestable by construction rather than
by omission — the fill-the-screen stop condition and the odd-column edge — since
the program observes neither its own output geometry nor its own fullness. The
pty harness both earlier specs left open remains available for the escape
sequences and the Ctrl-C path.
