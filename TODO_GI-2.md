# TODO-GI-2: teleport.cpp condition review

cppcheck reports always-true conditions in `src/teleport.cpp` (`knownConditionTrueFalse`, e.g. line 207) and missing
copy semantics for `CTeleport` (`noCopyConstructor`, `noOperatorEq`). Both need gameplay-context review: the conditions
may encode cross-frame invariants, and the class owns a remote teleport instance.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [x] GI-2.1: Analysis

Re-run cppcheck on `src/teleport.cpp`, read each flagged condition with its surrounding logic, and check `CTeleport`
ownership (who creates, draws, and deletes `remote`) to judge whether copy semantics are actually needed.

Findings (2026-09-22 session, cppcheck 2.20 re-run):

- Lines 91/101 (`knownConditionTrueFalse` in the old triage) are NO LONGER flagged — resolved.
- Line 207 `if (object && remote)`: NOT dead. `remote` stays null when the target-end `new` at line 112 throws
  (`catch (std::bad_alloc&)` prints and continues degraded), and `object` may be null per the ctor guard at line 68.
  The check is genuine allocation-failure defensiveness; keep, possibly with a clarifying comment.
- `noCopyConstructor`/`noOperatorEq` (line 112): `CTeleport` is `final`, privately constructs its remote end, and is
  only ever handled via `new` + raw pointers (`tank.cpp`, `client.cpp`) — never copied. A shallow copy would corrupt
  the destroy-flag protocol in `~CTeleport`. Correct outcome is non-copyable enforcement (`= delete`), not added copy
  semantics.
- Out of scope, no action: `the_tank` `constVariablePointer` (line 123) and `physobj.h` `uselessOverride` fall under
  the already-accepted triage categories in `tools/run-cppcheck.sh`; no new `TODO_Xtra.md` entry needed.
- Choices go to WP GI-2.2.

## [x] GI-2.2: Discussion

Confirm with the user for each finding whether it is an intentional invariant (keep, possibly with a clarifying comment)
or a real defect (fix), and whether `CTeleport` needs copy semantics or non-copyable enforcement instead.

Decision (2026-09-22 session): **comment plus non-copyable enforcement** — keep the null checks, document the
`bad_alloc` path, `= delete` copy ctor/assignment. Implementation additionally guards the `blit` at (old) line 204,
which dereferenced `remote` unconditionally two lines above the guarded check and would still have crashed the degraded
object in `draw()`.

## [x] GI-2.3: Implementation, address the findings

Apply the agreed fixes. Further parts (if the analysis splits conditions from ownership) become additional
implementation Work Packages inserted before Tests.

Implemented 2026-09-22: `= delete` copy ctor/assignment in `teleport.h`; `blit` guarded by `if (remote)` plus
`cppcheck-suppress knownConditionTrueFalse` with reason (project-sanctioned pattern); redundant `else if`s at
91/101 simplified to `else` (provably equivalent — found live in the LINUX config pass during verification).
Verified: `make user` warning-free, `make test` green, all original findings gone from cppcheck (remaining
`the_tank`/`uselessOverride` notes are accepted triage categories).

## [x] GI-2.4: Tests

`make test` stays green; teleport behavior validated in-game.

Validated 2026-09-22: user tested in-game, testing successful. `make test` green (session-verified).

## [x] GI-2.5: Documentation

No user-doc changes expected; note the outcome in `CHANGELOG.md` only if it triggers a release.

No release triggered (review cleanup with zero behavior change, version untouched); recorded at the next release
instead.

## [x] GI-2.6: Final testing and finalization

Re-run cppcheck on the touched file, smoke-test teleports in-game, then close the item per `docs/todo_planning.md`.

cppcheck clean and user in-game validation done (see GI-2.4). Item finished; removal from `TODO.md` plus plan-file
deletion waits for the next release per the Removal Rule.
