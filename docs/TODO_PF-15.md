# TODO-PF-15: High-voltage missiles

Add high voltage missiles (discharge on impact or when within range). Idea by Bharat Dhareshwar.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-15.1: Analysis

Read the missile and explosion systems for discharge mechanics (area effect on impact or proximity trigger), the weapon
record layout for a new entry, and balance against comparable missiles.

## [ ] PF-15.2: Discussion

Agree trigger rules (impact versus proximity range), damage model, cost, and AI handling with the user.

## [ ] PF-15.3: Implementation, weapon record and discharge

Add the TOML record (plus translations), the discharge behavior, and AI/shop wiring. No further parts expected;
follow-ups become additional implementation Work Packages inserted before Tests.

### [ ] PF-15.3.1: Add the weapon record plus translations to the arsenal data
### [ ] PF-15.3.2: Implement discharge on impact and within range
### [ ] PF-15.3.3: Wire shop availability and AI use

## [ ] PF-15.4: Tests

Smoke-test discharge on impact and in range, plus AI use, in-game.

## [ ] PF-15.5: Documentation

Document the weapon in `README.md` user docs; `CHANGELOG.md` on release.

## [ ] PF-15.6: Final testing and finalization

Full validation (shop, battle, AI, balance), then close the item per `docs/todo_planning.md`.
