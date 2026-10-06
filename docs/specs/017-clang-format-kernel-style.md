# clang-format-kernel-style

## Refactoring Goal

The codebase has no machine-enforced style. `main.c`, `input.[ch]`,
`paint.[ch]`, `text_buffer.[ch]` and `tests/` were each hand-indented with two
spaces, and nothing stops the next contributor from indenting differently.
Adopt a single `.clang-format` that encodes the kernel style the project
wants (hard tabs, tab width 8, 80 columns, case labels flush with `switch`)
and wire it into `make` so the style is checkable in one command.

This is a formatting/tooling refactor only. No behavior changes, no new
modules, no change to `struct band`, the event loop, the buffer or the
renderer. An earlier idea of a `BasedOnStyle: Google`, `IndentWidth: 4` config
is rejected: the project standardises on kernel style.

## Technical Design

### Components

- `.clang-format` (new, repo root) — the style definition. `clang-format`
  discovers it by walking up from each source file, so every file in the
  repo uses it with no per-file config.
- `Makefile` — gains `FORMAT_FILES`, a `format` target and a `lint` target.
- `flake.nix` — `devShells.default` gains `pkgs.clang-tools`, which provides
  the `clang-format` binary the new targets call. Without this the targets
  only work on a machine that already has clang-format on `PATH`.
- Re-formatted sources (no content change): `main.c`, `input.c`, `input.h`,
  `paint.c`, `paint.h`, `text_buffer.c`, `text_buffer.h`,
  `tests/test_input.c`, `tests/test_text_buffer.c`.

### `.clang-format`

`BasedOnStyle: Linux` is the closest built-in preset to the kernel style, but
its defaults drift between clang-format versions. Every option that affects
the output is therefore pinned explicitly so the same input produces the same
bytes regardless of which clang-format runs it:

```yaml
BasedOnStyle: Linux
UseTab: Always
IndentWidth: 8
TabWidth: 8
ContinuationIndentWidth: 8
ColumnLimit: 80
IndentCaseLabels: false
BreakBeforeBraces: Linux
AllowShortIfStatementsOnASingleLine: Never
AllowShortFunctionsOnASingleLine: None
SortIncludes: Never
PointerAlignment: Right
SpaceBeforeParens: ControlStatements
AlignAfterOpenBracket: Align
```

The decisions behind the non-obvious pins:

- `UseTab: Always` + `IndentWidth: 8` + `TabWidth: 8` — hard tabs, one level
  is one tab byte, a tab renders 8 columns wide. This is the "8 spaces tabs"
  requirement: tab characters, not 8 space bytes.
- `IndentCaseLabels: false` — `case`/`default` sit flush with their `switch`.
  This is the kernel convention and is the one visible change beyond
  whitespace: today the cases in `main.c` and `input.c` are indented one
  level.
- `SortIncludes: Never` — include order is left exactly as authored. The
  reformat commit is then whitespace-only, so a reviewer can trust that no
  include moved. This matches kernel practice.
- `PointerAlignment: Right` (`struct band *b`, not `struct band * b` or
  `struct band* b`) — matches every existing declaration.
- `ColumnLimit: 80` — kernel width; lines over 80 get wrapped, which is the
  only place the reformat may insert line breaks rather than just reindent.

### `Makefile`

Discover files by glob so new sources are formatted without editing the
Makefile, and never touch `build/`:

```make
FORMAT_FILES = $(wildcard *.c *.h tests/*.c tests/*.h)

format:
	clang-format -i $(FORMAT_FILES)

lint:
	clang-format --dry-run --Werror $(FORMAT_FILES)
```

`format` rewrites in place; `lint` is the check — `--dry-run` makes no
changes and `--Werror` turns any would-be edit into a non-zero exit, so
`make lint` is the gate. Both are added to `.PHONY`.

### Rollout

Two commits, so the config decisions stay readable:

1. `.clang-format`, the `Makefile` targets and the `flake.nix` devShell entry.
   No source touched.
2. The mechanical reformat of all nine source files. Whitespace-only except
   for `ColumnLimit` wraps and the `IndentCaseLabels` shift.

### Testing

There is no unit test for formatting, so the checks are mechanical:

- `make lint` exits 0 after the reformat — no file still wants an edit.
- `make` builds clean with the existing `-Wall -Wextra`.
- `make test` passes `test_text_buffer` and `test_input` unchanged.
- `git diff --ignore-all-space` on the reformat commit is empty, proving the
  only changes are indentation/whitespace (this is the check that catches an
  accidental `SortIncludes` or line-join changing code).
- Behavior spot-check: build and run `dre`, confirm bands still draw, select
  with `j`/`k`, type with `i`, delete with `d`, and Ctrl-C restores the
  terminal.
