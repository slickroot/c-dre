## Story

Doug runs the benchmark and it presses `a` and `A` at itself on his real
terminal, thousands of times each, then tells him how long each key took to
actually appear on screen — side by side. Happy, he finally knows whether the
prepend key is worth what it cost him.

## Acceptance Criteria

1. Doug runs one command and it finishes on its own, without him pressing any
   keys.
2. It reports one number for `a` and one for `A`, in the same unit, so they sit
   next to each other and can be read as a comparison.
3. Each number comes from many presses, not a handful.
4. Each number measures the time until the change is visible on Doug's screen —
   not just the time to write bytes into a pipe.
5. Both numbers come from the same run: same terminal, same window size, same
   number of presses each.

## Technical Design

### The seam

The story needs the program to press keys at itself, so the press logic has to
stop being tangled up with the fact that keys arrive on stdin. One function
comes out:

```c
static void press(char key);
```

Its body is `main.c:66-78` verbatim, moved out of the `while` loop. It knows
three things — `n_a`, `n_A`, `cols` — and it does one thing: paint and bump the
counters. It does not read, and nothing about it changes. Both callers use it:

- the interactive loop, which is `main.c:63-79` with the two branch bodies
  replaced by `press(byte)`
- the benchmark, which calls it in a loop with no stdin involvement at all

Everything below is scaffolding on the bench side of that seam. The shipped
path is unchanged, byte for byte.

### New state

`enter()` (`main.c:12`) caches `cols` from `ioctl` at `main.c:14-15` and never
looks at height. This story needs height — it is what bounds the run — so
`rows` joins `cols` as a global and both come from the same `winsize`. This is
the first time the program knows how tall the terminal is. `cols` is still read
exactly once, so a resize mid-run is not noticed; AC5 fixes the window size for
the duration, and nobody resizes a window during a 900-press run.

### How long the run is

Every press appends exactly one cell to the ribbon, whatever the key, so after
`P` presses the ribbon is `P` cells long and the last one sits at row
`2*(P-1)/cols + 1`. It is on screen while that row is `<= rows`:

```
P_max = (rows - 1) * cols / 2 + 1
```

`P_max` is where the bench stops. Not a chosen constant, not a fixed
thousand — the terminal being full is the end of the benchmark. On an 80x24
that is about 920 presses, which is AC3's "many, not a handful" by a wide
margin and happens to be exactly the range where the feature is usable.

Presses come in pairs, `a` then `A`, and `P_max` is rounded down to an even
count so both keys are pressed identically many times. AC5 asks for that, and
the pairing makes it true by construction instead of by bookkeeping.

Interleaving is also what keeps the comparison honest. A run of all `a` presses
measures `a` against a growing ribbon; a run of all `A` presses measures `A`
against an empty one. Alternating means press 400 of `a` and press 400 of `A`
both happen with a 400-cell ribbon behind them.

It also settles the warmup question for free. The first `A` on an empty screen
costs four writes rather than six, because the second pair at `main.c:74-75` is
gated on `n_a > 0`. Since the first press of the bench is an `a`, `n_a` is
already 1 before any `A` runs, and no sample is ever taken in the degraded
state.

### What ends the stopwatch

This is the AC4 problem and it is the real content of the story. `write()`
returning means the bytes reached the tty driver, which is the thing AC4 rejects.
Nothing inside the process can see photons.

The mechanism is DEC private mode 2026, synchronized output. Each press is
framed:

```
ESC [ ? 2026 h      <- start of frame
  <the press's own writes>
ESC [ ? 2026 l      <- terminal presents the frame here, atomically
```

A terminal that implements 2026 buffers everything between the `h` and the `l`
and presents it as one frame at the `l`. Timing to the `l` is then a genuine
bound on "this frame is complete and will appear as a unit" — not a guess, and
not a pipe.

The watch is deliberately *outside* the scaffolding:

```c
if (sync) write(STDOUT_FILENO, "\x1b[?2026h", 8);
t0 = now_ns();
press(key);
if (sync) write(STDOUT_FILENO, "\x1b[?2026l", 8);
sample = now_ns() - t0;
```

Start after the `h` returns, stop after the `l` returns. In `--bench=nosync`
the two `if`s vanish and the same three lines measure a bare `press()`. Both
modes therefore report the same quantity — time to present this press — and the
ruler is not part of the measurement. Starting the watch before the `h` instead
would time `a` over 5 writes and `A` over 8; the scaffolding constant is equal
on both sides but it shrinks the measured ratio, and we would be reporting a
smaller gap than the feature actually costs.

`now_ns()` is `clock_gettime(CLOCK_MONO)`, which means adding `<time.h>` to
`main.c:1-5`. `CLOCK_MONO` and not `CLOCK_REALTIME`: an NTP step mid-run should
not produce negative latencies.

### We cannot detect 2026

There is no in-band way to ask a terminal whether it implements a private mode,
and there is no timing signature that distinguishes a buffered frame from an
unbuffered one — `write()` returns at the same speed either way. Probing would
be a guess wearing a lab coat.

So the mode is Doug's to declare:

- `dre --bench` — assumes 2026
- `dre --bench=nosync` — write-returns fallback, for Terminal.app, iTerm2,
  older xterm

The report names the mode and `$TERM`, so a terminal that quietly drops private
modes shows up as a mismatch in the output rather than only as a wrong number in
Doug's head. This mitigates the failure; it does not eliminate it.

### The two numbers

One statistic per key, and it is the median. Means are the obvious default and
the wrong one: write latency is right-skewed, and one terminal hiccup in a
900-press run drags the mean of whichever key happened to be in flight. With
interleaving that is a coin flip, and the two means could land within noise of
each other for entirely the wrong reason. The median reports the typical press
instead of the worst one.

Unit is microseconds, same for both keys. AC2 says one number each so they sit
next to each other and can be read as a comparison, so there is no p95 column
hiding in the corner — a tail figure would be a second number per key, and a
different deliverable than the story asked for. If the two medians come out
close, "the prepend costs about what it costs" is a real answer and the honest
thing to report.

Samples are held in two `malloc`'d arrays of `long long`, one per key, sized to
the press count, and each is sorted in place to take its middle element (the
mean of the two middle values, since the count is even).

### Output

`restore()` is already on `atexit` (`main.c:59`), so the alt screen is left on
the way out and the report prints into normal scrollback afterwards. Three
lines to stdout:

```
a: 41 us (median of 920)
A: 118 us (median of 920)
mode: synchronized output (2026)   TERM=xterm-256color
```

The press counts are printed because AC5 claims the two runs matched; printing
them lets Doug check the claim instead of taking it. The mode line exists for
the reason above.

### Shape of the code

`main.c:53` becomes `int main(int argc, char **argv)` and dispatches on a single
optional argument — the program has no other argv handling and does not grow
one. Unknown arguments are a usage error on stderr and exit 2. The `isatty`
guard at `main.c:54-57` is untouched: the bench runs against Doug's real
terminal, which is the entire point.

New functions, all of them bench-side:

| function | knows | does |
| --- | --- | --- |
| `press(key)` | `n_a`, `n_A`, `cols` | paints one square, updates counters |
| `now_ns()` | nothing | monotonic nanoseconds |
| `median(v, n)` | an array | sorts in place, returns the middle value |
| `run_bench(sync)` | `cols`, `rows` | times every press, prints the report |

Collaborators: `run_bench` is the only caller of `now_ns`, `median`, and the
2026 wrapper; the interactive loop is the only other caller of `press`. Nothing
in the bench path is reachable from an interactive session.

### Testing: None

Consistent with 001 through 004, and here the usual deferral has a real
justification rather than being a habit. The pty harness those specs keep
deferring would allocate a pty, run the binary on the slave, and assert on the
byte stream. Asserting on bytes is the right tool for the four writes at
`main.c:71-75`. It is the wrong tool here, because from outside a pty all we
can time is the write to the master — which is precisely the measurement AC4
rejects as "bytes into a pipe." A harness could only ever confirm that the
report has the right shape.

The report is the artifact, and Doug reads it. That is the whole verification
story, and it is a weak one, which is worth naming rather than hiding.

### Rejected

- **A second `bench.c` binary driving `dre` over a pty.** This is the obvious
  design and it is worse on every axis. `flake.nix:36` whitelists only
  `main.c`, so a new source file is silently dropped from `nix build` until
  that line changes; we would ship two binaries for one tool; and it cannot
  satisfy AC4, because from outside the pty the only observable moment is the
  write to the master.
- **`write()` returning as the stop condition.** AC4 rejects it by name.
- **Clearing the canvas every K presses and resetting the counters.** Doubly
  wrong. It starts the run over on a schedule, which is exactly what Doug
  objected to; and it cannot recover what resetting throws away anyway, because
  the number of writes per press is fixed at `main.c:66-78` and there is no loop
  over the stream to grow. Worse, a full clear puts the terminal into a full
  repaint immediately before the next timed press.
- **Wrapping rows modulo the window height.** Keeps the counters growing but
  index 0 and index `rows*cols/2` land on the same cell, so prepend writes
  clobber unrelated squares. AC4 stops being true for `A` in a way that cannot
  be written around. It also means teaching `paint_at` (`main.c:46-51`)
  benchmark-only behavior on the path the real app uses.
- **Clearing when the terminal fills, then continuing.** A compromise that
  gives up the growth data *and* reopens the "starting over" problem. The run
  simply ends.
- **Blocks of all-`a` then all-`A`.** Two different experiments in one report;
  see the interleaving argument above.
- **Mean instead of median.** Right-skewed latency, and interleaving makes the
  skew land on a coin flip.
- **The 2026 wrapper inside `press()`.** It would give the interactive app
  atomic frames for free, which is tempting, but this story was asked a
  question about `a` versus `A`, not a request to retrofit synchronized output
  into the editor. The wrapper is measurement scaffolding and belongs on the
  bench side of the seam. If atomic frames turn out to be worth having in the
  app, that is its own story and its own spec.
- **Probing whether the terminal honors 2026.** Not detectable. See above.
- **Starting the stopwatch before the `h`.** Times the ruler along with the
  press, and compresses the reported ratio.
- **Also writing a `result` file.** `.gitignore:2` reserves that name, which
  suggests it was once intended for this. The story asks for one command that
  finishes on its own and mentions no artifact. Trivially added later.
- **Staying on the alt screen to show the report.** Breaks the restore
  discipline the last four specs were careful about, and a killed process
  leaves Doug with no terminal. That failure is precisely what
  `atexit(restore)` exists to prevent.

### What this gives up

The headline number is time-to-present under an atomic frame, not
time-to-photon. No program can measure photons, and this is the closest honest
approximation available from inside the process. On a terminal that ignores
2026, a plain `dre --bench` measures write-to-tty and reports it as though it
did not — the flag and the printed `$TERM` reduce how likely that is to go
unnoticed, and do not make it impossible.

There is also no data beyond one screenful. Whether a press gets slower as the
stream grows can only be observed across the first `P_max` presses, which is
the entire range where prepend is geometrically real: `A` repaints the head at
`main.c:71` and the tail at `main.c:74`, `n_A` cells apart, and once `n_A`
exceeds the window's width those two cannot both be on screen. A thousand-press
run showing prepends is not something a finite terminal can display at all.
The short canvas is not a limitation of the benchmark. It is the regime the
feature was built for.
