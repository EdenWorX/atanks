# Changelog

Releases use semantic versioning (`MAJOR.MINOR.PATCH`); all changes are documented here on `MINOR` and `MAJOR` version
changes. Finished to-do items from `TODO.md` are removed once their essence is documented here (see `docs/todo_planning.md`
for the planning rules and `docs/release_process.md` for the release process). Older history is preserved exactly as it was in
`docs/Changelog.history`.

## 6.7.1 (2026-09-21)

### Added

- CMake build (`CMakeLists.txt`, CMake 3.25+, ninja mandatory) with `project(VERSION ...)` as the version single source of
  truth; generated `config.h` (`config.h.in`) carries the version, data-dir, platform, and `NETWORK` macros; install rules
  mirror the old `make install` layout (binary, metainfo, desktop file, icons, data tree) with `DESTDIR` staging support.
- `Makefile` kept as a thin cmake+ninja wrapper (`make`, `make user/osxuser/bsduser`, `make debug/aidebug/fulldebug`,
  `SANITIZE_ADDRESS/THREAD/UNDEF` knobs, one build directory per flag set). Stale Windows targets (`winuser`, `win32-dist`,
  the `windres.exe` resource flow) retired; Windows builds go through CMake (Visual Studio 2026).
- CppUTest unit suite (`tests/`, decoupled logic units) wired to `ctest`: `make test`, `make test-all`, `make test-asan`,
  `make test-ubsan`, `make test-tsan`. The dependency resolves in three tiers: local package, pkg-config probe, FetchContent
  (`v4.0`) as last resort.
- Static analysis: `tools/run-cppcheck.sh` plus `.clang-tidy` with `compile_commands.json` exported from the CMake build.
- API reference: `make doc` (Doxygen) with complete public-API coverage; the build fails on undocumented public APIs.
- Documented TOML arsenal format (`docs/weapons_toml_spec.md`): `text/weapons.toml`, `text/naturals.toml`, `text/items.toml`
  (56 weapons, 6 naturals, 24 items) plus per-language `weapons_*.toml` display-string translations, parsed with tomlplusplus
  (three tiers: CMake package, system header probe, FetchContent `v3.4.0`); the legacy positional `text/weapons*.txt` files
  are removed.
- Planning and release rules (`docs/todo_planning.md`, `docs/release_process.md`); the legacy `Changelog` is preserved
  untouched as `docs/Changelog.history`.

### Changed

- License consolidated on `LICENSE` (GPL-3.0): source headers, AppStream metadata (`GPL-3.0-or-later`); `COPYING` is now a
  pointer to `LICENSE`.
- Version truth consolidated at 6.7 (Visual Studio projects, Windows resource, AppStream release entry); stale strings swept.
- Project references moved from SourceForge to GitHub (issue tracker, URLs, update-checker endpoints, help texts); the legacy
  SourceForge update checker is disabled; the AppStream ID is renamed to `io.github.EdenWorX.atanks`.
- Obsolete artifacts removed: `Makefile.bsd`, `atanks-4.3.spec`, tracked `dep/*.d` files, `exporter/`, `cb/` (Code::Blocks),
  `-DUBUNTU` branches and the `ubuntu` target.
- Naming normalized tree-wide (`C`-prefixed classes, `E`-prefixed enums, snake_case functions/variables/members,
  `ATANKS_<NAME>_H_INCLUDED` guards, `.clang-format` at 128 columns); the convention is recorded in `AGENTS.md`/`README.md`.
- Config and savegame parsing modernized: one shared growing line reader (`read_config_line`/`split_config_field` in
  `src/files.cpp`) replaces the static char buffers (`MAX_CONFIG_LINE` removed); overlong lines no longer split silently.
- Remaining risky C-string handling replaced (player names are `std::string` now).
- `README.md` rewritten as the verified architecture/build/config reference; new `AGENTS.md` agent instructions.

### Fixed

- Frame-rate independence: per-frame updates across entities, physics, animations, menus, and AI scale with the refresh rate
  (velocities linearly, accelerations such as gravity quadratically); the game no longer runs fast and shots no longer fall
  short on high-refresh-rate displays (identical behavior verified at 60 vs 120 FPS).
- Empty roster no longer aborts menu actions: PLAY/PLAYERS with no players force-creates a human player plus the default AI
  set (`create_human_player`/`create_ai_players`, guarded `menu.distribute`).
- Player screens no longer crash on missing tank bitmaps (null-bitmap guard in `src/menu.cpp`).
- Background music handling: missing music files clean up `background_music` and disable `play_music` instead of failing; the
  music-folder path typo (`/tank_gun/` → `/tankgun/`) is corrected.

## 6.7 (2023-08-09)

### Added

- The `Makefile` got some debugging targets for easier debug mode building.
- Random number generation has been modernized and was made thread local.
- Beautified land and moon creation.

### Changed

- Source code is now formatted uniformly using clang-format.
- Got rid of C90 style pointer juggling where possible (aka where performance is non-critical).
- Bot values have been lowered and the spread between worst and best AI player has been increased. The DEADLY player is also
  no longer immune to making errors.
- SDI will no longer shoot a missile down that is going to be repulsed.
- Some recursive functions have been rewritten to be non.recursive.
- Some "spaghetti-code" style functions have been split up.
- Many C++17 perks have been made use of where they make sense.
- Got rid of sscanf(). Everywhere.
- Decoupled many objects that deserved their own compilation unit.
- SDI repulsor calculation is now independent from AI level of its owner.
- Enhance armour/amp/weapon boosting, so that AI players no longer forget what they wanted to buy (but couldn't) between
  rounds.
- The maximum SDI range calculation has been rewritten, and the repulsor shields have been strengthened a bit.
- AI takes target level and already dealt damage ratios more into account. This stops the bots to get "stuck" on a (mainly
  human) opponent.
- AI does not longer know the exact coordinates of their opponent, but "drift" a bit according to their level.

### Fixed

- Fixed a bug that caused background updates to glitch.
- Added missing Theft Bomb missile image, that caused an assertion failure whenever someone tried to shoot one.
- Fixed all g++ and clang-tidy warnings.
- Fixed a bug where global.surface was being used as height instead of the surface y-coordinate.
- Fixed a bug in land creation that caused possible crashes.
- Fixed a bug that allowed AI players to have a ton of amps but no armour and vice versa.
- Remove C89 style TRUE/FALSE integers where possible.
- Fixed typo: Swapper costs 4 grand, not 40k.
- Fixed a bug that caused AI to ignore 90% of the items they might have wanted to save their money on.
- Fixed a bug that caused the AI to break off their attack planning early just to shoot a Riot Bomb anywhere.
- Fixed a bug that caused the AI to use the first guess on their last attack planning attempt without actually planning it
  through.
- Fixed a bug that could cause a segfault when fast-forward-play ends.
