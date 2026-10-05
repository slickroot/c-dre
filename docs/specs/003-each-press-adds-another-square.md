## Story

Doug keeps pressing `a`. Each press adds another `#3F3F46` square directly to the right of the last, and when the row runs out the squares carry on at the left of the row below. He paints his way across the whole screen, top to bottom, and when he finally fills the last row, pressing `a` again leaves his picture exactly as it is. He exits with Ctrl-C, scrollback intact.

## Acceptance Criteria

1. Doug presses `a` any number of times, starting from one square already on the screen.
2. Each press adds one `#3F3F46` square, 2 cells wide and 1 cell tall, directly to the right of the previous square, with no gap between them.
3. Squares on one row form a solid unbroken run of `#3F3F46` starting at the first cell of the row.
4. When the current row cannot fit another square, the next square is drawn at the first cell of the row directly below, and filling continues left to right again.
5. No cursor is visible anywhere on the screen at any point.
6. Every cell Doug has not drawn on stays `#0A0A0B`.
7. Once Doug has filled every row of the terminal, pressing `a` again leaves the screen completely unchanged, with no message and no error.
8. Ctrl-C returns Doug to his shell with his scrollback intact, exactly as it is today.

## Technical Design
