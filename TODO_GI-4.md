# TODO-GI-4: shop.cpp finding review

cppcheck flags three sites in `src/shop.cpp`: the trolley loop versus `std::fill` (line 830), `weap`
const-correctness (line 912), and `teamFee` handling (line 931). All need gameplay-context review before any change.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [x] GI-4.1: Analysis

Read each flagged site with its surrounding shop logic: confirm the trolley loop is a fill candidate, check whether
`weap` is ever reseated, and trace `teamFee` through the team-money division to judge the finding.

Findings (2026-09-22 session, cppcheck 2.20 re-run):

- Trolley loop (`Shop::reset`, line 830): range-for zeroing the `int32_t trolley[THINGS]` member — a direct
  `std::fill` candidate (needs `<algorithm>`; pointer form avoids an `<iterator>` dependency).
- `weap` (line 912, `calc_potential_dmg`): never reseated, reads only (`submunition`, `numSubmunitions`, `damage`) —
  `CWeapon const*` is safe. Same finding exists at line 182 (hover description, reads `radius`, `spread`, const
  `get_desc()`); also safe, proposed as a fourth site.
- `teamFee` (line 931): only the `= 0` initializer is dead — every path assigns before reading (lines 936/943/963/979
  versus reads right below each). Dropping the initializer suffices; the variable stays function-scoped.
- Per-site apply/keep decisions go to WP GI-4.2.

## [x] GI-4.2: Discussion

Confirm with the user for each site whether to apply the suggestion or keep the current form (with reasoning recorded).

Decision (2026-09-22 session): **apply all four** — the listed three sites plus the identical `weap` const finding at
line 182.

## [x] GI-4.3: Implementation, apply the agreed changes

Apply the agreed changes. Further parts (if the review splits per site) become additional implementation Work Packages
inserted before Tests.

Implemented 2026-09-22, all four sites: `std::fill` for the trolley loop (plus `<algorithm>`), `CWeapon const*` at both
`weap` sites, bare `teamFee` declaration. Verified: `make user` warning-free, `make test` green, all four cppcheck
findings gone; hunks match the surrounding v19 style (v23-only `x[ y ]` drift rejected).

## [x] GI-4.4: Tests

`make test` stays green; buy-screen behavior validated in-game (buy, sell, team-money division).

Validated 2026-09-22: user tested the shop after loading a game and after playing a round — everything works, no
errors. `make test` green (session-verified).

## [x] GI-4.5: Documentation

No user-doc changes expected.

## [x] GI-4.6: Final testing and finalization

Re-run cppcheck on the touched file, smoke-test the shop in-game, then close the item per `docs/todo_planning.md`.

cppcheck clean and user shop validation done (see GI-4.4). Item finished; removal from `TODO.md` plus plan-file
deletion waits for the next release per the Removal Rule.
