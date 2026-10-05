# band-no-background

Sam types a line of text into a band and looks at it. The text sits straight on the canvas with nothing colored behind it, like writing directly on the wallpaper. Sam smiles and keeps going.

## Acceptance Criteria
- Text in a band is drawn directly on the canvas background, with no colored block behind it.

## Technical Design

This is a two-file change: `paint.c` stops painting a band background, and
`main.c`'s `restore()` picks up the cleanup job that `paint.c` gives up. No new
module, no new field on `struct band`, no new event, and no signature changes.

### The band SGR loses its background half

`paint_label` currently emits one escape that sets both the band fill and the
text colour, and later emits `ESC[0m` to undo it. Both halves of that pair go:

```c
static const char ink[] = "\x1b[38;2;201;201;207m"; // text colour #C9C9CF
```

The `\x1b[48;2;63;63;70m` (band fill `#3F3F46`) is deleted, and the
`write(STDOUT_FILENO, "\x1b[0m", ...)` after the text is deleted. The text is
still centred on the middle row and still positioned with the same cursor
escapes; only the SGR that surrounded it changes. The local is renamed `bg` →
`ink` because it no longer describes a background. AC1 falls directly out of
this: the glyphs are written with no fill active, so the canvas colour shows
behind them.

### Erasing the row reuses the wallpaper's background, not a band fill

The band block used to do double duty: it was the visible fill *and* it made
the trailing `ESC[K` erase in band colour. With the fill gone, `ESC[K` erases
to end-of-line in whatever background is currently active. `paint_wallpaper`
sets that background once at startup (`\x1b[48;2;10;10;11m`, canvas `#0A0A0B`)
and, after this change, nothing ever clears it — so the same `ESC[K` sequence
that erased the band now erases back to canvas. The erase sequence itself is
untouched:

```c
static const char erases[] = "\x1b[K\r\n"
                             "\x1b[K\r\n"
                             "\x1b[K"
                             "\x1b[?25h";
```

The deliberate contract introduced here is: **no `ESC[0m` is emitted anywhere
in the paint path.** `paint_label` depends on `paint_wallpaper` having run
first and on nothing resetting the SGR in between. This is the implicit
collaborator in the design — `paint_label` no longer owns its own backdrop, it
borrows the wallpaper's. The ink escape is still written before the erase, but
`ESC[K` reads only the background, so the ordering is harmless.

### `restore()` becomes the single place that resets attributes

Removing the per-paint `ESC[0m` means the last thing the app does before
handing the terminal back is leave canvas `#0A0A0B` and ink `#C9C9CF` active.
`restore()` is the correct owner of cleanup, so it gains one write:

```c
static void restore(void) {
  write(STDOUT_FILENO, "\x1b[0m", sizeof "\x1b[0m" - 1);
  write(STDOUT_FILENO, "\x1b[?1049l", sizeof "\x1b[?1049l" - 1);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_tty);
  fflush(stdout);
}
```

This runs via `atexit` on every exit path, including the Ctrl-C path that 001
and 012 already rely on, so the shell gets its own colours back.

### Components

- `paint.c` `paint_label()` — delete the `48;2;63;63;70` background escape and
  the trailing `\x1b[0m`; rename `bg` → `ink`; keep the ink escape, the centred
  text, the caret positioning and the `erases` sequence unchanged
- `paint.c` `paint_wallpaper()` — unchanged; its background now stays active
  for the life of the session instead of being reset after each band
- `main.c` `restore()` — gains the `\x1b[0m` write
- `paint.h`, `text_buffer.[ch]`, `input.[ch]` — untouched

### Testing

No unit test. `paint_label` writes to `STDOUT_FILENO` directly and the project
has no paint test harness; adding one (a write sink or a stdout redirect) is a
refactor that this story does not justify. The change is hand-judged end to
end: start `dre`, press `a`, type a line, and confirm the text sits straight on
the canvas with no grey block behind it; backspace to shrink the line and
confirm the removed glyphs leave canvas, not band fill, behind; add a second
band and confirm both are clean; quit and confirm the shell's own colours are
restored rather than staying dark.
