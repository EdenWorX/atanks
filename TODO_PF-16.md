# TODO-PF-16: Tornadoes

Add tornadoes which pick up tanks and objects. Objects could be the new rocks (see TODO-PF-9). Idea by Bharat
Dhareshwar.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-16.1: Analysis

Survey the object and physics systems (`CPhysicalObject`, wind, `CTank` throwing in `src/explosion.cpp`) for a
pick-up-and-carry mechanic: movement, lift, drop damage, duration, and interaction with rocks from TODO-PF-9.

## [ ] PF-16.2: Discussion

Agree spawn rules, strength, duration, and damage with the user, plus the dependency on TODO-PF-9.

## [ ] PF-16.3: Implementation, part A (rules fixed by the discussion)

Build the tornado object and the pick-up behavior. Further parts become additional implementation Work Packages
inserted before Tests.

## [ ] PF-16.4: Tests

Smoke-test tornadoes against tanks and objects in-game.

## [ ] PF-16.5: Documentation

Document the mechanic in `README.md` user docs; `CHANGELOG.md` on release.

## [ ] PF-16.6: Final testing and finalization

Full validation (weather, battle, balance), then close the item per `docs/todo_planning.md`.
