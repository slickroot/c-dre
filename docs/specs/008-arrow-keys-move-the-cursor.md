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
