# TODO-GI-3: Redundant conditional assignments

Three `if (x) x = value` patterns are logically equivalent to plain assignment (cppcheck `duplicateConditionalAssign`).
Simplify each site when its function is next touched.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [ ] GI-3.1: Analysis

Confirm all three sites still carry the redundant pattern and that plain assignment preserves behavior (no concurrent
modification between condition and assignment at any site).

## [ ] GI-3.2: Discussion

Confirm with the user whether the three one-line simplifications land together or ride along with the next touch of each
function.

## [ ] GI-3.3: Implementation, simplify the sites

### [ ] GI-3.3.1: Simplify `is_teleported` in `src/tank.cpp`
### [ ] GI-3.3.2: Simplify `has_cleared` in `src/explosion.cpp`
### [ ] GI-3.3.3: Simplify the redundant assignment in `src/floattext.cpp`

## [ ] GI-3.4: Tests

`make test` stays green; no behavior change expected, verified by inspection plus a smoke run.

## [ ] GI-3.5: Documentation

No doc changes.

## [ ] GI-3.6: Final testing and finalization

Re-run cppcheck on the touched files, then close the item per `docs/todo_planning.md`.
