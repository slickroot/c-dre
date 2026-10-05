## Story

Doug runs the benchmark and it presses `a` and `A` at itself on his real
terminal, thousands of times each, then tells him how long each key took to
actually appear on screen — side by side. Happy, he finally knows whether the
prepend key is worth what it cost him.

## Acceptance Criteria

1. Doug runs one command and it finishes on its own, without him pressing any
   keys.
2. It reports one number for `a` and one for `A`, in the same unit, so they sit
   next to each other and can be read as a comparison.
3. Each number comes from many presses, not a handful.
4. Each number measures the time until the change is visible on Doug's screen —
   not just the time to write bytes into a pipe.
5. Both numbers come from the same run: same terminal, same window size, same
   number of presses each.

## Technical Design
