# stack-second-text-in-tall-row

Doug has made a row on his diagram 5 lines tall, with "Login" centered in the middle. He presses `Esc` to move around, lands on that row, and presses `o`. "Login" moves up a line, and he's typing right away on a new line below it, with one empty line between them. He types "Logout". Now the row shows "Login" on top and "Logout" underneath. Both are centered, and the pair sits in the middle of the row. Happy with how neat it looks, he moves on.

## Acceptance Criteria
- While Doug is moving around, pressing `o` on a row taller than 1 line adds a second text underneath the first, with one empty line between them.
- Both texts are centered left-to-right, and the pair sits in the middle of the row top-to-bottom. In a 3-line row, they fill it exactly: text, gap, text.
- After pressing `o`, Doug is typing on the bottom line right away, with the caret next to the new text.
- When Doug presses `i` on a tall row with two stacked texts, he types into the bottom text.
- If Doug presses `o` and then `Esc` without typing, the first text stays on top with an empty spot below the gap.

## Technical Design

### Band model (`editor.c`): unchanged
A band still holds up to two texts (`texts[2]`, `count`). Nothing on the band says whether they are stacked or side by side. `layout()` works it out from `pad` every frame:
- `pad == 0` (1-line row): side by side, as in spec 024.
- `pad >= 1` (3+ lines tall): stacked.

So the arrangement follows the height. Growing a 1-line row with two texts stacks them, and shrinking a stacked row back to 1 line puts them side by side again.

`EVENT_ADD_TEXT`, `EVENT_CHAR`, `EVENT_BACKSPACE` and `active_text()` are unchanged. `o` still adds `texts[1]` and enters `MODE_TYPE`, and `i` still types into the last text, which is the bottom one when stacked.

### Layout (`layout.h`): each text carries its own row
```c
struct placed_text {
	int row;
	int col;
	const char *text;
	int len;
};
```

`placed_band.row` stays as the band's center line, and the highlight fill still uses it with `pad`.

`layout()` places each text (1-based):
- **One text:** `row` = band row, `col = (cols - len) / 2 + 1`. Same as today.
- **Two texts, `pad == 0`:** `row` = band row. Left at col `3`, right at col `cols - len - 1`. Same as spec 024.
- **Two texts, `pad >= 1`:** `texts[0].row = row - 1`, `texts[1].row = row + 1`. Both centered: `col = (cols - len) / 2 + 1`. The pair (text, gap, text) sits in the middle of the band whatever the pad is. With pad 1 it fills the band exactly.
- **Caret** (in `MODE_TYPE`): `caret_row = texts[count - 1].row`, `caret_col = texts[count - 1].col + cursor`.

`text_col()` becomes a small helper that fills in both `row` and `col` for a text from the band row, `pad`, `cols`, `len`, index and count.

### Paint (`paint.c`)
`paint_frame` writes each text at `p->texts[t].row` and `p->texts[t].col` instead of `p->row`. It knows nothing about stacking. The highlight fill is unchanged (`row - pad` … `row + pad`).

### Tests (`tests/test_editor.c`)
- `o` on a pad-1 band: the old text is at `row - 1`, the new empty text at `row + 1`, both centered, and the mode is `MODE_TYPE`.
- Typing after `o` on a pad-2 band: "Logout" is centered on `row + 1`, and the caret is on `row + 1` at `col + cursor`.
- `o` then `Esc` on a tall band: the first text stays on `row - 1`, and an empty second text is on `row + 1`.
- `i` on a stacked band types into the bottom text, and the caret is on `row + 1`.
- A single text in a tall band stays centered on the band row.
- Two texts on a pad-0 band are still side by side on the band row (regression).
- Growing a band with two side-by-side texts stacks them, and shrinking it back puts them side by side.
- Existing assertions on `texts[i]` also check `row` where it matters.

### Out of scope
- Long stacked texts wider than the screen: same `cols - 1` buffer limit as today.
