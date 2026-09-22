# TODO-PF-2: Modern update checker

Develop a modern update checker to replace the disabled legacy SourceForge checker in `src/atanks.cpp` (masked with
`#if 0`, with its farewell URL). The replacement needs a version endpoint on project infrastructure the fork controls
(e.g. GitHub) and should reuse the `env.check_for_updates` option and the `update_data`/`update_string` plumbing where
practical.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-2.1: Analysis

Inventory the disabled checker (`src/atanks.cpp`, `src/update.h/.cpp`): what the legacy flow did, which
`env.check_for_updates` option and `update_data`/`update_string` plumbing survives, and what a GitHub-hosted version
endpoint (releases API versus static file) would have to provide.

## [ ] PF-2.2: Discussion

Agree the endpoint, the check cadence and failure behavior (offline, no network), and how much legacy plumbing to reuse,
with the user.

## [ ] PF-2.3: Implementation, part A (endpoint and check plumbing)

Build the endpoint lookup and the check flow behind the existing option. Further parts (UI surfacing, farewell-URL
replacement) become additional implementation Work Packages inserted before Tests once the analysis fixes the scope.

## [ ] PF-2.4: Tests

Unit-test the version comparison; smoke-test enabled/disabled and offline paths in-game.

## [ ] PF-2.5: Documentation

Document the endpoint and the option in `README.md`; note the feature in `CHANGELOG.md` on release.

## [ ] PF-2.6: Final testing and finalization

End-to-end validation against the real endpoint, then close the item per `docs/todo_planning.md`.
