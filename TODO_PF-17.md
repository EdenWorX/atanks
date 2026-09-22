# TODO-PF-17: Another armor level

Add another level of armour improvement. What that actually means and what its effect on the game will be still needs
discussion.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-17.1: Analysis

Read the existing armor levels (shop items, damage reduction, costs) and the `CTank` damage path to frame the
discussion with concrete options for a further level.

## [ ] PF-17.2: Discussion

Decide with the user what the new armor level means: protection values, cost, availability, and AI purchase behavior.

### [ ] PF-17.2.1: Clarify protection, cost, and availability of the new armor level with the user

## [ ] PF-17.3: Implementation, part A (values fixed by the discussion)

Build the armor level (item record, damage path, shop/AI wiring). Further parts become additional implementation Work
Packages inserted before Tests.

## [ ] PF-17.4: Tests

Smoke-test purchase, damage reduction, and AI behavior in-game.

## [ ] PF-17.5: Documentation

Document the armor level in `README.md` user docs; `CHANGELOG.md` on release.

## [ ] PF-17.6: Final testing and finalization

Full validation (shop, battle, balance), then close the item per `docs/todo_planning.md`.
