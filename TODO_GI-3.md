# TODO-GI-3: Redundant conditional assignments

Three `if (x) x = value` patterns are logically equivalent to plain assignment (cppcheck `duplicateConditionalAssign`).
Simplify each site when its function is next touched.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [x] GI-3.1: Analysis

Confirm all three sites still carry the redundant pattern and that plain assignment preserves behavior (no concurrent
modification between condition and assignment at any site).

Findings (2026-09-22 session, all three sites confirmed live):

- `src/tank.cpp:634-636`: `if (is_teleported) is_teleported = false;` in fall handling. Writers set it `true` from
  teleport paths; a cross-thread race window exists either way and plain assignment introduces no new hazard class.
- `src/explosion.cpp:744-746`: `if (!has_cleared) has_cleared = true;` inside `do_clear`, itself nested in the outer
  `if (!has_cleared)` (line 713); nothing between the two tests writes the member, so the inner check is dead.
- `src/floattext.cpp:341`: `pos_y = y;` duplicates line 336 (`y` untouched between — only `pos_x`/`xv` change), so the
  line deletes cleanly. Choice of landing goes to WP GI-3.2.

## [x] GI-3.2: Discussion

Confirm with the user whether the three one-line simplifications land together or ride along with the next touch of each
function.

Decision (2026-09-22 session): **together now** — all three land in one pass.

## [x] GI-3.3: Implementation, simplify the sites

### [x] GI-3.3.1: Simplify `is_teleported` in `src/tank.cpp`
### [x] GI-3.3.2: Simplify `has_cleared` in `src/explosion.cpp`
### [x] GI-3.3.3: Simplify the redundant assignment in `src/floattext.cpp`
### [x] GI-3.3.4: Simplify `color` in `CFloatText::set_color` (same pattern found adjacent during implementation)

Implemented 2026-09-22, all four sites (the fourth found by re-running cppcheck after the fix). Verified: `make user`
warning-free, `make test` green, no `duplicateConditionalAssign` remains in the three files; touched hunks match the
surrounding v19 style (v23-only drift rejected).

## [x] GI-3.4: Tests

`make test` stays green; no behavior change expected, verified by inspection plus a smoke run.

Validated 2026-09-22: user smoke-tested in-game, program works as expected. `make test` green (session-verified).

## [x] GI-3.5: Documentation

No doc changes.

## [x] GI-3.6: Final testing and finalization

Re-run cppcheck on the touched files, then close the item per `docs/todo_planning.md`.

cppcheck clean and user smoke run done (see GI-3.4). Item finished; removal from `TODO.md` plus plan-file deletion
waits for the next release per the Removal Rule.
