# TODO-GI-1: tank.cpp cur_x/cur_y cleanup

`CTank::how_buried` (`src/tank.cpp:1072-1073`) initializes `cur_x`/`cur_y` to zero, but the initializers are never read
(cppcheck `unreadVariable`; the loop overwrites both before any read) and their scope can be reduced (cppcheck
`variableScope`). Decide with gameplay context whether the variables (and their computations) can go or something was
meant to consume them.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [x] GI-1.1: Analysis

Re-run cppcheck on `src/tank.cpp` and read `how_buried` end to end: confirm the initializers are dead, check whether the
per-angle `cur_x`/`cur_y` values feed anything besides the buried-pixel test, and decide the minimal cleanup.

Findings (2026-09-22 session): cppcheck 2.20 still reports exactly `variableScope` plus `unreadVariable` on the `= 0`
initializers (`src/tank.cpp:1072-1073`). End-to-end read of `CTank::how_buried`: both variables are reassigned every
loop iteration (lines 1078-1079) before any read (1081-1082, 1085-1086), so only the initializers are dead — the
per-angle computations genuinely feed the buried-pixel test, the `old_x`/`old_y` dedup, and the `left`/`right` outputs.
`old_x`/`old_y` initializers are live (first-iteration comparison) and stay. Minimal cleanup: move both declarations
into the loop body, which clears both findings at once with identical behavior. Choice goes to WP GI-1.2.

## [x] GI-1.2: Discussion

Confirm with the user whether to drop the dead initializers (and narrow the scope) or whether the computations were
meant to feed something else.

Decision (2026-09-22 session): **move into loop** — declare `cur_x`/`cur_y` in the loop body at first assignment,
clearing both findings with identical behavior.

## [x] GI-1.3: Implementation, clean up the variables

Apply the agreed cleanup. No further parts expected; extra findings become additional implementation Work Packages
inserted before Tests.

Implemented 2026-09-22: declarations moved into the loop body. Verified: `make user` warning-free, `make test` green,
both cppcheck findings gone (v23 formatter diff on the hunk is tree-wide drift only — rejected per `AGENTS.md`).

## [x] GI-1.4: Tests

`make test` stays green; buried-level behavior validated in-game (dug-in tanks).

Validated 2026-09-22: user tested in-game, all scenarios passed. `make test` green (session-verified).

## [x] GI-1.5: Documentation

No user-doc changes expected; note the cleanup in `CHANGELOG.md` only if it triggers a release.

No release triggered (comment-free two-line cleanup, version untouched); recorded at the next release instead.

## [x] GI-1.6: Final testing and finalization

Re-run cppcheck on the touched file, smoke-test in-game, then close the item per `docs/todo_planning.md`.

cppcheck clean and user in-game validation done (see GI-1.4). Item finished; removal from `TODO.md` plus plan-file
deletion waits for the next release per the Removal Rule.
