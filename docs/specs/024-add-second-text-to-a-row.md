# add-second-text-to-a-row

Doug has a row on his diagram with "Login" centered in it. He presses `Esc` to move around, lands on that row, and presses `o`. "Login" slides over to the left edge, and he's typing right away in a new text on the right. He types "Logout". Now the row reads "Login" on the left and "Logout" on the right, with space between them. Happy, he moves on to the next row.

## Acceptance Criteria
- While Doug is moving around (not typing), pressing `o` on a row adds a second text to it, and Doug is typing in that new text right away.
- With two texts, the old text is left-aligned and the new text is right-aligned, each 2 columns from the screen edge.
- A row with one text still shows it centered, the same as today.
- Pressing `o` on a row that already has two texts does nothing.
- Pressing `o` on a row whose text is empty works the same way: an empty left text and a new right text.
- If Doug presses `o` and then `Esc` without typing, the row keeps an empty right text and the old text stays on the left.
- When Doug presses `i` on a row with two texts, he types into the right text.

## Technical Design

### Band model (`editor.c`)
A band holds up to two texts in a fixed array:

```c
struct band {
	struct text_buffer texts[2];
	int count;            /* 1 or 2 */
	struct style style;
	int pad;
	struct band *next;
	struct band *prev;
};
```

- `insert_node` initialises `texts[0]` and sets `count = 1`.
- The active text is always the last one, found through a small helper `active_text(band)` that returns `&band->texts[band->count - 1]`. `EVENT_CHAR`, `EVENT_BACKSPACE` and the caret all go through it, so `i` on a band with two texts types into the right text with no extra logic.
- `delete_node` and `editor_free` free all `count` buffers.

### Input (`input.c` / `input.h`)
New event `EVENT_ADD_TEXT`. `input_parse` maps `o` to it in `MODE_MOVE` only.

### Editor behaviour (`editor_apply`)
```c
case EVENT_ADD_TEXT:
	if (e->selected && e->selected->count < 2) {
		text_buffer_init(&e->selected->texts[1], e->cols - 1);
		e->selected->count = 2;
		e->mode = MODE_TYPE;
	}
	break;
```
- If no band is selected, or the band already has two texts, nothing happens: the mode stays `MODE_MOVE` and the band stays highlighted.
- An empty left text is treated like any other text. `Esc` right after `o` leaves an empty right text in place.

### Layout (`layout.h`): bands, not labels
The highlight belongs to the band, so layout describes bands, and each band carries its texts. `placed_label` is renamed:

```c
struct placed_text {
	int col;
	const char *text;
	int len;
};

struct placed_band {
	int row;
	int pad;
	struct style style;          /* dim + highlight: band-level */
	struct placed_text texts[2];
	int count;
};

struct layout {
	struct placed_band bands[LAYOUT_MAX_BANDS];
	int count;
	int caret_visible;
	int caret_row;
	int caret_col;
};
```

`layout()` decides the columns (1-based):
- **One text:** centered, as today: `(cols - len) / 2 + 1`.
- **Two texts:** the left text is at col `3`, the right text at col `cols - len - 1`, each 2 columns from the screen edge.
- **Caret** (in `MODE_TYPE`): the active text's `col + cursor`.

### Paint (`paint.c`)
`paint_frame` loops over `bands`. If a band is highlighted it fills the band's rows (`row - pad` … `row + pad`) once, then writes each of its texts at its `col` with the band's fg/bg.

### Tests
- `tests/test_input.c`: `o` → `EVENT_ADD_TEXT` in move mode; `o` → `EVENT_CHAR 'o'` in type mode.
- `tests/test_editor.c`: update existing `l.labels[i]` assertions to `l.bands[i].texts[0]`. Add tests for:
  - `o` gives two texts with the old one at col 3, enters type mode, and puts the caret on the right text
  - typed text is right-aligned at `cols - len - 1`
  - `o` on a band with two texts is a no-op (mode stays move)
  - `o` with no bands is a no-op
  - `o` on an empty text
  - `o` then `Esc` keeps an empty right text
  - `i` types into the right text
  - a single text stays centered

### Out of scope
- Long texts overlapping: both buffers keep `cols - 1` capacity, and the left and right texts may overlap visually. A later story can add a limit.
