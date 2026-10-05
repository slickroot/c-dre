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
