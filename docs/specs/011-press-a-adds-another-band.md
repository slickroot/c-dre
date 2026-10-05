## Story

Doug is in move mode, with a band already on screen. He presses `a`, and a new
empty band appears directly below the last one, with no gap between them. The
caret is waiting in the new band, so he starts typing straight away, and his
earlier band keeps its text, untouched. When he's done, he presses `Esc`, and
later presses `a` again to add yet another band below the last. What happens
once the bands reach the bottom of the screen is left for another day.

## Acceptance Criteria

1. While in move mode with at least one band already on screen, pressing `a`
   adds a new band directly below the last band, with no blank row between them.
2. The new band matches the existing bands: 3 rows tall, full terminal width,
   same grey background and same text colour.
3. The new band starts empty, and the visible caret sits in it, centred.
4. The next characters Doug types appear in the new band, centred; no earlier
   band changes.
5. Text already typed in earlier bands stays visible and unchanged.
6. Pressing `Esc` leaves typing mode; pressing `a` again adds another band
   directly below the last one.
7. Ctrl-C still exits the app and restores the terminal.
8. What happens when the bands fill the screen is out of scope for this story.

## Technical Design
