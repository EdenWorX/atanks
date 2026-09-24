# TODO-PF-7: Field repair kit item

Add a "field repair kit" item: spend a turn to repair your tank rather than fire. Limited uses, heals more than the Auto
Repair Kit.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-7.1: Analysis

Read the item system (`src/item.h/.cpp`, `text/items.toml` plus translations, shop availability, `CAICore` item use):
item record layout, count limits (`ITEMS`), turn-flow integration for a spend-turn-to-repair action, and how the Auto
Repair Kit compares.

## [ ] PF-7.2: Discussion

Agree cost, heal amount, use limit, and AI handling with the user.

## [ ] PF-7.3: Implementation, item record and repair action

Add the TOML record (plus translations), the repair action, shop/AI wiring. No further parts expected; follow-ups
become additional implementation Work Packages inserted before Tests.

### [ ] PF-7.3.1: Add the item record plus translations to the arsenal data
### [ ] PF-7.3.2: Implement the spend-turn-to-repair action and turn flow
### [ ] PF-7.3.3: Wire shop availability and AI use

## [ ] PF-7.4: Tests

Smoke-test buying, using (turn spent, healing applied, uses decremented), and AI behavior in-game.

## [ ] PF-7.5: Documentation

Document the item in `README.md` user docs; `CHANGELOG.md` on release.

## [ ] PF-7.6: Final testing and finalization

Full validation (shop, battle, AI), then close the item per `docs/todo_planning.md`.
