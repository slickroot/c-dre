## Story

Doug types `./dre` at his prompt. His whole terminal fills with the near-black colour `#0A0A0B`. He presses Ctrl-C, and his terminal is exactly as it was before — nothing of his scrollback lost. Happy, he carries on.

## Acceptance Criteria

1. Typing `./dre` and pressing enter takes over the entire terminal window, filling every cell with `#0A0A0B`.
2. Pressing Ctrl-C returns Doug to his shell.
3. After Ctrl-C, the terminal shows exactly what was there before `./dre` ran, including scrollback.
4. Ctrl-C exits immediately, with no confirmation prompt and nothing else on screen.

## Technical Design

### Environment

- Language: C. Built and run through a Nix flake.
- `flake.nix` exposes `devShells.default` (C toolchain, gdb, strace) and
  `packages.default` producing the `dre` binary. `nix develop` to work on it,
  `nix build` for the binary.
- Terminal handling is hand-rolled against `<termios.h>` and `<unistd.h>`. No
  ncurses, no libvterm, no libtermcap. This story needs about forty lines and a
  zero-dependency C file is worth more than the abstraction.

### Terminal buffer

We enter the alternate screen buffer (`ESC[?1049h`) rather than painting over
the normal one. This resolves the tension between AC1 and AC3: taking over the
window *is* the alt-screen switch, and leaving it is what restores the normal
buffer with scrollback intact. There is no cell-by-cell snapshot of Doug's
scrollback, and none is needed.

Colour is 24-bit truecolor and this is a hard requirement, not a preference.
`#0A0A0B` decomposes to 10, 10, 11. The xterm 256-colour cube has levels 0, 95,
135, 175, 215, 255, and the grey ramp steps 8, 18, 28, ... — 10 is in neither, so
the colour has no palette index and can only be sent exactly.

### Paint

Two escape sequences, independent of window size, so no `TIOCGWINSZ` query is
needed:

1. `ESC[48;2;10;10;11m` — set the default background colour
2. `ESC[2J` — erase to end of screen, painted with that default

The alt-screen buffer arrives already cleared, so there is no unpainted flash
between the switch and the fill. Setting a default background plus an erase
beats writing one SGR per cell, which would be O(rows x cols) bytes and would
need a size query that can fail on an unusual pty.

Exit is `ESC[?1049l` alone. The alt-screen switch restores the buffer *and* the
saved cursor position, so there is nothing to repaint and nothing to say.

### Raw mode

Ctrl-C does not generate SIGINT. `cfmakeraw(3)` clears `ISIG`, so the key
arrives as byte `0x03` in the input stream and our own read loop sees it. That
gives us a single exit path with no signal handler and no async-unsafe cleanup
to get right. `cfmakeraw` is available on glibc, musl and macOS, so we call it
rather than reimplementing its eight flag assignments and drifting from the
platform's definition.

- `tcgetattr` to save the current settings
- `cfmakeraw(&tty)`
- `c_cc[VMIN] = 1`, `c_cc[VTIME] = 0` so `read` returns as soon as one byte
  arrives
- `tcsetattr` to apply

### Components

One `main.c`. No headers, no module seams — the work is small enough that a
split would create boundaries with nothing on the other side. `main.c` holds:

- `enter()` — save termios, apply raw mode, write the three enter sequences
- `restore()` — write `ESC[?1049l`, `tcsetattr` the saved termios back,
  `fflush(stdout)`
- `main()` — the guard, enter, read loop, exit

### Control flow

`main()`:

1. `isatty(STDIN_FILENO)`; if false, message to stderr, `exit(1)`, touching no
   terminal state. A stray `./dre > out.txt` must not spray escape codes into a
   file.
2. `restore()` registered with `atexit()`, so no exit path — normal return,
   error, future early return — can leave the terminal in the alt screen.
3. `enter()`
4. Read loop: `read(STDIN_FILENO, &byte, 1)` one byte at a time. `0x03` breaks
   the loop; every other byte is read and dropped. There is no second feature
   yet for other keys to serve.
5. `return 0` from `main`, `atexit` runs `restore()`.

Exit status is 0. Ctrl-C is the designed way out, so it is a success; this keeps
shell prompts and any future wrapper scripts happy.

### Ordering

Each escape sequence is its own `write()`. `restore()` ends with
`fflush(stdout)` so the leave-alt-screen bytes reach the tty before the process
exits and the shell prints its prompt. AC3 and AC4 both depend on this ordering:
any leak of buffered output past exit means Doug gets his prompt over a
`#0A0A0B` background.

Sequences go to stdout, not `/dev/tty`. Simple, and the `isatty` guard means a
redirected invocation is refused before it can write anything.

### Testing

None for now. The acceptance criteria are judged by hand in a real terminal:
run `./dre`, confirm the window is near-black, press Ctrl-C, confirm the
scrollback is untouched. When tests do arrive, the natural harness is a script
that allocates a pty, runs the binary on the slave, sends `0x03`, and asserts
on the byte stream — no test-only code needs to exist in `main.c` for that.
