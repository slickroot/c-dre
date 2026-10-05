## Story

Doug opens the app, presses `a` to summon a label, and types some text. He presses `Esc` to leave typing mode. Curious, he presses a few other keys, but nothing is added to the label. Satisfied, he presses `i` to return to typing mode and continues typing right where he left off.

## Acceptance Criteria

1. Pressing `Esc` while typing exits typing mode.
2. While out of typing mode, pressing keys does not modify the text in the label.
3. Pressing `i` returns to typing mode, resuming text entry at the current cursor position.
4. Pressing `Ctrl-C` exits the application while in or out of typing mode.

## Technical Design
