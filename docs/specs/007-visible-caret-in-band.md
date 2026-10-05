## Story

Doug runs `./dre` and presses `a`. The band appears with the terminal's own
cursor already sitting in it, waiting where his first letter will go. He types
`deploy` and the cursor rides along at the end of the word, one letter behind
his typing. He can see exactly where the next letter will land.

## Acceptance Criteria

1. Doug runs `./dre`, presses `a`, and before typing anything the terminal's
   own cursor is visible on the band's middle row, at the position where his
   first character will appear.
2. Each character he types appears in the band and the cursor stays immediately
   after the last character typed.
3. While it is visible the cursor is always on the band's middle row, inside
   the band.
4. The text stays horizontally centred after every keystroke and the cursor goes
   with it, within one cell.
5. Backspace deletes the last character and the cursor moves back one cell with
   it.
6. Typing changes no colours: the band stays 3 rows of `#3F3F46` full width, the
   text stays `#C9C9CF`, everything outside the band stays `#0A0A0B`.
7. When the label has grown to fill the terminal's full width there is no cell
   left for the cursor, so the cursor is not visible.
8. Pressing an arrow key still types its trailing bytes into the label, exactly
   as it does today — moving the caret is a separate story.
9. Ctrl-C returns Doug to his shell with his scrollback intact and a visible,
   usable cursor.

## Technical Design
