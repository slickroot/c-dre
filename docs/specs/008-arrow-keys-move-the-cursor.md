## Story

Doug runs `./dre`, presses `a`, and the band appears with the cursor waiting after the word. He types `deploy`, presses Left twice, and the cursor moves back two characters. He types `x` and it lands at the cursor, giving `deplxoy` — nothing he already typed is lost.

## Acceptance Criteria

1. Left and Right each move the cursor exactly one character, once per press.
2. Typing with the cursor in the middle inserts at the cursor and shifts the letters to its right along: `deploy` becomes `deplxoy`. Nothing already typed is ever lost.
3. Left at the start and Right at the end leave the cursor where it is.
4. Left and Right never change the text — the `[D` that Left types today no longer appears.
5. Backspace deletes the character left of the cursor and moves the cursor back with it.
6. Everything else is unchanged: the word stays centred, colours stay the same, the full-width limit still applies, and Ctrl-C still returns Doug to his shell with a usable cursor.

## Technical Design

This is the story 006 and 007 both deferred. 006 said caret movement "belongs
with caret movement, which needs the same parsing and has somewhere to put the
result"; 007 made the caret implicit because there was no position to place.
Both debts land here.

### The caret becomes state, and is placed by explicit address

007 built the caret as a consequence of byte order: `paint_label()` wrote the
label last and moved nothing afterwards, so the terminal left the hardware
cursor one cell past the end. That is AC2 of 007 and it works only because the
caret is *always* at the end.

**Decision: add `static int cursor`, an index into the label, always in
`0..len`.** It is initialised to `0` beside `len`, set to `0` again when `a`
summons the band (the label is empty then, so that is free), and thereafter
only ever moved by the four branches below.

007's implicit trick is **deleted**. Two variants of keeping it were considered
and rejected. Writing the left run, then a CUP, then the right run leaves the
terminal sitting after the *whole* label, not at the caret — it does not work at
all. Writing the right run first, then the CUP, then the left run does work, and
was the option preferred during this meeting, but it puts the caret position in
a write ordering a reader has to reverse to understand. Ten bytes per keystroke
buy a position that is stated, computed from the same expression as the text
start, and checkable by reading one line.

So `paint_label()` gains a final move, after the label bytes and the `ESC[0m`
reset:

```
ESC[2;%dH   with (cols - len) / 2 + cursor + 1
```

The text start is `(cols - len) / 2 + 1`, so the caret column is the text start
plus `cursor` — one expression, one `snprintf`, already in scope from 006. The
label is still written in a single run; it is only the *caret* that is addressed.

Nothing else in the paint sequence changes: same band colour, same text
foreground, same three `ESC[K` erases, same `ESC[?25h`, same `ESC[0m`. AC6 of
this story ("colours stay the same") is satisfied by not touching those bytes.

### The caret is clamped to `len`, and `len` still caps at `cols`

Four branches, and each is a bounds test rather than an assignment:

- **Left** (`D`): `if (cursor > 0) cursor--;` then repaint.
- **Right** (`C`): `if (cursor < len) cursor++;` then repaint.
- **Printable** `0x20`–`0x7e`, while `len < cols`: `memmove(&label[cursor + 1],
  &label[cursor], len - cursor)`, then `label[cursor] = byte`, `len++`,
  `cursor++`, `label[len] = 0`, repaint. The `memmove` is AC2 — it is what
  makes `deploy` into `deplxoy` instead of losing a letter.
- **Backspace** `0x7f`/`0x08`, while `cursor > 0`: `memmove(&label[cursor - 1],
  &label[cursor], len - cursor)`, then `len--`, `cursor--`, `label[len] = 0`,
  repaint. AC5.

Clamping Left at `0` and Right at `len` is AC3. Note Right clamps at `len`, not
at `cols - 1`: there is no separate "one past the end" position, because with
`cursor` addressed explicitly the caret after the last character is just
`cursor == len`, and painting it uses column `(cols - len) / 2 + len + 1`, which
is inside the band for every `len < cols`.

Backspace is AC5 and is now cursor-relative rather than tail-relative. The
existing `len > 0` test becomes `cursor > 0`, and the `memmove` makes it delete
the character to the *left* of the caret and close the gap. Backspace at
`cursor == 0` does nothing at all, where before it deleted the last character.

The `len < cols` guard on insertion is unchanged from 006, and this story does
not revisit it. The consequences were argued then and stand: characters typed
at the boundary are dropped, `paint_label()` early-returns at `len >= cols`, and
the band goes blank while the caret keeps moving invisibly inside it. Left and
Backspace both recover from it. 006 already called this "loud but
recoverable", and AC6 of this story says the full-width limit still applies —
so the rule is kept rather than re-litigated.

### The parser is the terminal's parser, and it needs three states

AC4 requires that `[D` stops appearing in the label. The bytes arrive as
`0x1b 0x5b 0x44`, and 006 already established that `0x5b` and `0x44` are both
inside `0x20`–`0x7e`, which is why they were stored as text.

The fix is not a special case for `[D`. It is to stop treating a byte as text
once we know it is inside a sequence, and the rule for knowing that is the
terminal's own.

A terminal reading `0x1b` commits to reading a control sequence and stops
considering the following bytes as text. `ESC [` is the *control sequence
introducer* — and this special case is the part that matters, because `[` is
`0x5b` and `0x5b` sits inside the final-byte range `0x40`–`0x7e`. So the naive
rule "swallow until a byte in `0x40`–`0x7e`" is **wrong**: it would terminate on
`[` and treat `D` as a fresh character, which is AC4's bug with extra steps.
Parameter bytes (`0x30`–`0x3f`) and intermediate bytes (`0x20`–`0x2f`) are
consumed inside the sequence, and only a final byte (`0x40`–`0x7e`) ends it.

**Decision: `static int seq`, with three values.**

- `seq == 0` — normal. A `0x1b` sets `seq = 1` and consumes nothing else.
- `seq == 1` — saw `ESC`, expecting the introducer. `0x5b` (`[`) means a
  parameterised sequence and sets `seq = 2`. Any other byte is a two-byte
  sequence: it is the final byte, it is dispatched, and `seq` returns to `0`.
- `seq == 2` — inside `ESC [`. Bytes `0x20`–`0x3f` are parameters, consumed and
  discarded. A byte `0x40`–`0x7e` is the final byte: dispatch it and return to
  `seq == 0`. A `0x1b` re-enters at `seq == 1` (a real sequence can be
  abandoned mid-stream by tmux and screen redraws).

Dispatch is on the final byte alone, because that is the only thing the two
keys this story needs differ by: `C` moves right, `D` moves left, **every other
final byte does nothing**. Parameters are read and thrown away, so `ESC [ 3 ~`
(Delete), `ESC [ H` (Home) and `ESC [ F` (End) are consumed and ignored rather
than typed into the label as `3~`, `H` and `F`. That is the terminal's
behaviour and it is strictly better than what happens today, where those keys
type their own names into the band.

This reverses 006's central input decision and should be read as such. 006
rejected exactly this parser — "an earlier draft tried to swallow sequences whole
… it ate a character Doug had typed" — and 006's reason for rejecting it now
does not apply. 006 rejected it because there was nowhere to put a sequence
result, so consuming `ESC [ 3 ~` would only have destroyed Doug's text. There is
a caret now, so consuming it means "the caret does not move", which is a
defensible outcome rather than data loss.

**The ambiguity is resolved in favour of "sequence", by design.** `Escape` then
`[` then `D` sent by hand is byte-identical to the Left key, and this parser
treats it as Left. There is no timing heuristic and no timeout, which is the
same trade 006 recorded and then had no use for. The cost is that a hand-typed
`ESC [ a` loses the `a`, because `0x61` is a valid final byte. The benefit is
that the program never types a `D` or a `[` into Doug's word.

**Ctrl-C always exits.** `0x03` is tested before anything else in the loop,
exactly where it is today (`main.c:78`), so it is honoured wherever it appears —
including mid-sequence, where it is below the final-byte range and would
otherwise be swallowed as payload. This is not treated as a design question: a
program whose only guaranteed exit depends on no earlier byte having been `ESC`
is a program that looks unkillable, and AC6 requires Ctrl-C to work. Backspace
is *not* given the same carve-out — `0x7f` is above `0x7e` and so would be a
final byte anyway, and the worst case is a keypress that does nothing after a
stray `Escape`.

### Flat, as 001 through 007

No `struct`, no `text_insert()`, no module seam. The loop owns `label`, `len`,
`cursor`, `seq`, `band_drawn` and `cols`, and `paint_label()` reads them all
with no arguments, exactly as it has since 006.

Extracting a text-buffer type with `insert`/`backspace`/`left`/`right` methods
was considered. It is the better shape for six pieces of related state with
invariants, and it is declined for one reason: this program has no test harness
(see Testing), so an extracted component buys no assertability, only a second
file to read. The loop is where 001–007 put the state and the story keeps that
convention. The cost is five statics and three `memmove`-shaped operations in
`main()`.

### Components

Still one `main.c`. Changes only:

- `static int cursor` — **new**, the caret index, `0..len`
- `static int seq` — **new**, the parser state, `0`/`1`/`2`
- `enter()` — unchanged, except `cursor` is implicitly `0` at startup
- `restore()` — unchanged
- `paint_label()` — unchanged through the label write and `ESC[0m`; **adds** the
  final `ESC[2;%dH` for the caret. Still early-returns on `cols == 0` and on
  `len >= cols`, so the caret column is never emitted at full width
- `main()` — the loop gains the `seq` state machine and the two `memmove`s;
  `0x03` stays the first test; `a` still summons the band once without appending

No new includes. `memmove` is in `<string.h>`, which is not currently included —
that is the story's only new dependency, and `string.h` rather than
`<stdlib.h>`'s implicit declarations.

Note that the Left/Right branches and the printable branch all repaint, so the
repaint stays the thing the input handler calls. `paint_label()` still owns its
own position, colours and reset; the caret is simply one more absolute move at
the end.

### Testing

None, continuing from 001 through 007. Hand-judged in a real terminal: press `a`,
type `deploy`, press Left twice and confirm the caret is between `l` and `x` with
the word still centred, type `x` and confirm `deplxoy` with the caret between
`x` and `o`, press Left four times and confirm it stops at the start without
deleting anything, press Right seven times and confirm it stops after `y`,
press Backspace and confirm it deletes `x` and the caret moves with it, press
Home and Delete and confirm they type nothing, press Left at the start and
confirm `x` is inserted at position 0, type past the terminal width and confirm
the band blanks, Backspace until it comes back, and press Ctrl-C and confirm
scrollback intact.

The pty harness the earlier specs left open remains available and remains
declined for the reason it was declined twice: `CONTRIBUTING.md` forbids running
the app, and no criterion here needs a seam that only a test would use. This
story makes the output stream more `cols`-dependent than before, which is the
argument that finally might carry it — but carrying it is a separate decision
from this one, and it is not taken here.
