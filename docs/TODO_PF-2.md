# TODO-PF-2: Modern update checker

Develop a modern update checker to replace the disabled legacy SourceForge checker in `src/atanks.cpp` (masked with
`#if 0`, with its farewell URL). The replacement needs a version endpoint on project infrastructure the fork controls
(e.g. GitHub) and should reuse the `env.check_for_updates` option and the `update_data`/`update_string` plumbing where
practical.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [x] PF-2.1: Analysis

Inventory the disabled checker (`src/atanks.cpp`, `src/update.h/.cpp`): what the legacy flow did, which
`env.check_for_updates` option and `update_data`/`update_string` plumbing survives, and what a GitHub-hosted version
endpoint (releases API versus static file) would have to provide.

Findings (2026-09-22 session):

- Legacy flow: background `std::thread` runs `UpdateData::operator()` — raw POSIX-socket HTTP/1.1 GET of
  `projects.sourceforge.net/version.txt`, scans for `"Version: "`, compares `web_version * 10 > game_version`,
  formats `global.update_string`. Whole body is `#ifdef NETWORK` (Linux/BSD only); MSVC socket support was never
  written (`update.cpp:12-14` open question). Silent on failure; thread joined at exit.
- Surviving plumbing: `env.check_for_updates` (default true, persisted, options-menu toggle), `global.update_string`
  (shown in the main menu at `atanks.cpp:992` and at exit at `:1594`), `UpdateData` struct plus thread pattern,
  integer `game_version` (`VERSION * 10`).
- Endpoint: the GitHub Releases API is LIVE with `v6.7.1` already published (`tag_name` compare needs HTTPS plus
  minimal JSON). A static `version.txt` would need hosting the fork controls — none exists today.
- Transport: raw sockets cannot do HTTPS (GitHub is HTTPS-only); libcurl exists on this machine but would be a new
  dependency with Windows/macOS story to solve. Endpoint, transport, and platform scope go to WP PF-2.2.

## [x] PF-2.2: Discussion

Agree the endpoint, the check cadence and failure behavior (offline, no network), and how much legacy plumbing to reuse,
with the user.

Decisions (2026-09-22 session): **GitHub Releases API** (`tag_name` compare; verified live with `v6.7.1`) fetched via
**libcurl on all platforms** (raw sockets cannot do HTTPS; legacy `NETWORK`-only scope dropped). Keep legacy
behavior otherwise: background thread at startup, silent failure, `check_for_updates` gate, `update_string` display
paths. Pure version parse/compare goes into `update.h` as inline helpers so `tests/test_update.cpp` can unit-test
them; numeric tuple compare (legacy `double * 10` breaks for 6.10-style versions).

## [x] PF-2.3: Implementation, part A (endpoint and check plumbing)

Build the endpoint lookup and the check flow behind the existing option. Further parts (UI surfacing, farewell-URL
replacement) become additional implementation Work Packages inserted before Tests once the analysis fixes the scope.

Implemented 2026-09-22: `UpdateData` reworked to libcurl + Releases API (`tag_name` numeric tuple compare);
`ATANKS_HAVE_CURL` config wiring plus `CURL::libcurl` link; spawn/join blocks unmasked in
`main` (`curl_global_init` before spawn, join plus result publish at exit); `check_for_updates` gate and both
`update_string` display paths reused untouched; farewell-URL block left `#if 0` for a later part. Verified:
`make user` warning-free (two `network.cpp` strncpy notes pre-exist, file untouched), `make test-all` green
(40 tests incl. 8 new), cppcheck clean on `update.cpp`, hunks match v19 style (v23-only drift rejected).

## [x] PF-2.4: Tests

Unit-test the version comparison; smoke-test enabled/disabled and offline paths in-game.

Implemented 2026-09-22: `tests/test_update.cpp` (8 tests: parse full/partial/garbage/overflow/trailing, newer/equal/
older incl. 6.10-style, tag extraction happy/malformed) wired into `atanks_tests`; `make test-all` green (40 tests).
Headless runtime: menu loop reached with the checker thread live, no errors, no spurious message (live API reports
v6.7.1, equal to local). In-game menu/quit paths stay with user validation (see PF-2.6).

## [x] PF-2.5: Documentation

Document the endpoint and the option in `README.md`; note the feature in `CHANGELOG.md` on release.

Documented 2026-09-22: endpoint, libcurl build requirement, `CHECKUPDATES` setting, and silent-failure behavior added
to the Configuration chapter. `CHANGELOG.md` waits for a release (none triggered).

## [ ] PF-2.6: Final testing and finalization

End-to-end validation against the real endpoint, then close the item per `docs/todo_planning.md`.
