## Story

Doug runs `./dre` and presses `a` — the grey band appears. He types `deploy` and
each letter shows up in the middle of the band, in the lighter grey text colour,
sliding sideways so it stays centred as the word grows. His first label on the
canvas.

## Acceptance Criteria

1. Doug runs `./dre`, presses `a`, then types `deploy`.
2. The band is 3 rows of `#3F3F46`, full terminal width, at the top.
3. Nothing is on the band until he types.
4. Each character appears on the middle row as he types it, so `deploy` reads
   left to right once he's done.
5. The text is `#C9C9CF`.
6. The text stays horizontally centred after every keystroke, within one cell
   when the width doesn't divide evenly.
7. Typing changes no background: the band stays `#3F3F46` on all 3 rows, and
   every cell outside it stays `#0A0A0B`.
8. No cursor is visible.
9. Non-printable keys other than Ctrl-C do nothing and show nothing.
10. Ctrl-C returns Doug to his shell with scrollback intact.

## Technical Design
