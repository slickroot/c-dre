# delete-selected-row

Doug is looking at his diagram with a few rows stacked on it. He moves his highlight onto a row he no longer wants and presses `d`. The row disappears, and the rows underneath move up one row so the diagram stays stacked with no gap.

## Acceptance Criteria
- While Doug is moving around (not typing), pressing `d` deletes the highlighted row.
- The deleted row disappears and the rows underneath it move up one row to close the gap, so the remaining rows stay stacked with no empty row between them.
- After the deletion, the highlight lands on the row that was above the deleted one.
- If the deleted row was the topmost row, the highlight lands on the row below it.
- If the deleted row was the only row, the screen is left empty with nothing highlighted.
- If nothing is highlighted, pressing `d` does nothing.
- While Doug is typing text into a row, pressing `d` types the letter "d" and does not delete the row.

## Technical Design
