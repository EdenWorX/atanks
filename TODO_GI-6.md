# TODO-GI-6: optiontypes.h license typo

`src/optiontypes.h:10` says "or (at your menu) any later version" — "menu" is a typo for "option" in the license header.
Fix with any future edit of that header (header normalization itself is complete, so this is a one-word correction).

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [ ] GI-6.1: Analysis

Confirm the typo and check whether the same header text was copied into any other file.

## [ ] GI-6.2: Discussion

Confirm with the user whether the fix lands on its own or rides along with the next edit of that header.

## [ ] GI-6.3: Implementation, fix the typo

Correct "menu" to "option". No further parts expected.

## [ ] GI-6.4: Tests

No behavior change; full build stays warning-free.

## [ ] GI-6.5: Documentation

No doc changes.

## [ ] GI-6.6: Final testing and finalization

Confirm the correction, then close the item per `docs/todo_planning.md`.
