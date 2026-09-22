# TODO-GI-4: shop.cpp finding review

cppcheck flags three sites in `src/shop.cpp`: the trolley loop versus `std::fill` (line 830), `weap`
const-correctness (line 912), and `teamFee` handling (line 931). All need gameplay-context review before any change.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [ ] GI-4.1: Analysis

Read each flagged site with its surrounding shop logic: confirm the trolley loop is a fill candidate, check whether
`weap` is ever reseated, and trace `teamFee` through the team-money division to judge the finding.

## [ ] GI-4.2: Discussion

Confirm with the user for each site whether to apply the suggestion or keep the current form (with reasoning recorded).

## [ ] GI-4.3: Implementation, apply the agreed changes

Apply the agreed changes. Further parts (if the review splits per site) become additional implementation Work Packages
inserted before Tests.

## [ ] GI-4.4: Tests

`make test` stays green; buy-screen behavior validated in-game (buy, sell, team-money division).

## [ ] GI-4.5: Documentation

No user-doc changes expected.

## [ ] GI-4.6: Final testing and finalization

Re-run cppcheck on the touched file, smoke-test the shop in-game, then close the item per `docs/todo_planning.md`.
