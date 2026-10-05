## Story

Doug runs `./dre` and gets a near-black screen. He presses `a` and a grey
square appears at the right-hand end of his row. He presses `A` and a lighter
grey square appears at the left-hand start of the row, sliding everything else
one place to the right. Now he can see, at a glance, which squares he put at
the front and which he appended. Happy, he presses Ctrl-C and gets his terminal
back.

## Acceptance Criteria

1. Pressing `a` adds exactly one square at the right-hand end of the row, in
   `#3F3F46`.
2. Pressing `A` adds exactly one square at the left-hand start of the row, in
   `#2A2A2E`.
3. Pressing `A` slides every square already on screen one place to the right to
   make room; none is overwritten, lost, or changed colour.
4. Every square placed by `A` sits to the left of every square placed by `a`.
5. Squares are laid out from the top-left of the terminal across each line; when
   a line is full, the next square goes on the following line, and squares
   already drawn do not move.
6. Ctrl-C still returns Doug to his shell with his scrollback intact.

## Technical Design

### Position belongs to the terminal, except when it doesn't

003 made "position belongs to the terminal" load-bearing: `paint_square()`
emitted no position, no `TIOCGWINSZ`, no geometry state anywhere. AC3 and AC4
here break that, because prepending needs to know the whole screen. This story
takes position back for one key and gives it back for the other.

The decision that makes it cheap is that **the run is homogeneous.** AC4
guarantees every `#2A2A2E` square sits left of every `#3F3F46` square, so the
stream never interleaves. Shifting the picture one square to the right is
therefore indistinguishable from overwriting the leftmost square of the tail and
appending a fresh one at the end. Same pixels, four writes, no redraw.

The stream is fully described by two counters, `n_A` and `n_a`. There is no
cell buffer and no per-cell state, because there is never any order to
remember. This is the buffer 002 said "earns a place when a feature needs to
know what is already on screen" — it does not, so it still does not get one.

### Two counters, and which square gets overwritten

It is not index `0`. After `a A a A A` the stream is `[A,A,a,a,a]`; painting at
the origin would overwrite an `A` with an `A`, change nothing, and grow the
`a`-run by one. So on `A`:

- the new `#2A2A2E` square goes at stream index `n_A` (overwriting the first
  `a`-square),
- one `#3F3F46` square is appended at stream index `n_A + n_a`.

Counters are incremented after the writes, so every access reads the pre-press
value. `a` needs no index and no position at all: it appends wherever the
terminal's own cursor already sits, exactly as 003 left it, and only bumps
`n_a`.

### Addressing the tail

Stream index `i` is cell `2*i`, which is row `2*i / cols + 1`, column
`2*i % cols + 1`. That is *exactly* the mapping the terminal's own cursor
already uses, wrap included, so a targeted write at index `i` reproduces the
sequential layout cell for cell. The odd-column raggedness 003 documented is
therefore inherited unchanged, not worsened.

`cols` is queried once in `enter()` with `TIOCGWINSZ` and stored in a global.
This is the first dependence on a query that can fail, which 003 declined
twice. Considered and declined:

- **Querying per press.** Correct under resize, but `ioctl` runs on every
  keypress including dropped keys, for a failure mode that only appears when
  Doug resizes.
- **Re-querying on `SIGWINCH`.** 001 refused a signal handler to keep a single
  exit path with nothing async-unsafe to clean up. A `volatile sig_atomic_t`
  flag store is async-safe so that argument is weaker, but it adds a second
  `ioctl` path to get right for one key.

Neither is worth it. Staleness is documented below.

The rejected alternative worth recording is the one that avoids geometry
entirely: save the cursor before homing, repaint the `A`-run plus the new
square, restore the cursor, then advance one square by writing two spaces in
the canvas colour — which repaints cells that are already `#0A0A0B`, so the
advance is invisible and wraps for free. It is genuinely cheaper and it works
on any terminal. It was declined because it leans on "every cell past the tail
is `#0A0A0B`" — true from 001, but it becomes load-bearing, and it needs three
escape sequences the program has never emitted (`ESC[s`, `ESC[u`, and a
canvas-pen write) for a key Doug presses rarely. `A` is not the hot path.

### Painting

`paint_square()` becomes `paint_square(int r, int g, int b)`. Both story
colours appear at call sites, so it is no longer a single-use literal:

```
paint_square(63, 63, 70);   // #3F3F46, from `a`
paint_square(42, 42, 46);   // #2A2A2E, from `A`
```

Decimal rather than hex because truecolor SGR is decimal-only
(`48;2;R;G;B`, 0–255); no terminal accepts `#3F3F46` in the sequence. A single
packed `0xRRGGBB` int split at paint time was considered and declined — it puts
the hex in the source but forces a shift-and-mask where three parameters
suffice, and the two story colours are `R=G=B` regardless.

This costs the function its stack buffer: the SGR is no longer a compile-time
constant, so it is `snprintf`-ed and `write`-n with a computed length. `#0A0A0B`
in `enter()` stays a literal — it is written once and never needs to vary.

### What was given up

**AC3 as written.** "none is overwritten, lost, or changed colour" is true of a
sliding implementation and is not physically true of this one: the leftmost
`a`-square's cells *are* repainted, in the colour they already held. The
guarantee that survives and is what Doug actually sees is that after the press
every square he placed with `a` is still on screen in `#3F3F46` and the new
`A`-square is left of all of them. AC4 says the same thing from the other side.

**Stale geometry.** `cols` is read once. If Doug resizes mid-session, every
square already drawn sits in a cell our arithmetic no longer agrees with, and
the next `A` paints in the wrong place. `a` is unaffected — it follows the
cursor. The fix is `ioctl` per press or a `SIGWINCH` flag; both were declined
above.

### Components

Still one `main.c`, still no headers. `enter()` gains the `TIOCGWINSZ` call and
one global; `restore()` is untouched; `paint_square()` gains three parameters;
`main()`'s loop stays a plain byte dispatcher — `0x03` breaks, `'a'` paints,
`'A'` prepends, everything else is dropped, exactly as before. Two new statics,
`n_A` and `n_a`, both zero at entry.

### Testing

None, continuing from 001 through 003. The acceptance criteria are judged by
hand in a real terminal: press `A` a few times and confirm the lighter squares
pile up at the left and the grey run stays intact; press `a` after `A` and
confirm the new grey square lands at the end rather than next to the
prepends.

The pty harness all three earlier specs left open remains available, and this
is the first story where it would earn its keep: the four writes and their
order are exactly what a byte-stream assertion would check. Still no test-only
code is needed in `main.c` for it.
