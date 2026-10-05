## Story

Doug types `./dre` at his prompt. His whole terminal fills with the near-black colour `#0A0A0B`. He presses Ctrl-C, and his terminal is exactly as it was before — nothing of his scrollback lost. Happy, he carries on.

## Acceptance Criteria

1. Typing `./dre` and pressing enter takes over the entire terminal window, filling every cell with `#0A0A0B`.
2. Pressing Ctrl-C returns Doug to his shell.
3. After Ctrl-C, the terminal shows exactly what was there before `./dre` ran, including scrollback.
4. Ctrl-C exits immediately, with no confirmation prompt and nothing else on screen.

## Technical Design
