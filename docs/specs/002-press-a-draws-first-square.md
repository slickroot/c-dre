## Story

Doug runs `dre`. His whole terminal fills with the quiet near-black canvas and no cursor blinks anywhere. He presses `a`, and a single grey square appears in the top-left corner. He has his first mark on the canvas.

## Acceptance Criteria

1. Doug runs `dre` and presses `a` exactly once.
2. A `#3F3F46` square appears, 2 cells wide and 1 cell tall, occupying the first two cells of the top row.
3. No cursor is visible anywhere on the screen.
4. Every other cell stays `#0A0A0B`.
5. Ctrl-C returns Doug to his shell with his scrollback intact, exactly as it is today.

## Technical Design
