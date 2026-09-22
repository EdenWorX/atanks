# TODO-GI-2: teleport.cpp condition review

cppcheck reports always-true conditions in `src/teleport.cpp` (`knownConditionTrueFalse`, e.g. line 207) and missing
copy semantics for `CTeleport` (`noCopyConstructor`, `noOperatorEq`). Both need gameplay-context review: the conditions
may encode cross-frame invariants, and the class owns a remote teleport instance.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [ ] GI-2.1: Analysis

Re-run cppcheck on `src/teleport.cpp`, read each flagged condition with its surrounding logic, and check `CTeleport`
ownership (who creates, draws, and deletes `remote`) to judge whether copy semantics are actually needed.

## [ ] GI-2.2: Discussion

Confirm with the user for each finding whether it is an intentional invariant (keep, possibly with a clarifying comment)
or a real defect (fix), and whether `CTeleport` needs copy semantics or non-copyable enforcement instead.

## [ ] GI-2.3: Implementation, address the findings

Apply the agreed fixes. Further parts (if the analysis splits conditions from ownership) become additional
implementation Work Packages inserted before Tests.

## [ ] GI-2.4: Tests

`make test` stays green; teleport behavior validated in-game.

## [ ] GI-2.5: Documentation

No user-doc changes expected; note the outcome in `CHANGELOG.md` only if it triggers a release.

## [ ] GI-2.6: Final testing and finalization

Re-run cppcheck on the touched file, smoke-test teleports in-game, then close the item per `docs/todo_planning.md`.
