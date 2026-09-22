# TODO-GI-1: tank.cpp cur_x/cur_y cleanup

`CTank::how_buried` (`src/tank.cpp:1072-1073`) initializes `cur_x`/`cur_y` to zero, but the initializers are never read
(cppcheck `unreadVariable`; the loop overwrites both before any read) and their scope can be reduced (cppcheck
`variableScope`). Decide with gameplay context whether the variables (and their computations) can go or something was
meant to consume them.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [ ] GI-1.1: Analysis

Re-run cppcheck on `src/tank.cpp` and read `how_buried` end to end: confirm the initializers are dead, check whether the
per-angle `cur_x`/`cur_y` values feed anything besides the buried-pixel test, and decide the minimal cleanup.

## [ ] GI-1.2: Discussion

Confirm with the user whether to drop the dead initializers (and narrow the scope) or whether the computations were
meant to feed something else.

## [ ] GI-1.3: Implementation, clean up the variables

Apply the agreed cleanup. No further parts expected; extra findings become additional implementation Work Packages
inserted before Tests.

## [ ] GI-1.4: Tests

`make test` stays green; buried-level behavior validated in-game (dug-in tanks).

## [ ] GI-1.5: Documentation

No user-doc changes expected; note the cleanup in `CHANGELOG.md` only if it triggers a release.

## [ ] GI-1.6: Final testing and finalization

Re-run cppcheck on the touched file, smoke-test in-game, then close the item per `docs/todo_planning.md`.
