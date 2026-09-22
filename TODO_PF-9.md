# TODO-PF-9: Semi-destructible rocks

Add rocks as semi-destructable items. Explosions look normal, but the carving radius is halved, so that the amount of
debris and the size of the chunk taken out are quartered.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-9.1: Analysis

Read terrain generation and explosion carving (`src/land.h/.cpp`, `src/levelcreator.h/.cpp`, `src/explosion.h/.cpp`):
where rock objects would spawn, how the carving radius and debris amounts derive from it, and how halving plays with
existing weapons (including riot and dirt weapons).

### [ ] PF-9.1.1: Read terrain generation and explosion carving plus debris scaling
### [ ] PF-9.1.2: Map riot- and dirt-weapon interactions with halved carving

## [ ] PF-9.2: Discussion

Agree rock placement, visuals, and the exact carving/debris rules with the user.

## [ ] PF-9.3: Implementation, rocks and halved carving

Add rock objects and the halved-radius carving path. No further parts expected; follow-ups (e.g. tornado interaction
with TODO-PF-16) become additional implementation Work Packages inserted before Tests.

## [ ] PF-9.4: Tests

Smoke-test rock generation, explosions on rock versus dirt, and debris amounts in-game.

## [ ] PF-9.5: Documentation

Document rocks in `README.md` user docs; `CHANGELOG.md` on release.

## [ ] PF-9.6: Final testing and finalization

Full validation (terrain gen, battle, balance), then close the item per `docs/todo_planning.md`.
