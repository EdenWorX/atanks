# TODO-PF-6: Buy-screen randomize button

Add a randomize button to the buying screen to have items automatically purchased (`src/shop.cpp`).

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-6.1: Analysis

Read the shop purchase flow (`src/shop.cpp`, `src/player.cpp` economy): budgets, item availability, and how an
automatic purchase pass would spend a player's money fairly across weapons and items.

## [ ] PF-6.2: Discussion

Agree the auto-purchase rules with the user (budget split, weapon versus item preference, re-rolls, AI parity).

## [ ] PF-6.3: Implementation, randomize button and auto-purchase

Build the button and the purchase pass. No further parts expected; follow-ups become additional implementation Work
Packages inserted before Tests.

## [ ] PF-6.4: Tests

Smoke-test the button across budgets and player types; `make test` stays green.

## [ ] PF-6.5: Documentation

Note the button in `README.md` buy-screen help; `CHANGELOG.md` on release.

## [ ] PF-6.6: Final testing and finalization

Full buy-screen validation, then close the item per `docs/todo_planning.md`.
