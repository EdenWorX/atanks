# TODO-PF-8: Radar-resistant missile

Add a radar resistant missile: a missile designed to avoid the missile defence system by masking its heat signature.
Yield: Large missile.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-8.1: Analysis

Read the missile defence system, the SDI/missile-interception logic, and the weapon record layout
(`text/weapons.toml`, `src/weapon.h/.cpp`, `CMissile`): how heat-signature masking would plug into interception checks,
and what a "Large missile" yield implies for balance.

## [ ] PF-8.2: Discussion

Agree masking mechanics, cost, damage, and AI selection weight with the user.

## [ ] PF-8.3: Implementation, weapon record and masking

Add the TOML record (plus translations), the masking behavior, and AI/shop wiring. No further parts expected;
follow-ups become additional implementation Work Packages inserted before Tests.

### [ ] PF-8.3.1: Add the weapon record plus translations to the arsenal data
### [ ] PF-8.3.2: Implement heat-signature masking in the interception checks
### [ ] PF-8.3.3: Wire shop availability and AI selection

## [ ] PF-8.4: Tests

Smoke-test firing against defences, interception rates, and AI use in-game.

## [ ] PF-8.5: Documentation

Document the weapon in `README.md` user docs; `CHANGELOG.md` on release.

## [ ] PF-8.6: Final testing and finalization

Full validation (shop, battle, AI, balance), then close the item per `docs/todo_planning.md`.
