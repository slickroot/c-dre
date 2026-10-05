## Story

Doug runs `./dre` and gets a near-black screen. He presses `a` and a grey
square appears at the right-hand end of his row. He presses `A` and a lighter
grey square appears at the left-hand start of the row, sliding everything else
one place to the right. Now he can see, at a glance, which squares he put at
the front and which he appended. Happy, he presses Ctrl-C and gets his terminal
back.

## Acceptance Criteria

1. Pressing `a` adds exactly one square at the right-hand end of the row, in
   `#3F3F46`.
2. Pressing `A` adds exactly one square at the left-hand start of the row, in
   `#2A2A2E`.
3. Pressing `A` slides every square already on screen one place to the right to
   make room; none is overwritten, lost, or changed colour.
4. Every square placed by `A` sits to the left of every square placed by `a`.
5. Squares are laid out from the top-left of the terminal across each line; when
   a line is full, the next square goes on the following line, and squares
   already drawn do not move.
6. Ctrl-C still returns Doug to his shell with his scrollback intact.

## Technical Design
