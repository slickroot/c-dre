## Story

Doug runs `./dre` and presses `a`. The band appears with the terminal's own
cursor already sitting in it, waiting where his first letter will go. He types
`deploy` and the cursor rides along at the end of the word, one letter behind
his typing. He can see exactly where the next letter will land.

## Acceptance Criteria

1. Doug runs `./dre`, presses `a`, and before typing anything the terminal's
   own cursor is visible on the band's middle row, at the position where his
   first character will appear.
2. Each character he types appears in the band and the cursor stays immediately
   after the last character typed.
3. While it is visible the cursor is always on the band's middle row, inside
   the band.
4. The text stays horizontally centred after every keystroke and the cursor goes
   with it, within one cell.
5. Backspace deletes the last character and the cursor moves back one cell with
   it.
6. Typing changes no colours: the band stays 3 rows of `#3F3F46` full width, the
   text stays `#C9C9CF`, everything outside the band stays `#0A0A0B`.
7. ~~When the label has grown to fill the terminal's full width there is no cell
   left for the cursor, so the cursor is not visible.~~ **Dropped.** The cursor
   stays visible at every label width. See Technical Design.
8. Pressing an arrow key still types its trailing bytes into the label, exactly
   as it does today — moving the caret is a separate story.
9. Ctrl-C returns Doug to his shell with his scrollback intact and a visible,
   usable cursor.

## Technical Design

The whole program is one `main.c`, five statics, and one paint function
(`paint_label()`). No new components, no new state, no new files. The caret is
not an object: it is a consequence of the byte order of a repaint that already
happens on every keystroke.

### The caret is implicit

`paint_label()` already ends with the label bytes as the last thing it writes
(`main.c:62`), and nothing moves the cursor afterwards. The terminal therefore
leaves the hardware cursor exactly one cell right of the last character, on
whatever row the last CUP addressed. That is AC2, and it is already true. The
only change needed for AC2 is to stop hiding the cursor.

The repaint addresses row 2 exclusively — `ESC[K\r\n` walks down the three band
rows, then `ESC[2;%dH` (`main.c:58`) puts the caret back on the middle row
before the text. So the implicit caret row is the middle row by construction:
AC3 needs no code either.

AC4 holds because the caret column is derived from the same `(cols - len) / 2`
expression as the text start, one addition apart. Centring cannot drift from
the caret because there is only one computation.

### Who shows the cursor

- `enter()` keeps `ESC[?25l` (`main.c:24`). The empty canvas at startup keeps
  today's behaviour: no blinking cursor on a blank screen.
- `paint_label()` appends `ESC[?25h` to its `prefix` byte string
  (`main.c:49-55`), so it is re-asserted on every repaint rather than tracked
  with a `cursor_shown` static. Five bytes per keystroke buys the elimination
  of a flag that could disagree with reality.
- Nothing ever hides it again. `restore()` is untouched: the cursor is already
  visible, and `ESC[?1049l` returns it to the primary screen's saved position.
  That is AC9.

Visibility is therefore owned entirely by `paint_label()`, and the input loop
knows nothing about it.

### Why AC7 was dropped

The scenario AC7 describes cannot actually reach a cell outside the band. The
input guard admits a character only while `len < cols` (`main.c:84`), and
`paint_label()` early-returns when `len >= cols` (`main.c:47`). The last repaint
that ever runs is at `len == cols - 1`, where the text starts at column
`(cols - (cols - 1)) / 2 + 1 == 1` and the implicit caret lands on column `cols`
— the final cell of the middle row, still inside the band.

So the caret is always inside the band at every reachable label width. Hiding
it in the one case where it would sit outside the band would require code for
a state the program cannot reach. AC7 is struck rather than implemented.

### Consequences

- AC8 is untouched: arrows still arrive as `ESC [ C`, the `ESC` is dropped as
  non-printable and `[C` is stored, exactly as spec 006 decided. The caret
  rides along after those bytes like any other character.
- AC5 and AC6 are untouched: backspace already repaints, and no colour byte
  changes. The added `ESC[?25h` sits alongside `ESC[1m` in the same prefix.
- `cols == 0` still early-returns (`main.c:45`), so the show is never emitted
  when the window size was never known.

