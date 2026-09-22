# TODO-II-2: Missile null-pointer exposure

`MISSILE::applyPhysicsFunky` (`src/missile.cpp:413-423`) dereferences `launchWeap` unconditionally while it is null for
any weapon type other than `FUNKY_BOMBLET`/`FUNKY_DEATHLET`. The shipped data is safe (parents map to bomblet
submunitions and AI mind-shots reuse those types), but any new weapon data or physics assignment putting `PT_FUNKY_FLOAT`
on another type segfaults. Fix with a defensive guard when that function is next touched (gameplay physics needs
in-game validation).

Source: `TODO_Xtra.md` (`Important Issues`). Status: see the overview table in `TODO.md`.

## [ ] II-2.1: Analysis

Read `applyPhysicsFunky`, map every `weap_type` that can reach the homing branch, and confirm which shipped
weapons/naturals keep `launchWeap` non-null. Define the safe fallback (skip homing versus default speed) for the null
case.

### [ ] II-2.1.1: Map the weapon types reaching the homing branch and their `launchWeap` values
### [ ] II-2.1.2: Define the fallback behavior for the null case

## [ ] II-2.2: Discussion

Agree the fallback with the user (skip homing, default speed, or assert in debug builds) and whether the fix rides along
with the next physics touch or lands on its own.

## [ ] II-2.3: Implementation, defensive guard

Add the agreed guard. Further parts (if the analysis finds sibling exposures) become additional implementation Work
Packages inserted before Tests.

## [ ] II-2.4: Tests

`make test` stays green; validate funky-float homing in-game (human and AI shots).

## [ ] II-2.5: Documentation

Record the fix in `CHANGELOG.md` if it triggers a release; no user-doc changes expected.

## [ ] II-2.6: Final testing and finalization

In-game validation plus `./do_memcheck.sh`, then close the item per `docs/todo_planning.md`.
