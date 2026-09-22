# TODO-PF-12: Shootable UFO

Make it possible to shoot down the UFO (`src/satellite.h/.cpp`).

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-12.1: Analysis

Read the satellite/UFO logic (spawn chance, movement, shooting): hit detection options against missiles and lasers,
health model, crash effect (explosion, debris, score), and respawn rules.

### [ ] PF-12.1.1: Read the satellite logic (spawn, movement, shooting)
### [ ] PF-12.1.2: Enumerate hit-detection options against missiles and lasers

## [ ] PF-12.2: Discussion

Agree hit rules, crash effects, and scoring with the user.

## [ ] PF-12.3: Implementation, part A (rules fixed by the discussion)

Build hit detection plus the crash sequence. Further parts (respawn tuning, scoring) become additional implementation
Work Packages inserted before Tests.

## [ ] PF-12.4: Tests

Smoke-test shooting the UFO down with several weapon types in-game.

## [ ] PF-12.5: Documentation

Document the mechanic in `README.md` user docs; `CHANGELOG.md` on release.

## [ ] PF-12.6: Final testing and finalization

Full validation (spawn, combat, crash, scoring), then close the item per `docs/todo_planning.md`.
