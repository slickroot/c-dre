## Story

Doug runs `./dre` and presses `a` — the grey band appears. He types `deploy` and
each letter shows up in the middle of the band, in the lighter grey text colour,
sliding sideways so it stays centred as the word grows. His first label on the
canvas.

## Acceptance Criteria

1. Doug runs `./dre`, presses `a`, then types `deploy`.
2. The band is 3 rows of `#3F3F46`, full terminal width, at the top.
3. Nothing is on the band until he types.
4. The `a` that summons the band does not appear as text. Each character he
   types afterwards appears on the middle row, so `deploy` reads left to right
   once he's done.
5. The text is `#C9C9CF`.
6. The text stays horizontally centred after every keystroke, within one cell
   when the width doesn't divide evenly.
7. Typing changes no background: the band stays `#3F3F46` on all 3 rows, and
   every cell outside it stays `#0A0A0B`.
8. No cursor is visible.
9. Non-printable keys other than Ctrl-C do nothing and show nothing, except
   Backspace, which deletes the last character of the label and repaints.
10. Ctrl-C returns Doug to his shell with scrollback intact.

## Technical Design

### This story reverses 003's central rule

003 decided that position belongs to the terminal: no `TIOCGWINSZ`, no
geometry state, no knowledge of where the program had drawn. 005 leaned on that
hard, painting the band with three `ESC[K` erases so it reached the right edge
without ever asking how wide the edge was.

AC6 cannot be satisfied under that rule. Centring is the first thing this
program must compute rather than delegate. There is no ANSI primitive that
centres for us: `CUP` needs a column number, and no combination of
save/restore-cursor or insert-blank makes the terminal halve a width we have
never learned. Either the program knows `cols` or AC6 is unimplementable.

**Decision: one `TIOCGWINSZ` in `enter()`, keeping `cols` in a static.**
`<sys/ioctl.h>` comes back — the header 005 deleted — along with
`struct winsize`. The query happens once, at startup, alongside the alt-screen
and raw-mode setup that already happens there. No per-keystroke query, so the
`len >= cols` guard and the centring arithmetic are pure arithmetic on a cached
value.

This is a deliberate reversal, not an oversight. 003's rule survives in one
weakened form and should be read that way from here on: **the terminal still
owns where the band reaches** — the erases are unchanged — but the program now
owns one number, `cols`, and uses it only for horizontal centring of text.
Every other use of geometry remains declined. The thing 003 said a later story
would have to argue past has arrived, and it argued itself out on AC6.

### `a` summons the band once, and is a character after that

002 through 005 gave `a` exactly one job: paint. That job is unchanged here. What
changed is that this story gives `a` a *second* meaning while typing, and the two
collide — AC1 has Doug press `a` before he types anything, and every keystroke he
makes after that must land on the band.

An earlier draft of this design resolved the collision by deleting the command:
`a` would become an ordinary character, the first printable keystroke would paint
the band, and that keystroke's own glyph would land with it, so the label would
read `adeploy`. The argument was that a canvas tool cannot reserve a letter, since
every letter is one Doug might eventually want to write. Doug rejected that on
review — pressing `a` draws an empty band, and no `a` appears on it. An empty
band with the text he then types is what the acceptance criteria describe, and
that is what this design now builds.

**Decision: `a` is the band command on the first keystroke only, and does not
append itself to the label.** `main()` keeps a `static int band_drawn` (or
equivalent latch), initialised to 0. The input loop tests the first keystroke
against `'a'`; if it matches, it sets the latch and calls `paint_label()` without
storing the character. Every `a` after that arrives at the printable-range branch
with the latch already set and is appended like any other letter.

The latch, not `len == 0`, decides. Tying it to an empty label would re-arm the
command whenever Doug backspaced down to nothing, so an `a` typed at the start of
a second label would silently eat itself. One keystroke, one band, for the life of
the process.

This keeps 002's key and leaves the alphabet whole for every keystroke that
follows, which is the compromise the collision actually forces.

### Text input: printable ASCII and Backspace

AC9 as originally written — non-printables other than Ctrl-C do nothing — was
amended during this meeting to admit Backspace. Doug asked why text input could
not work properly, with arrows and Backspace. The honest reason is that arrows
and Backspace are not the same size of thing.

In raw mode an arrow key arrives as three bytes, `ESC [ C` or `ESC [ D`, not
one. Making arrows do something means parsing that sequence and giving the caret
a position. AC8 says no cursor is visible, so there would be nothing on screen
to show where the caret is — the label would slide invisibly, or the story
would need a visible caret it never mentions. Caret movement is a text-field
feature and belongs to its own story.

Backspace is the cheap half and stays: it is one byte, either `0x7f` or `0x08`
depending on the terminal, and deleting a character shows nothing *else*.

So the input rule is: bytes `0x20` through `0x7e` append to the label; `0x7f`
and `0x08` decrement `len` when `len > 0` and repaint; `0x03` breaks; everything
else is read and dropped. Restricting to `0x20`–`0x7e` also means the buffer can
never contain a byte that `ESC`-prefixes into an escape sequence of our own
making. `0x7f` is excluded from the printable range precisely so Backspace stays
a distinct case.

### `paint_label()` replaces `paint_band()`

One function owns the entire repaint, so there is no ordering contract between
two painters and no pen left set across a call boundary. `paint_band()` is
renamed and its body extended; 002's convention that a paint function states its
own colours and resets the pen is kept, and the band background and the text
foreground are simply set in the same pen.

That convention was load-bearing. 005 ended `paint_band()` with `ESC[0m` and
recorded that the reset "buys the first feature that prints text". This is that
feature, and the reset is why AC7 holds: the text is written while the pen still
says `#3F3F46`, so the cells under the glyphs stay band grey and only the
foreground changes to `#C9C9CF`. Splitting this into `paint_band()` then
`paint_text()` would have to re-state the band background and would create a
real hazard — pressing `a` again... which, per the decision above, cannot happen.
But the composition would still be fragile, and one function that owns one
repaint has nothing to get out of order.

It emits, in order:

1. `ESC[48;2;63;63;70m` — the band background, unchanged from 002 through 005.
2. `ESC[38;2;201;201;207m` — the text foreground. `#C9C9CF` decomposes to 201,
   201, 207, and as with the band colour it is outside every xterm palette
   index, so it can only be sent exactly.
3. `ESC[1;1H` — the top-left cell. Not inherited state; the alt screen arrives
   with the cursor wherever it was left.
4. `ESC[K\r\n` `ESC[K\r\n` `ESC[K` — the three erases, byte-for-byte as 005
   shipped them. Still relative, still geometry-free: `ESC[K` reaches the right
   edge by definition rather than by arithmetic on `cols`. This is where 003's
   rule still holds.
5. `ESC[2;%dH` — the cursor to the middle row at the start column, formatted
   with `snprintf`. `snprintf` is the story's only new dependency.
6. the label bytes, `len` of them, no terminator.
7. `ESC[0m` — the pen reset, for 002's reason.

The cursor is left sitting inside the label after the text write. That is
unobservable: `enter()` hid it with `ESC[?25l` and nothing shows it, and the
next repaint re-establishes position with step 3. No trailing move is sent, and
no trailing move is needed.

**Start column: `(cols - len) / 2 + 1`, integer division.** The `+ 1` is because
`CUP` is 1-based and the division is 0-based. The truncation is what satisfies
AC6's "within one cell when the width doesn't divide evenly" — with `cols = 81`
and `len = 6`, the label starts at column 38 and the leftover cell goes to the
right, so it is off-centre by half a cell at most and never by more. Ceiling
instead of truncation would be equally within one cell and equally unobservable;
truncation is chosen because it never attempts to place a start left of column 1.

**Why `snprintf` for the CUP and relative `\r\n` for the erases.** 005 chose
relative moves because the program had no geometry, so absolute `ESC[2;1H` would
have been pure waste. The program has geometry now, but only for the text: the
erases never depend on `cols`, so converting them to absolute CUP would cost
bytes and change nothing. The one coordinate that genuinely needs a number is the
text position, and it gets one. This was pressed in the meeting and the honest
answer is that there is no benefit to be had here either way — the choice is to
change as little of 005's shipped byte stream as the story allows.

### A label wider than the terminal draws nothing

Writing a character in the last column makes the terminal wrap. Writing past the
last cell of the middle row usually *scrolls* the alt screen, shifting the band
up one row and breaking AC2 and AC7 at once. 003 struck an acceptance criterion
rather than guess at this; the same applies.

**Decision: `paint_label()` writes no text at all once `len >= cols`.** AC6
stays satisfiable because there is nothing left to centre. No wrap, no scroll,
no write-past-the-margin surprise. Keystrokes are still accepted and `len` still
grows — the label is frozen on screen, and Backspace shrinks `len` until it is
under `cols` again, at which point the label reappears, centred, from the
buffer. The freeze is visible and the recovery is automatic, which is the honest
behaviour: the alternative, truncating to the last `cols - 1` characters, would
silently discard what Doug typed, and wrapping across the three rows would make
the middle row stop being the label.

The overflow is recorded, not repaired, in the same spirit as 003's struck AC7
and 005's declined short-terminal edge. It is not an acceptance criterion
failure — AC6 says nothing about a label longer than the terminal.

### The buffer is sized to the terminal, and a zero width stops everything

003 named the reason it avoided `TIOCGWINSZ`: `ws_col` may come back 0 on an
unusual pty. That was tolerable when the query would only have fed a draw
counter. Now it feeds an allocation and the centring arithmetic, so it gets a
decision.

**Decision: if `cols == 0`, nothing is ever painted.** `enter()` leaves `cols`
at zero, the buffer is allocated as one byte (or not at all), and every
`paint_label()` call returns early. The band never appears, no text is drawn, and
the program sits there reading bytes until Ctrl-C. That is loud — a user on such
a pty sees a blank near-black screen and cannot tell whether the program is
broken — but the alternatives were worse. Falling back to 80 puts the label in
the wrong place, which AC6 calls a failure anyway. Exiting with a diagnostic is
the most honest of the three and was declined: a canvas tool that refuses to
start has no recovery path, and the failure is not recoverable by the user in
any case.

**The buffer is `char label[cols + 1]`, NUL-terminated, allocated once.** The
`+ 1` is for the terminator; the text write never sends it, since `CUP` plus
`len` bytes is already exact. Sizing to the queried width rather than a
hand-picked constant means no arbitrary limit exists to be wrong: `len` cannot
outgrow what the terminal could show, and the `len >= cols` guard means the
overflowing characters are stored but never drawn.

### Components

Still one `main.c`, still no module seams, still matching 001 through 005.

- `static int cols` — the cached `ws_col`, zero until `enter()` fills it
- `static char *label` — `cols + 1` bytes, allocated in `enter()`
- `static int len` — how much of the label is meaningful
- `static int band_drawn` — whether the first-keystroke `a` has been spent
- `enter()` — unchanged, plus the `TIOCGWINSZ` query, the allocation, and
  `<sys/ioctl.h>` / `<stdlib.h>` for the two. Still writes `ESC[?1049h`,
  `ESC[?25l`, the `#0A0A0B` default, `ESC[2J` and `ESC[1;1H`
- `restore()` — unchanged
- `paint_label()` — the eight-step sequence above, early-returning when `cols`
  is 0 or `len >= cols`
- `main()` — `isatty` guard, `atexit(restore)`, `enter()`, read loop. `0x03`
  breaks; a first-keystroke `a` sets `band_drawn` and repaints without appending;
  `0x20`–`0x7e` append and repaint; `0x7f` and `0x08` delete and repaint when
  `len > 0`; everything else is dropped

`paint_band()` is deleted rather than kept as dead code. The `byte == 'a'` branch
survives in the one form this story settles on: it now also tests `!band_drawn`,
and it does not append. `paint_at()`, `paint_square()`, `n_A` and `n_a` remain
gone from 005.

Note that Backspace and a printable both repaint, which means the repaint is
what the input handler calls, not the other way round. `paint_label()` takes no
arguments and reads all three statics; it stays self-contained in the sense 002
meant, owning its position, its colours and its reset, but it is no longer
argument-free and stateless. That is the cost of this story and it is the first
state the program has carried since 005.

### Testing

None, continuing from 001 through 005. Hand-judged in a real terminal: press `a`,
confirm the band appears with no `a` on it, type `deploy`, confirm the label reads
`deploy` and is centred to within a cell, press Backspace, confirm `deplo`, press
`a` again and confirm it appends an `a`, backspace to empty and press `a` again
and confirm it appends rather than repainting the band, press an arrow key and
confirm nothing moves, press Ctrl-C and confirm scrollback intact.

The pty harness the earlier specs left open is still available and is now more
attractive than it was, because `paint_label()`'s output depends on `cols` and a
harness can assert both the byte stream and the arithmetic against a pty of a
known width. It is still declined here for the reason it was declined before: the
criteria are cheap to check by eye, and nothing in this design needs a test-only
seam. There are no unit tests yet, by decision.