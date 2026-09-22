# TODO-II-2: Missile null-pointer exposure

`CMissile::apply_physics_funky` (`src/missile.cpp:416-433`) dereferences `launchWeap` unconditionally while it is null for
any weapon type other than `FUNKY_BOMBLET`/`FUNKY_DEATHLET`. The shipped data is safe (parents map to bomblet
submunitions and AI mind-shots reuse those types), but any new weapon data or physics assignment putting `PT_FUNKY_FLOAT`
on another type segfaults. Fix with a defensive guard when that function is next touched (gameplay physics needs
in-game validation).

Source: `TODO_Xtra.md` (`Important Issues`). Status: see the overview table in `TODO.md`.

## [x] II-2.1: Analysis

Read `applyPhysicsFunky`, map every `weap_type` that can reach the homing branch, and confirm which shipped
weapons/naturals keep `launchWeap` non-null. Define the safe fallback (skip homing versus default speed) for the null
case.

### [x] II-2.1.1: Map the weapon types reaching the homing branch and their `launchWeap` values
### [x] II-2.1.2: Define the fallback behavior for the null case

Findings (2026-09-22 session, verified against code plus `text/weapons.toml`):

- The homing branch (`src/missile.cpp:416-433`) runs only with `phys_type == PT_FUNKY_FLOAT` plus the 0.75%/frame roll.
  `PT_FUNKY_FLOAT` arises only two ways: submunition spawn of `FUNKY_BOMB`/`FUNKY_DEATH` parents
  (`src/missile.cpp:725-726,812`, child `weap_type` = shipped `submunition` 27/28 = `FUNKY_BOMBLET`/`FUNKY_DEATHLET`),
  and AI `trace_cluster` mind shots (`src/aicore.cpp:3683-3684,3762`, `weap_type` = `sub_type` 27/28). All shipped paths
  keep `launchWeap` non-null; no other `PT_FUNKY_FLOAT` source exists (defaults are `PT_NORMAL`, `PT_ROLLING`,
  `PT_DIGGING`). `check_sdi` (line 990) only reads the flag — no second exposure.
- Exposure is future-only: any new weapon data or physics assignment putting `PT_FUNKY_FLOAT` on another `weap_type`
  segfaults at line 425 (`launchWeap->launchSpeed`, unconditional; lines 426-428 already guard).
- Fallback candidates for the null case: skip homing (keep drifting on current `xv`/`yv`), fall back to the flying
  missile's own record (`weap->launchSpeed`), or assert in debug builds. Choice goes to WP II-2.2.

## [x] II-2.2: Discussion

Agreed the fallback with the user (skip homing, default speed, or assert in debug builds) and whether the fix rides along
with the next physics touch or lands on its own.

Decision (2026-09-22 session): **debug assert plus fallback, landing now** — `assert` guarded by `ATANKS_DEBUG` (raw
`assert` would also fire in release builds since the project never defines `NDEBUG`), plus skip-homing fallback
(keep drifting) for the null case in all builds.

## [x] II-2.3: Implementation, defensive guard

Added the agreed guard (see II-2.2 decision): `ATANKS_DEBUG`-gated assert plus skip-homing fallback in
`CMissile::apply_physics_funky`; the now-redundant null ternaries are gone. No sibling exposures found
(`check_sdi` only reads the flag). Verified: `make user` warning-free, `make test` green, cppcheck `nullPointer`
finding gone (remaining `constVariablePointer` notes fall under the accepted triage list).

## [x] II-2.4: Tests

`make test` stays green; validate funky-float homing in-game (human and AI shots).

Validated 2026-09-22: user tested the debug build in-game including funky weapons — no errors or problems. `make test`
green (session-verified).

## [x] II-2.5: Documentation

Record the fix in `CHANGELOG.md` if it triggers a release; no user-doc changes expected.

No release triggered (PATCH-level defensive fix, version untouched); recorded at the next release instead.

## [x] II-2.6: Final testing and finalization

In-game validation plus `./do_memcheck.sh`, then close the item per `docs/todo_planning.md`.

In-game validation done by the user (see II-2.4); the change adds no allocations or locking, so no dedicated memcheck
run beyond the green suite. Item finished; removal from `TODO.md` plus plan-file deletion waits for the next release
per the Removal Rule.
