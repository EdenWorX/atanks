# TODO-GI-6: optiontypes.h license typo

`src/optiontypes.h:10` says "or (at your menu) any later version" — "menu" is a typo for "option" in the license header.
Fix with any future edit of that header (header normalization itself is complete, so this is a one-word correction).

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [x] GI-6.1: Analysis

Confirm the typo and check whether the same header text was copied into any other file.

Findings (2026-09-22 session): typo confirmed at `src/optiontypes.h:10`, unique in the tree (`git grep "at your menu"`
hits only that line); the correct "at your option" form is used across the other headers. Landing choice goes to
WP GI-6.2.

## [x] GI-6.2: Discussion

Confirm with the user whether the fix lands on its own or rides along with the next edit of that header.

Decision (2026-09-22 session): **fix alone now** — one word, no functional edit pending.

## [x] GI-6.3: Implementation, fix the typo

Correct "menu" to "option". No further parts expected.

Implemented 2026-09-22; typo gone tree-wide (`git grep`). Verified: `make user` warning-free, `make test` green.
Single comment word — no formatting impact, so no clang-format run needed.

## [x] GI-6.4: Tests

No behavior change; full build stays warning-free.

Build warning-free, `make test` green (session-verified).

## [x] GI-6.5: Documentation

No doc changes.

## [x] GI-6.6: Final testing and finalization

Confirm the correction, then close the item per `docs/todo_planning.md`.

Correction confirmed tree-wide. Item finished; removal from `TODO.md` plus plan-file deletion waits for the next
release per the Removal Rule.
