## Story

Doug has a few bands on screen, having added them earlier with `a`. In move mode, he presses `k`, and the caret steps up into the band above; he presses `j` a couple of times, and the caret steps back down, band by band, stopping once it reaches the bottommost band. Each band remembers its own caret position, so when he lands back on one he'd typed in before, the caret sits right where he left it. He presses `i` and starts typing — the text lands in the band the caret was resting in, not necessarily the newest one. He presses `Esc`, moves the selection elsewhere with `j`/`k`, and presses `i` again to keep typing in a different band. When he presses `a`, a new band appears below the others and becomes selected, same as before.

## Acceptance Criteria

1. In move mode, pressing `k` moves the caret to the band directly above the currently selected one; pressing `j` moves it to the band directly below.
2. Pressing `k` while on the topmost band does nothing; pressing `j` while on the bottommost band does nothing.
3. The caret is visible only in the selected band, at that band's own last cursor position.
4. Pressing `i` enters typing mode and inserts characters into whichever band is currently selected.
5. Adding a band with `a` makes the new band the selected one.
6. `Ctrl-C` still exits the app and restores the terminal, from move or typing mode.

## Technical Design
