# TODO-PF-14: Harder ground option

Add an option to make the ground harder. Possible options like "hard", "rock" and "steel" could multiply the carving
radius by 0.75, 0.5 and 0.1. The carving radius is the radius of ground removed by an explosion and the base for debris
amounts. Open questions: how this affects riot weapons, and whether dirt weapons generate a small amount of damage.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-14.1: Analysis

Read explosion carving and debris scaling (`src/explosion.h/.cpp`), the option system for a new ground-hardness setting,
and how riot versus dirt weapons use the carving radius today.

### [ ] PF-14.1.1: Read explosion carving, debris scaling, and the option system
### [ ] PF-14.1.2: Map riot- versus dirt-weapon use of the carving radius

## [ ] PF-14.2: Discussion

Decide with the user the hardness levels and multipliers, the riot-weapon interaction, and the dirt-weapon damage
question.

### [ ] PF-14.2.1: Clarify hardness levels, riot-weapon effects, and dirt-weapon damage with the user

## [ ] PF-14.3: Implementation, part A (rules fixed by the discussion)

Build the option plus the carving multiplier. Further parts (weapon-specific rules) become additional implementation
Work Packages inserted before Tests.

## [ ] PF-14.4: Tests

Smoke-test each hardness level with several weapon types in-game.

## [ ] PF-14.5: Documentation

Document the option in `README.md`; `CHANGELOG.md` on release.

## [ ] PF-14.6: Final testing and finalization

Full validation (terrain, battle, balance), then close the item per `docs/todo_planning.md`.
