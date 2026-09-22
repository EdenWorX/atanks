# Agent Instructions for Atomic Tanks

## First Steps

1. Read `README.md` in full before touching anything. It contains the verified architecture, build commands, config paths, and
   module map.
2. Confirm repository safety: run `git status --ignored --short` and `git ls-files --other` to see what is ignored or untracked.
   Never use ignored/untracked files as evidence or edit them unless the task explicitly requires it.

## Required Reading

- `README.md` — primary technical documentation (mandatory first).
- `TODO.md` (with `docs/todo_planning.md` rules) — canonical planning overview (status table); detailed plans live in
  per-item files (`TODO_PF-<nr>.md`, `TODO_GI-<nr>.md`, `TODO_II-<nr>.md`). New work items go here as `TODO-GI-*` /
  `TODO-II-*` / `TODO-PF-*` with category-prefixed Work Package numbers (e.g. `WP PF-1.1`). Follow the fixed heading
  hierarchy (`##` phase, `###` Work Package, `####` Implementation Task, `- [ ]` Action Item) and title templates from
  `docs/todo_planning.md` when adding or restructuring items — the hierarchy lives in the per-item files, `TODO.md`
  holds only the overview table plus one row per item.
- For the task at hand, the relevant headers/sources under `src/` (see `Project Map for Agents` below).
- `TODO.md`, `TODO_Xtra.md`, `CHANGELOG.md`, and `docs/Changelog.history` when fixing bugs or changing gameplay balance —
  they record planned recent behavior changes. The legacy `TODO` file was removed after the 6.7.1 Cleanup and Modernization
  (its ideas preserved in the `Planned Features` section of `TODO_Xtra.md`); do not reference it as an existing file.
- `.clang-format` before reformatting or writing new code.

## Repository Safety Rules

- Ignored files (per `.gitignore`) that must not be read for documentation content or modified incidentally: the `atanks`
  binary, `obj/*.o`, `atanks.log`, `allegro.log`, `screenshot_*.*`, `.opencode/`, `.aiignore`, `talk_*.md`. The `atanks` binary
  and `*.log`/`screenshot_*` files are regenerable artifacts. `allegro.cfg` is tracked but gitignored; it disables vertical sync
  on Windows builds (Allegro 4 sync workaround) — preserve it, do not "clean it up".
- `AGENTS.md` itself is matched by a `.gitignore` rule; edits to it stay local unless the user force-adds it.
- Use Git-aware inspection: `git ls-files` (tracked set), `git status --ignored --short`, `git ls-files --other`, `git grep`
  (searches tracked files). Prefer `git grep` over recursive filesystem grep so ignored build output and vendored tooling are
  never searched.
- Do not create files in ignored locations (`obj/`, `.opencode/`, screenshot paths). Do not commit the `atanks` binary, `*.o`,
  `*.log`, or screenshots.

## External / Vendored Code Policy

- `src/extern/dirent.{h,c}` is third-party code (Kevlin Henney, header comment at `src/extern/dirent.h:4-12`). Do not modify it
  except intentionally for a Windows-port task, and do not derive project style rules from it.
- Allegro 4 is a system dependency, not vendored. Do not vendor it, do not edit its headers, and do not change Allegro API usage
  patterns without a task that requires it.
- `alleg44.dll` / `alleg44_64.dll` are tracked Windows release runtimes. Do not rebuild, replace, or delete them unless the task
  is a Windows release update, and follow `vs12|vs14/README_allegro.txt` when doing so.
- `.opencode/node_modules/` is untracked third-party tooling. Never analyze or edit it.

## Project Map for Agents

| Work area | Look at |
|---|---|
| Program startup, CLI flags, menu dispatch | `src/atanks.cpp` (`main`, `parse_args`) |
| Per-round state, object lists, locks | `src/globaldata.h/.cpp`, `src/globals.h`, `src/externs.h` |
| Options, players, assets, dir resolution | `src/environment.h/.cpp` |
| Enums, constants, shared types | `src/globaltypes.h/.cpp`, `src/main.h` |
| Round loop, threading, frame phases | `src/gameloop.h/.cpp` |
| Tank / missile / explosion / laser | `src/tank.h/.cpp`, `src/missile.h/.cpp`, `src/explosion.h/.cpp`, `src/beam.h/.cpp` |
| Weapon/item stats and registry | `src/weapon.h/.cpp`, `src/item.h/.cpp`, `src/files.cpp` (`load_weapons_text`), `text/weapons.toml`, `naturals.toml`, `items.toml` |
| Player state, economy, AI memory | `src/player.h/.cpp`, `src/player_types.h/.cpp` |
| Bot planning/aiming | `src/aicore.h/.cpp` (workflow documented in the header) |
| Shop, scores | `src/shop.h/.cpp`, `src/score.h/.cpp` |
| Terrain, sky, level gen | `src/land.h/.cpp`, `src/sky.h/.cpp`, `src/levelcreator.h/.cpp` |
| Menus and option screens | `src/menu.h/.cpp`, `src/optionscreens.h/.cpp`, `src/option*.h/.cpp` |
| Config, savegames, file scans | `src/files.h/.cpp` |
| Localization | `src/text.h/.cpp`, `text/*.txt` |
| Sound, clock, locks, logging | `src/sound.h/.cpp`, `src/clock.h/.cpp`, `src/spinlock.h/.cpp`, `src/debug.h/.cpp` |
| Network | `src/network.h/.cpp`, `src/client.h/.cpp` (only active with `-DNETWORK`) |
| Build | Thin cmake+ninja wrapper (`Makefile`); real build in `CMakeLists.txt` (`project(VERSION ...)` is the version single source of truth; CMake also covers Windows via VS2026). `vs12/`, `vs14/` are legacy MSVC solutions |

## Build and Test Commands

- Local Linux build and run: `make user` then `./cmake-build-release/atanks --windowed`. Prerequisites: CMake 3.25+,
  ninja, Allegro 4 development files (`allegro-config`).
- System install: `make` then `make install` (`PREFIX=`, `DESTDIR=` supported).
- Windows: open the project folder in Visual Studio 2026 (built-in CMake support) after following `README_allegro.txt` for
  the Allegro paths. (There are no Windows `Makefile` targets; the legacy `vs12/`/`vs14/` solutions target retired toolsets.)
- macOS: `make osxuser` (or `gmake osxuser`). BSD: `make bsduser`.
- Debug builds: `make debug` (general + log file), `make aidebug` (AI aiming/emotions), `make fulldebug` (all flavors, into
  `./cmake-build-debug`). Fine-grained: `make DEBUG=YES DEBUG_AICORE=YES DEBUG_LOG_TO_FILE=YES`, plus `SANITIZE_ADDRESS=YES`,
  `SANITIZE_THREAD=YES`, or `SANITIZE_UNDEF=YES` (undefined combines with either; thread flavor also defines
  `USE_MUTEX_INSTEAD_OF_SPINLOCK`).
- Dry-run to validate flags without compiling: `make -n <target>`.
- Memory/thread validation: `./do_memcheck.sh`, `./do_helgrind.sh`, `./gdb_memcheck.sh` (use with `allegro.supp` expectations).
- Static analysis: `bash tools/run-cppcheck.sh` must report no findings beyond the triaged list in its header comment (triage
  new findings instead of ignoring them). `clang-tidy -p cmake-build-release src/**/*.cpp` uses the checks from `.clang-tidy`
  (the CMake build always exports `compile_commands.json`).
- Automated unit suite: `make test` must be green (use `make test-all` for release+debug,
  `make test-asan`/`test-ubsan`/`test-tsan` for memory/thread-sensitive changes). Add manual validation by actually testing the
  change in-game (new game, buy screen, fired shots), and Valgrind for memory work.
- API documentation: `make doc` builds the Doxygen reference into `docs/html/` (requires Doxygen; the build fails on
  undocumented public APIs, so keep coverage complete).
- Clean: `make clean` / `make veryclean` (remove `cmake-build*` plus legacy `obj/*` / `atanks` leftovers). Never delete
  `obj/.keep_dir` / `dep/.keep_dir` as "cleanup" — the `.keep_dir` files keep empty directories in git.

## Coding Style

Defined by `.clang-format` (clang-format 19 or later) and the existing tree:

- `ColumnLimit: 140`, `LineEnding: LF`, `IndentWidth: 8`, `TabWidth: 8`, `UseTab: AlignWithSpaces`.
- Braces attach (all `BraceWrapping ...After*: false`, `BeforeCatch/BeforeElse: false`); short blocks only when empty
  (`AllowShortBlocksOnASingleLine: Empty`); short functions/lambdas inline; short loops allowed on one line.
- `BinPackArguments: false`, `BinPackParameters: OnePerLine` — one parameter per line in declarations.
- Pointer left, reference right (`PointerAlignment: Left`, `ReferenceAlignment: Right`); always space in angle brackets
  (`SpacesInAngles: Always`); consecutive assignments/declarations/macros are aligned (`AlignConsecutive*`); trailing comments
  always aligned.
- Includes are sorted, case-sensitive, regrouped (`SortIncludes: CaseSensitive`, `IncludeCategories` regrouping).
- Class names are upper snake (`CGlobalData`, `CEnvironment`, `CPlayer`, `CTank`, `CMissile`, `CExplosion`, `CVirtualObject`,
  `CPhysicalObject`, `CAICore` excepted); files are lowercase (`tank.cpp`, `player_types.h`); include guards use `ATANKS_...`
  (`#ifndef ATANKS_<NAME>_H_INCLUDED`, verified by `git grep "#ifndef ATANKS"` across `src/`). These are the historical
  styles, not the target: new code must follow the planned convention (normalization of the whole tree was completed in 6.7.1):
  variables/functions in snake_case (`var_name`, `func_name()`), classes/structs in PascalCase (`MyClass`,
  `MyStruct`), templates in PascalCase with `T` prefix (`TContainer`), constants and macros in UPPER_SNAKE (`MAX_SIZE`,
  `DO_NOTHING`); prefixes `C` for important classes, `T` for templates, `s` for widely-used structs. New headers use guards of
  the form `ATANKS_<header name>_H_INCLUDED` (normalized from the mistaken `ATANKS_SRC_*` form in 6.7.1).
- The 6.7 `CHANGELOG.md` entry states the tree was uniformly clang-format formatted: match surrounding code. Running
  `clang-format` after edits is mandatory for every touched header and source file (`.clang-format` is the single source
  of truth; clang-format 19 or later). Always review `git diff` afterwards and keep only the intended hunks: if the
  installed clang-format rewrites unrelated lines (version drift — the tree was formatted with v19), revert those hunks
  and match the surrounding style by hand instead of accepting them. `clang-format` does not parse `CMakeLists.txt` or
  the `Makefile`: for those files match the surrounding style manually (tabs for make recipes, which make requires).

## C/C++ Guidelines

- Standard is C++17 (`CMAKE_CXX_STANDARD 17` in `CMakeLists.txt`). Do not use C++20+ features; on Windows a Visual Studio
  with C++17 and CMake project support is required (VS2026).
- `src/main.h` is the common hub: `#include "debug.h"` must stay before the Allegro includes; keep
  `winalleg.h` handling and the MSVC shims intact.
- `src/globals.h` may only be included from `src/atanks.cpp`. Every other translation unit accesses the globals via `extern`
  declarations in `src/externs.h`.
- Fixed-width types: use `int32_t`/`uint32_t` and the atomic aliases (`ai32_t`, `abool_t` in `src/globaltypes.h`) where
  the surrounding code does.
- New `src/*.cpp` files are picked up by the CMake glob (`file(GLOB ... CONFIGURE_DEPENDS src/*.cpp)`) automatically, but
  must be added manually to `vs12/atanks.vcxproj` and `vs14/atanks.vcxproj`.

## Header and Include Guidelines

- Guard new headers with `#ifndef ATANKS_<NAME>_H_INCLUDED` / `#define ... 1` ... `#endif // ...`. (The `SRC` part in pre-6.7.1
  guards was a mistake; all guards were renamed tree-wide in 6.7.1.)
- Include order in a new gameplay header: `debug.h` (as needed for platform macros), then `main.h` or the specific dependency
  headers actually used; avoid including `globals.h` outside `src/atanks.cpp`.
- Break include cycles with forward declarations (`class CPlayer; class CTank; class CVirtualObject;`) following the pattern in
  `globaldata.h` / `environment.h`, respecting the `HAS_GLOBALDATA` / `HAS_ENVIRONMENT` guards.
- `VERSION` comes from the build (generated `config.h` for CMake, VS defines, or `src/atanks.rc`); `src/main.h` falls
  back to `"0.0.0"` when it is missing, so do not include `main.h` from standalone host tools (which must include only Allegro +
  stdio for this reason).

## Memory Management Guidelines

- Ownership is manual in entity code (`new`/`delete` appear across `src/`; e.g. `CPlayer::tank`, `TEXTBLOCK` buffers via
  `calloc`/`realloc`/`strdup`, `sort_scores()` returns a caller-owned array). When allocating, mirror the deallocation path in
  the same module (`destroy`/`unload`/`delete`) and check both creation and teardown call sites.
- `TEXTBLOCK::Load_File` owns whole-file buffers; `Find_Saved_Games()` / `Find_Bitmaps()` return caller-owned lists — free them
  at every call site, including error paths.
- `CSpinLock` (`src/spinlock.h`) is non-recursive with an owner id: never take it twice on one thread, never hold it across
  blocking calls, and keep critical sections minimal. `CTank::damage_lock` is the canonical example.
- Thread-sanitizer builds define `USE_MUTEX_INSTEAD_OF_SPINLOCK`; any new locking code must compile under both the spinlock and
  mutex configurations.
- Allegro-owned objects (`BITMAP*`, `SAMPLE*`, `FONT*`) follow Allegro create/destroy pairing; centralize lifetime in
  `CEnvironment::load*/destroy` rather than scattering it.

## Error Handling Guidelines

- Fatal startup errors (data dir, game files) return `EXIT_FAILURE` from `main` after printing a message.
  New fatal startup checks must follow this pattern, not `exit()` deep in helpers.
- `parse_args()` failures print `ERROR: ...` and return a non-zero code that `main` propagates. Keep messages on `stdout`/`stderr` consistent with the existing style.
- `CWeapon::getDelayDiv()`-style guards (zero `delay`) show the expected pattern for numeric edge cases: guard at the accessor,
  not at every call site.
- Feature-disabled paths (e.g. network game without `-DNETWORK`, see `src/atanks.cpp`) must report an in-game/menu error,
  never crash or silently no-op.

## Logging Guidelines

- Use `DEBUG_LOG(Title, Msg, ...)` and flavor macros (`DEBUG_LOG_AIM`, `DEBUG_LOG_EMO`, `DEBUG_LOG_AI`, `DEBUG_LOG_FIN`,
  `DEBUG_LOG_OBJ`, `DEBUG_LOG_PHY`) from `src/debug.h`. They compile away unless the matching `ATANKS_DEBUG*` macro is
  set, so they are safe in hot paths.
- Never `printf`/`cout` debug traces in gameplay code; use the macros so release builds stay silent. User-facing errors go
  through the existing message helpers, not the debug log.
- Log-to-file behavior is controlled by `ATANKS_DEBUG_LOGTOFILE` (`DEBUG_LOG_TO_FILE` option in `CMakeLists.txt`,
  see `src/debug.cpp`): stdout by default, `atanks.log` when enabled or on Windows. Do not hardcode log paths.
- When adding a new debug flavor, add the `ATANKS_DEBUG_<X>` guard in `debug.h` plus the `DEBUG_<X>` option and
  `-DATANKS_DEBUG_<X>` wiring in `CMakeLists.txt`.

## Platform Compatibility Guidelines

- Code must compile for Windows (Visual Studio 2026 with CMake support), Linux (clang++/g++), macOS, and BSD. Only `Windows`,
  `BSD` are accepted by `src/debug.h` (`#error` otherwise).
- Use the `atanks_snprintf` / `atanks_tzset` / `atanks_localtime` wrappers and the `main.h` MSVC mappings (`snprintf`,
  `strncpy`, `access`, `strcasecmp`, `strdup`, `unlink`, `mkdir`) instead of raw POSIX calls.
- Directory scanning goes through `wrap_dirent.h` (`extern/dirent.h` on MSVC, system `<dirent.h>` elsewhere) with the
  `Filter_File` signature split in `src/files.h` (`const struct dirent*` vs `struct dirent*` on macOS). Preserve both
  signatures.
- `PATH_MAX`/`M_PI`/`M_PIl` fallbacks live in `main.h`; do not redefine them elsewhere.
- `NETWORK` code must stay behind `#ifdef NETWORK` guards: the macOS and Windows GNU-make paths do not define it, and
  `play_networked()` has an explicit disabled-path error. Test-compile any network change with and without `-DNETWORK`. Network
  play is currently a Linux-only first draft; proper network development is deferred to a future task — do not expand its scope
  meanwhile.
- Keep `ALLEGRO_NO_MAGIC_MAIN` handling (see `main.h`, `END_OF_MAIN()` in `atanks.cpp`) intact; do not introduce a second
  `main()`.

## Generated Files and Build Artifacts

- Generated at build time: `obj/*.o`, `obj/atanks.res` (Windows), `dep/*.d` (legacy outputs only; the CMake build tracks
  dependencies internally via ninja), the `atanks`/`atanks.exe` binary. Do not edit them; regenerate via make.
- `dep/*.d` files are untracked legacy outputs; never commit them. If a header refactor changes includes, refresh them with a
  build rather than hand editing.
- `src/atanks.res` is produced from `src/atanks.rc` by the Visual Studio build (the `windres.exe` flow was retired with the
  MinGW targets); edit the `.rc`, never the `.res`.
- Runtime-generated: `atanks.log`, `screenshot_*.*`, savegames (`<configDir>/*.sav`), `music/` in the config dir. Never commit
  these.

## Where to Make Changes

- Gameplay balance (costs, damage, radii): `text/weapons.toml`, `text/naturals.toml`, `text/items.toml` numbers plus
  translations' display strings (see `docs/weapons_toml_spec.md`); sizes in `src/main.h` if counts change.
- Round/turn rules: `src/gameloop.cpp` frame loop and stage machine.
- Bot behavior: `src/aicore.cpp` (planning/aiming), personality defaults in `src/player.cpp` (`generatePreferences`), difficulty
  scaling near the `CAICore` focus/error parameters.
- Shop/economy: `src/shop.cpp`, `src/player.cpp` (`choose_item_to_buy`, `get_boost_value`, `get_money_to_save`), finance constants in
  `src/environment.h`.
- Options/menus: `src/optionscreens.cpp`, `src/menu.cpp`, `src/optionitem*.cpp`, enum additions in `src/optiontypes.h`,
  persistence in `CEnvironment::save_to_file` / `load_from_file`.
- Persistence format: `src/files.cpp` (`Save_Game`/`Load_Game`, savegame `VERSION` upgrade path noted in the 6.6
  `docs/Changelog.history` entry) — bump the savegame version when changing the format.
- New diagnostics: `src/debug.h` + `Makefile` knob wiring (see Logging).
- New source files: use license headers matching `LICENSE`, which is the single source of truth (contradicting license text
  elsewhere was cleaned up in 6.7.1 — do not propagate the old variants).
- Packaging: `Makefile` `install` target, `atanks.desktop`, `io.sourceforge.atanks.metainfo.xml` (use GitHub URLs, not
  SourceForge ones, in any packaging metadata touched meanwhile).

## Where Not to Make Changes

- `src/extern/` — third-party shim; read-only unless the task is the Windows port itself.
- `alleg44.dll`, `alleg44_64.dll`, `atanks.ico`, `atanks.png` — release binaries/icons; change only for an explicit
  asset/release task.
- `unicode.dat` — binary Allegro datafile and data-dir probe; do not edit. It will become obsolete with the post-cleanup move
  away from Allegro 4; until then it is ignored, not touched.
- `atanks.rc` version strings — bumped to 6.7.1 with this release; do not "fix" versions opportunistically outside a release.
  The `VERSION` variable in `Makefile` (currently `6.7.1`, derived from `CMakeLists.txt`) is the single source of truth.
- `dep/*.d` — regenerable tracked dependencies; do not hand-edit.
- Anything ignored/untracked (`atanks` binary, `obj/*.o`, logs, screenshots, `.opencode/`, `.idea/workspace.xml`) — leave alone.

## How to Document Unclear Findings

- If evidence is missing or contradictory, do not encode a guess in code comments, docs, or defaults. Stop and ask the user
  instead of guessing.
- Keep `README.md` and `AGENTS.md` free of `probably/likely/appears/seems` speculation. Only write verified facts into them.
- When blocked on an unclear point mid-task, ask the user via the question tool for small blockers, or capture larger ones as
  `TODO.md` items and report them at the end of the turn.

## Documentation Guidelines

- Documentation files (`AGENTS.md`, `README.md`, `TODO.md`, `TODO_Xtra.md`, `docs/*`) use a maximum line length of 128
  characters. Table rows are exempt and may be longer when a row cannot be split.
- The `.clang-format` `ColumnLimit` (currently 140) applies only to C/C++ source files and headers, never to documentation
  files.
- When writing or rewrapping documentation, preserve tables, fenced code blocks, and ASCII diagrams verbatim; only reflow prose
  paragraphs and list-item text.

## Pre-Commit Verification Checklist

1. `git status --ignored --short` shows only intended tracked files; no binaries, logs, screenshots, or `obj/` output staged.
2. `git diff` reviewed; no changes in `src/extern/`, DLLs, `unicode.dat`, `dep/*.d`, or ignored paths.
3. `make -n <target>` expands sensibly for the touched build path; full `make user` (or the platform target) compiles with no
   new `-Wall -Wextra -Wpedantic` warnings.
4. Smoke test: `./cmake-build-release/atanks --windowed` starts and the change is actually tested in-game (new game, buy
   screen, fired shots). For AI changes, run a demo game (`play_demo` path) and watch several full turns.
5. Memory/thread-sensitive changes validated with `./do_memcheck.sh` (and `./do_helgrind.sh` for locking changes).
6. New/changed options persist across restart (config save/load round-trip); savegame format changes bump the savegame version
   and old saves still load or are cleanly rejected.
7. Network changes compile and run both with `-DNETWORK` (Linux default) and without it (macOS/Windows path), including the
   disabled-path error.
8. Every touched header/source file was run through `clang-format` (19+) after editing, and `git diff` shows no
   unrelated reformatting; `CMakeLists.txt`/`Makefile` changes match the surrounding style by hand.
9. Unclear findings encountered during the work surfaced to the user instead of guessed.
10. Newly discovered work captured in `TODO.md` as `TODO-GI-*` / `TODO-II-*` / `TODO-PF-*` items (overview-table row plus
    per-item plan file per `docs/todo_planning.md`) with category-prefixed Work Package numbers; bug reports go to
    `https://github.com/EdenWorX/atanks/issues` (use GitHub URLs, not SourceForge ones, in any touched references).
