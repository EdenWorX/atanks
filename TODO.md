# TODO

Canonical planning rules: `docs/todo_planning.md`. Release rules:`docs/release_process.md`.

Numbering: `TODO-GI-*` (defects), `TODO-II-*` (design constraints), `TODO-PF-*` (planned features). Work Packages,
Implementation Tasks, and Action Items always use the full category-prefixed number (e.g. `WP PF-1.1`, task `PF-1.1.1`), never
a bare relative number. The document structure (heading hierarchy, title templates) is fixed in `docs/todo_planning.md`. Mark
items complete by changing `[ ]` to `[x]`; a parent is complete only when all its children are complete. Finished items are
removed once their essence is documented in `CHANGELOG.md`.

## [ ] TODO-PF-1: Cleanup and Modernization

Retire obsolete build/packaging artifacts, consolidate license and version truth, move project references from SourceForge to
GitHub, remove ancient workarounds and obsolete limits, normalize guards and naming, and migrate the project to the EdenWorX
planning and release rules (`docs/todo_planning.md`, `docs/release_process.md`). Source: user answers (rounds of 2026-09-17 and
2026-09-18) plus the `Changelog` preservation decision, plus gap analysis of the `docs/*` target state (which describes the
project as if this cleanup was already done).

### [x] PF-1.1: Consolidate license information

`LICENSE` is the single source of truth. Fix the contradicting license information throughout the project: `COPYING` (GPLv2
text), the `either version 2 ... or (at your option) any later version` source headers (e.g. `src/main.h`, `src/globaltypes.h`),
the `GPL-2.0-or-later` declaration in `io.github.EdenWorX.atanks.metainfo.xml`, and the `License: GPL` line in `atanks-4.3.spec`
(covered by its removal in `WP PF-1.4`).

#### [x] PF-1.1.1: Inventory contradicting license statements

Grep the tracked tree for license identifiers (`GPL`, `General Public License`, `COPYING`, `License:`) including `README`,
`README_ru.txt`, packaging files, and all `src/*.h`/`src/*.cpp` headers. Expected result: a complete hit list that drives tasks
`PF-1.1.2`–`PF-1.1.4`. No files changed.

#### [x] PF-1.1.2: Normalize source-file license headers to LICENSE

Replace the `either version 2 ... or (at your option) any later version` headers with headers matching `LICENSE`, keeping the
existing copyright holders and the surrounding comment style. Pure text change; no code behavior changes.

#### [x] PF-1.1.3: Align packaging and metadata declarations

Set the license fields in `io.github.EdenWorX.atanks.metainfo.xml` (`project_license`) and any remaining packaging metadata
to the `LICENSE` identifier. Coordinate with `WP PF-1.4` (the spec file is deleted there, not fixed here).

#### [x] PF-1.1.4: Decide the fate of COPYING and verify install lists

Either remove `COPYING` or replace it with a pointer to `LICENSE` (decision with the user), then verify the `Makefile`
`INCOMMON` install list and any other file enumerations are consistent with the outcome.

### [x] PF-1.2: Consolidate version information

The `VERSION` variable in `Makefile` (currently `6.7`) is the single source of truth. Propagate it to `vs12/atanks.vcxproj` and
`vs14/atanks.vcxproj` (currently `6.5_rc1`), `src/atanks.rc` (currently `6, 4, 99`), and
`io.github.EdenWorX.atanks.metainfo.xml` (currently `6.5`). `Makefile.bsd` and `atanks-4.3.spec` are covered by their removal in
`WP PF-1.3`/`WP PF-1.4`. Task `PF-1.2.4` runs last, after `WP PF-1.3`, `WP PF-1.4`, and `WP PF-1.10` (which creates the history
file the sweep excludes).

#### [x] PF-1.2.1: Propagate 6.7 to the Visual Studio projects

Replace `6.5_rc1` with the `Makefile` version in all configurations (Debug/Release x Win32/x64) of `vs12/atanks.vcxproj` and
`vs14/atanks.vcxproj`. Preserve the existing per-project differences (e.g. the `DATA_DIR` define present only in `vs14`).
Expected result: all MSVC builds report the same version string.

#### [x] PF-1.2.2: Sync the Windows resource version

Update `VERSIONINFO` in `src/atanks.rc` to the `Makefile` version and verify the `windres.exe` build still produces
`obj/atanks.res` (until that flow is retired in `WP PF-1.9`). No behavior change besides the version.

#### [x] PF-1.2.3: Update the metainfo release entry

Set the release entry in `io.github.EdenWorX.atanks.metainfo.xml` to version `6.7`, released `2023-08-09` (verified from git
log, commit `571c0be`).

#### [x] PF-1.2.4: Sweep for stale version strings

After `WP PF-1.3`/`WP PF-1.4` removals, grep the tracked tree for leftover stale versions (`6.5_rc1`, `6, 4, 99`, `4.3`) outside
`docs/Changelog.history` (created in `WP PF-1.10`). Expected result: zero hits.

### [x] PF-1.3: Remove deprecated `Makefile.bsd`

The file is deprecated (stale `VERSION 6.5`, references to untracked `src/main.cpp` and `imagedefs.h`) and will be removed, not
fixed.

#### [x] PF-1.3.1: Delete the file and its documentation mentions

Delete `Makefile.bsd` and remove its mentions from `README.md` and `AGENTS.md` build docs. The GNU `make bsduser` path is
unaffected.

#### [x] PF-1.3.2: Verify no references or users remain

Confirm `git grep Makefile.bsd` returns no hits outside `docs/Changelog.history`, and that the documented BSD build (`make
bsduser`) is still described correctly.

### [x] PF-1.4: Remove obsolete packaging and dependency artifacts

Remove `atanks-4.3.spec` (from 2015 or older, no longer needed) and the tracked `dep/*.d` files (obsolete, regenerable via
`Makefile`).

#### [x] PF-1.4.1: Delete the RPM spec and its documentation mentions

Delete `atanks-4.3.spec` and remove its mentions from `README.md` and `AGENTS.md` packaging docs. No replacement; downstream
packagers own their specs.

#### [x] PF-1.4.2: Untrack the dependency files and ignore them

`git rm --cached` the tracked `dep/*.d` files and add an ignore rule, then verify a clean checkout regenerates them via the
`Makefile` dependency rules (`dep/%.d`, `-include $(DEPENDS)`). Build output must be unchanged.

#### [x] PF-1.4.3: Verify consistency of the remaining file lists

Confirm no references to the spec or to tracked `.d` files remain (`git grep atanks-4.3.spec`, `git ls-files dep/` shows only
`.keep_dir`), and that install/packaging docs no longer promise them.

### [x] PF-1.5: Clean up `exporter/` and `cb/`

Both date from 2015: no Allegro datafiles have been used since then (`exporter/export.cpp`, `exporter/move.cpp`), and
Code::Blocks has been unsupported for almost a decade (`cb/atanks.cbp`, `cb/atanks.workspace`).

#### [x] PF-1.5.1: Delete `exporter/` and update the asset workflow docs

Delete `exporter/export.cpp` and `exporter/move.cpp`, and update the `README.md` "new asset" workflow (which cites
`exporter/move.cpp` for bitmap renumbering) to describe the manual numbering step. Game behavior unchanged.

#### [x] PF-1.5.2: Delete `cb/` and update the build docs

Delete `cb/atanks.cbp` and `cb/atanks.workspace`, and remove the Code::Blocks rows from the `README.md` build docs and
`AGENTS.md` project map. The `Makefile` itself is untouched.

#### [x] PF-1.5.3: Verify no references remain

Confirm `git grep -E "exporter/|atanks\.cbp"` returns no hits outside `docs/Changelog.history`.

### [x] PF-1.6: Move project references from SourceForge to GitHub

Issue reports go to `https://github.com/EdenWorX/atanks/issues` from now on. Historical `Changelog` entries keep their
SourceForge URLs untouched (preserved as `docs/Changelog.history` in `WP PF-1.10`).

#### [x] PF-1.6.1: Update live SourceForge references to GitHub

Update the live references in `README`, `README_ru.txt` (URLs only, preserving the Russian prose), `Makefile` comments,
`credits.txt`, `io.github.EdenWorX.atanks.metainfo.xml` (homepage, bugtracker, update contacts), the `src/atanks.cpp`
update-checker endpoints (verify the replacement endpoints with the user before changing), and the `text/Help*.txt` display
strings (all 7 languages, strings only). No functional changes besides the endpoints.

#### [x] PF-1.6.2: Replace the bug-tracker placeholder

Replace the `xxx` bug-tracker placeholder (`README:240-241`) with `https://github.com/EdenWorX/atanks/issues`.

#### [x] PF-1.6.3: Verify the migration is complete and history untouched

Confirm `git grep -i sourceforge` returns hits only in `docs/Changelog.history`, the `TODO*.md` plan records, the `#if 0`
-masked legacy checker in `src/atanks.cpp` (kept deliberately, with its replacement tracked in `TODO_Xtra.md`), the
descriptive migration prose in `README.md`, the historical contributor addresses in `credits.txt` (kept per user decision),
and the metainfo `update_contact` plus screenshot URLs (contact kept per user decision; screenshots carved out in
`TODO_Xtra.md`), and that the history file is byte-identical to the former `Changelog` (verified: commit `89ce8a3` renames
with zero changes; empty `git diff` between the pre- and post-rename blobs, see `WP PF-1.10`).

#### [x] PF-1.6.4: Rename the AppStream component ID to the fork

This fork coexists with the still-active upstream project on SourceForge, which keeps its own `io.sourceforge.atanks` identity;
shipping the same AppStream ID would collide in software centers. Rename `io.github.EdenWorX.atanks.metainfo.xml` to
`io.github.EdenWorX.atanks.metainfo.xml` (via `git mv`), set `<id>` to `io.github.EdenWorX.atanks`, and update the `Makefile`
install rule plus all documentation references. Keep `atanks.desktop` as the launchable and the screenshot block untouched.
### [x] PF-1.7: Document `allegro.cfg`

Record that it disables vertical sync on Windows builds because Allegro 4 has sync problems there. Full UI-framework
modernization away from Allegro 4 is explicitly deferred to a future to-do item (see planned follow-ups below).

#### [x] PF-1.7.1: Document the role in README.md

Add a short paragraph to the platform-specific build docs: what `allegro.cfg` does (disables vertical sync on Windows as an
Allegro 4 sync workaround), that the tracked copy is preserved as-is, and that broader UI modernization is deferred. No file
content changes.

#### [x] PF-1.7.2: Confirm agent guidance is consistent

Verify `AGENTS.md` already instructs agents to preserve `allegro.cfg` and not "clean it up"; adjust wording only if it
contradicts the `README.md` paragraph from `PF-1.7.1`.

### [ ] PF-1.8: Leave the legacy `TODO` file alone

The file is almost a decade old and explicitly out of scope for now: do not triage it, do not act on it. Proper `TODO-PF-*`
entries will be created by the user after `TODO-PF-1` is done.

#### [ ] PF-1.8.1: Verify the legacy TODO file is untouched at PF-1 completion

At the end of `TODO-PF-1`, confirm `git diff --quiet -- TODO` reports no changes to the legacy file. If any PF-1 work touched
it accidentally, revert. The user then creates post-PF-1 entries.

None of the following exists yet: `CMakeLists.txt`, `config.h.in` / generated `config.h`, `CHANGELOG.md`, `make test` /
`test-all` / `test-asan` / `test-ubsan` / `test-tsan`, `tools/run-cppcheck.sh`, `make doc`, Doxygen coverage of public APIs.

### [x] PF-1.9: Migrate the build to CMake, keep `Makefile` as wrapper

Add `CMakeLists.txt` with `project(VERSION ...)` as the single source of truth for the version, add `config.h.in` generating
`config.h` with the version macros (replacing `-DVERSION=` / `-DDATA_DIR=` flag plumbing), and port targets, flags, platform
paths, and install rules. The legacy `Makefile` stays as a thin wrapper so plain `make`, `make test`, `make install`, and so on
keep working, but it only wraps cmake+ninja. All Windows targets in the `Makefile` (`winuser`, `win32-dist`, the `windres.exe`
resource build) are stale and are retired as part of this migration — Windows builds go through CMake (Visual Studio 2026
with C++17 and CMake support); `vs12/`/`vs14/` are legacy solutions.

#### [x] PF-1.9.1: Create CMakeLists.txt with version and build options

Add `CMakeLists.txt` declaring `project(... VERSION 6.7 ...)` as the single source of truth, with options covering install
layout (`PREFIX`/`DESTDIR` equivalents), `DEBUG` flavors, and sanitizers. Agree the minimum CMake version and the ninja
prerequisite with the user. Expected result: a configured build tree produces the same `atanks` binary layout as the legacy
`Makefile`.

#### [x] PF-1.9.2: Generate config.h from config.h.in

Add `config.h.in` producing a `config.h` with the version macros, replacing the `-DVERSION=` / `-DDATA_DIR=` (and related
`-DNETWORK` / platform-define) flag plumbing. Keep the `src/main.h` missing-`VERSION` hard-error satisfied through the generated
header; behavior of version reporting (`game_version`) must be preserved.

#### [x] PF-1.9.3: Port install rules and verify equivalence

Port the `install` target (binary, metainfo, desktop file, icons, data tree) to CMake and verify byte-equivalent install results
against the legacy `make install`, including a staged `DESTDIR` install.

#### [x] PF-1.9.4: Convert the Makefile to a thin cmake+ninja wrapper

Rewrite `Makefile` so plain `make`, `make test`, `make install`, etc. delegate to cmake+ninja while keeping their familiar names
and variables. Retire the stale Windows targets (`winuser`, `win32-dist`, the `windres.exe` resource flow); Windows builds
go through CMake (VS2026) while `vs12/`/`vs14/` stay as legacy solutions. GNU platform targets (`user`, `osxuser`, `bsduser`,
`debug`/`aidebug`/`fulldebug`) must keep working through the wrapper. The wrapper owns the build directory: base
`./cmake-build` plus option postfixes (`DEBUG=NO` → `-release`, `DEBUG=YES` → `-debug`, `SANITIZE_ADDRESS=YES` → `-asan`,
`-release`, `DEBUG=YES` → `-debug`, `SANITIZE_ADDRESS=YES` → `-asan`, `SANITIZE_THREAD=YES` → `-tsan`), with `-usan`
appended for `SANITIZE_UNDEF=YES`. Examples: `make` → `./cmake-build-release`, `make DEBUG=YES` → `./cmake-build-debug`,
`make DEBUG=YES SANITIZE_THREAD=YES` → `./cmake-build-tsan`, `make SANITIZE_ADDRESS=YES SANITIZE_UNDEF=YES` →
`./cmake-build-asan-usan`. Any sanitizer implies `DEBUG=YES`, so no extra `-debug` postfix is added. `SANITIZE_LEAK` is
dropped (lsan is part of asan now); `SANITIZE_UNDEF` (ubsan) is new. Address and thread sanitizers are mutually exclusive
(address wins) but each combines with undefined.

#### [x] PF-1.9.5: Update the build documentation

Update the `README.md` build system docs and the `AGENTS.md` build/test commands to the CMake flow (wrapper commands,
prerequisites, VS2026 with CMake support for Windows). Remove documentation of retired targets.

### [x] PF-1.10: Adopt `CHANGELOG.md` and semantic versioning

Keep the legacy `Changelog` (2500+ lines) exactly as it is — SourceForge URLs and all — by renaming it to
`docs/Changelog.history` to preserve its historical context. Create a fresh `CHANGELOG.md` with a single entry for the `6.7`
release (released 2023-08-09 per git log, commit `571c0be`): `## 6.7 (2023-08-09)` with `Added` / `Changed` / `Fixed` sections
rewritten in markdown from the `Atanks-6.7 released` section of `docs/Changelog.history`. Adopt `MAJOR.MINOR.PATCH` numbering
going forward; document `MINOR`/`MAJOR` changes per release and apply the removal rule (finished to-do items removed after
their `CHANGELOG.md` entry). Task `PF-1.10.1` runs after `WP PF-1.5` (so no Code::Blocks unit list needs updating); the history
file it creates is the carve-out for the `PF-1.2.4` and `PF-1.6.3` sweeps.

#### [x] PF-1.10.1: Rename the history file and fix references

`git mv Changelog docs/Changelog.history` (after `WP PF-1.5`, so no Code::Blocks unit list needs updating), then update the
`Makefile` `INCOMMON`/install lists and the `README.md`/`AGENTS.md` references to point at the new path. Verify the renamed
file is byte-identical to the original blob.

#### [x] PF-1.10.2: Create CHANGELOG.md with the 6.7 entry

Create `CHANGELOG.md` containing only `## 6.7 (2023-08-09)` with `Added` / `Changed` / `Fixed` sections rewritten in markdown
from the `Atanks-6.7 released` section of `docs/Changelog.history`. No new claims beyond that section; later releases append
entries per `docs/release_process.md`.

#### [x] PF-1.10.3: Record the semver and removal rules going forward

Document that new versions use `MAJOR.MINOR.PATCH` (tied to the CMake `project(VERSION ...)` format from `WP PF-1.9`) and that
finished to-do items are removed after their `CHANGELOG.md` entry, per `docs/todo_planning.md`. No code changes.

### [x] PF-1.11: Add test and sanitizer targets

Implement `make test` and `make test-all`, plus `make test-asan`, `make test-ubsan`, and `make test-tsan` (mapping the existing
`SANITIZE_*` Makefile knobs), so the pre-release checklist in `docs/release_process.md` is executable. Tasks `PF-1.11.1`–
`PF-1.11.2` build on the `WP PF-1.9` CMake base.

#### [x] PF-1.11.1: Define and implement make test / test-all

Agree with the user what `test`/`test-all` cover for an interactive GUI application without a test suite, then implement the
targets on top of the `WP PF-1.9` CMake base. Expected result: `make test` and `make test-all` run green on a clean checkout.
Scope agreement (recorded): unit tests cover exactly the decoupled logic units (verified zero Allegro/global references) —
`random`, `zbuffer`, `spinlock`, `optiontypes`, `box`, `clock`, `globaltypes` — as a CppUTest suite in `tests/` wired to
`ctest`. `make test` builds and runs the suite in the directory selected by the current flags (`DEBUG` and sanitizers pick
`cmake-build-debug`, `-asan`, `-tsan`, `-usan`); `make test-all` runs it in both `cmake-build-release` and `cmake-build-debug`
(sanitizer runs belong to the `test-asan`/`test-tsan`/`test-ubsan` targets of `PF-1.11.2`). Interactive smoke tests stay manual.
Framework decision (recorded): CppUTest, consumed via FetchContent pinned to release tag `v4.0` (both `CppUTest` and
`CppUTestExt` libraries — mocking lives in Ext). To go online only when really necessary, resolve the dependency in this
order: (1) `find_package(CppUTest 4.0 QUIET)` against a local install, (2) `pkg_check_modules(... IMPORTED_TARGET
cpputest>=4.0)` probe, (3) FetchContent as last resort. Verified on the dev machine: system install is version 4.0 (config
file, header macro, pkg-config agree); the pkg-config probe finds it (`CPPUTEST_FOUND=1 ver=4.0`) and a mock-based smoke test
linked via `PkgConfig::CPPUTEST` compiles, links, and runs green. Note: that machine's installed CMake config is internally
inconsistent (references `/usr/lib/libCppUTest.a`, actual libs in `/usr/lib64`) so tier (1) hard-fails there until the install
is repaired (reported to the Gentoo maintainers) — which is exactly why the pkg-config tier (2) exists.

#### [x] PF-1.11.2: Implement the sanitizer targets

Implement `make test-asan`, `make test-ubsan`, and `make test-tsan`, mapping the `SANITIZE_ADDRESS` / `SANITIZE_UNDEF` /
`SANITIZE_THREAD` knobs (thread flavor keeps `USE_MUTEX_INSTEAD_OF_SPINLOCK`). Expected result: each target builds and runs the
test scope from `PF-1.11.1` under its sanitizer.

#### [x] PF-1.11.3: Update the validation documentation

Replace the interim `make DEBUG=YES` + manual in-game validation rule in `README.md`/`AGENTS.md` with the new test targets once
they exist. Until then the interim rule stays.

### [x] PF-1.12: Add static analysis, doc builds, and Doxygen coverage

Add `tools/run-cppcheck.sh`, clang-tidy integration (`clang-tidy -p cmake-build-release src/**/*.cpp`), a `make doc` target, and
Doxygen comments for all public APIs, so `make doc` is warning-free per the release checklist. Verify `make install` and `make
install PREFIX=/tmp/ewx-test` as part of this work.

#### [x] PF-1.12.1: Add tools/run-cppcheck.sh

Add the `tools/run-cppcheck.sh` runner referenced by `docs/release_process.md` with the project's include paths and
suppressions, and document when to run it. Expected result: the script runs clean on the tree (or reports only triaged
findings).

#### [x] PF-1.12.2: Integrate clang-tidy via the CMake build

Ensure the `WP PF-1.9` CMake build exports compile commands so `clang-tidy -p cmake-build-release src/**/*.cpp` works, and
record the invocation in the workflow docs.

#### [x] PF-1.12.3: Add make doc and Doxygen coverage for public APIs

Add the `make doc` target (Doxygen config included) and write Doxygen comments for all public APIs module by module — split into
Action Items per module below. Expected result: `make doc` builds with no undocumented-public-API warnings.

- [x] **PF-1.12.3.1**: Add the Doxygen config and wire the `make doc` target
  (Doxyfile with undocumented-API warnings as errors, CMake custom target, Makefile wrapper goal; exclude third-party
  `src/extern/dirent.h` and MSVC-generated `src/resource.h`).
- [x] **PF-1.12.3.2**: Document core state and hub headers
  (`main.h`, `globaldata.h`, `environment.h`, `globaltypes.h`, `externs.h`, `globals.h`).
- [x] **PF-1.12.3.3**: Document entities
  (`virtobj.h`, `physobj.h`, `tank.h`, `missile.h`, `explosion.h`, `beam.h`, `debris_pool.h`, `decor.h`, `teleport.h`,
  `floattext.h`).
- [x] **PF-1.12.3.4**: Document player state and AI
  (`player.h`, `player_types.h`, `aicore.h`).
- [x] **PF-1.12.3.5**: Document menus, options, and UI widgets
  (`menu.h`, `optionscreens.h`, `optiontypes.h`, `optioncontent.h`, `optionitem.h`, `optionitembase.h`,
  `optionitemcolour.h`, `optionitemmenu.h`, `optionitemplayer.h`, `box.h`, `button.h`).
- [x] **PF-1.12.3.6**: Document world generation
  (`land.h`, `sky.h`, `levelcreator.h`, `moon.h`, `satellite.h`, `random.h`; `perlin.cpp` has no header, cover it from the
  `levelcreator.h`/`sky.h` docs).
- [x] **PF-1.12.3.7**: Document services
  (`files.h`, `text.h`, `sound.h`, `clock.h`, `winclock.h`, `spinlock.h`, `zbuffer.h`, `debug.h`, `update.h`, `gfxData.h`,
  `bitmap.h`, `wrap_dirent.h`).
- [x] **PF-1.12.3.8**: Document round driver, meta, arsenal, and network
  (`gameloop.h`, `shop.h`, `score.h`, `weapon.h`, `item.h`, `network.h`, `client.h`).
- [x] **PF-1.12.3.9**: Verify `make doc` builds warning-free
  (full run with zero undocumented-public-API warnings; record the invocation in `README.md`/`AGENTS.md`).

#### [x] PF-1.12.4: Verify install targets per the checklist

Run `make install` and `make install PREFIX=/tmp/ewx-test` and verify the installed tree (binary, metainfo, desktop file, icons,
data) matches expectations.

### [x] PF-1.13: Remove the `UBUNTU` workarounds

The `-DUBUNTU` flag (`Makefile:155-157`) and every `#ifdef UBUNTU` branch are ancient and long-forgotten; remove the flag, the
branches, and the `ubuntu` build target, and delete the workaround from the user documentation.

#### [x] PF-1.13.1: Remove the flag, branches, and build target

Delete the `-DUBUNTU` wiring and the `ubuntu` target from `Makefile`, and remove every `#ifdef UBUNTU` branch (keeping the
non-`UBUNTU` code path in each case). Verify `git grep -i UBUNTU` returns no source hits and that a `make DEBUG=YES` build still
compiles. No behavior change outside the removed workaround.

#### [x] PF-1.13.2: Remove the workaround from the documentation

Delete the Ubuntu sound-workaround instructions from `README` user docs and the `ubuntu` rows from the `README.md`/`AGENTS.md`
build docs.

### [x] PF-1.14: Normalize header include guards

The `SRC` part in `ATANKS_SRC_<NAME>_H_INCLUDED` was a mistake (no headers live elsewhere). Rename all guards to
`ATANKS_<header name>_H_INCLUDED` and record that form in `AGENTS.md` / `README.md`.

#### [x] PF-1.14.1: Rename all guards to ATANKS_<NAME>_H_INCLUDED

Rename every `ATANKS_SRC_*_H_INCLUDED` guard (plus outliers such as `MAIN_DEFINE` in `src/main.h`) to the `ATANKS_<header
name>_H_INCLUDED` form, script-assisted with per-file review. Third-party `src/extern/dirent.h` keeps its own guard; the
already-conforming `ATANKS_WRAP_DIRENT_H` is untouched.

#### [x] PF-1.14.2: Verify with a full rebuild

Rebuild all GNU targets including the `DEBUG`/`aidebug`/`fulldebug` flavors to prove the renames broke nothing, and confirm the
`AGENTS.md` guard rule matches the final tree.

### [ ] PF-1.15: Normalize naming conventions (own Work Package, major refactoring)

Apply the planned convention throughout the tree: variables/functions → snake_case (`var_name`, `func_name()`), classes/structs
→ PascalCase (`MyClass`, `MyStruct`), templates → PascalCase with `T` prefix (`TContainer`), constants/macros → UPPER_SNAKE
(`MAX_SIZE`, `DO_NOTHING`); prefixes `C` for important classes, `T` for templates, `s` for widely-used structs. Record the
convention in `AGENTS.md` / `README.md`.

#### [x] PF-1.15.1: Build the rename mapping and agree it with the user

Inventory the current names per module and produce the old→new mapping table (e.g. `GLOBALDATA`, `ENVIRONMENT`, `PLAYER`,
`TANK`, `MISSILE`, `EXPLOSION`, `VIRTUAL_OBJECT`, `PHYSICAL_OBJECT`, `AICore` and helpers such as `sScore`, `sGfxData`,
`TEXTBLOCK`, `ZBuffer`, `Menu`, `OptionItem`). No renames yet; the mapping is reviewed with the user first because of the
refactoring size.

Agreed mapping (reviewed with the user; decisions recorded below):
- Important classes get the `C` prefix: `GLOBALDATA` → `CGlobalData`, `ENVIRONMENT` → `CEnvironment`, `PLAYER` → `CPlayer`,
  `TANK` → `CTank`, `MISSILE` → `CMissile`, `EXPLOSION` → `CExplosion`, `VIRTUAL_OBJECT` → `CVirtualObject`,
  `PHYSICAL_OBJECT` → `CPhysicalObject`, `BEAM` → `CBeam`, `DECOR` → `CDecor`, `TELEPORT` → `CTeleport`,
  `FLOATTEXT` → `CFloatText`, `SATELLITE` → `CSatellite`, `TEXTBLOCK` → `CTextBlock`, `ZBuffer` → `CZBuffer`,
  `Menu` → `CMenu`, `BUTTON` → `CButton`, `WEAPON` → `CWeapon`, `ITEM` → `CItem`, `MESSAGE_QUEUE` → `CMessageQueue`,
  `LevelCreator` → `CLevelCreator`, `AICore` → `CAICore`.
- Non-template option items get the `C` prefix: `OptionItemBase` → `COptionItemBase`,
  `OptionItemColour` → `COptionItemColour`, `OptionItemMenu` → `COptionItemMenu`, `OptionItemPlayer` → `COptionItemPlayer`.
- Only `OptionItem` in `optionitem.h` is a class template (verified: all other `template<>` declarations are member-function
  templates, i.e. `getHeadOfClass`, `getPrev`/`getNext`, and the `optionitembase.h` dispatchers), so only it gets the `T`
  prefix: `OptionItem` → `TOptionItem`.
- Already conforming, keep: `CSpinLock`, `sScore`, `sGfxData`, `sOpponent`, `sDebrisItem`, `sDebrisPool`, `sItemListEntry`,
  `sOppMemEntry`, `sWeapListEntry`, `sSDI`.
- Structs to rename: `gradient` → `sGradient`, `MESSAGE` → `sMessage`, `SEND_RECEIVE_TYPE` → `sSendReceive`,
  `POINT_t` → `Point`, `PLAYER_mini` → `PlayerMini`, `update_data` → `UpdateData`, `BOX` → `sBox`.
- Enums get the `E` prefix in PascalCase (enumerators stay UPPER_SNAKE): `eBackgroundTypes` → `EBackgroundTypes`,
  `eBoxModes` → `EBoxModes`, `eColourTheme` → `EColourTheme`, `eControl` → `EControl`, `eDataStage` → `EDataStage`,
  `eFileStage` → `EFileStage`, `eFullScreen` → `EFullScreen`, `eLandscapeTypes` → `ELandscapeTypes`,
  `eLandSlideTypes` → `ELandSlideTypes`, `eLanguages` → `ELanguages`, `eRoundStages` → `ERoundStages`,
  `eSatelliteLaser` → `ESatelliteLaser`, `eSaveGameStage` → `ESaveGameStage`, `eSkipPlayType` → `ESkipPlayType`,
  `eSoundDriver` → `ESoundDriver`, `eSounds` → `ESounds`, `eTurnTypes` → `ETurnTypes`, `eViolentDeath` → `EViolentDeath`,
  `eWallTypes` → `EWallTypes`, `eWinner` → `EWinner`, `eMissileType` → `EMissileType`, `eBeamType` → `EBeamType`,
  `eMenuClass` → `EMenuClass`, `eMenuReturnCodes` → `EMenuReturnCodes`, `eTextClass` → `ETextClass`,
  `eTextSway` → `ETextSway`, `eEntryType` → `EEntryType`, `ePlayerStages` → `EPlayerStages`, `ePlayerEdit` → `EPlayerEdit`,
  `ePhysType` → `EPhysType`, `eTeamTypes` → `ETeamTypes`, `eTankOffsets` → `ETankOffsets`, `eTankTypes` → `ETankTypes`,
  `playerType` → `EPlayerType`, `playerPrefType` → `EPlayerPrefType`, `itemType` → `EItemType`,
  `weaponType` → `EWeaponType`, `selfDestructVals` → `ESelfDestructVals`, `decorTypes` → `EDecorTypes`,
  `alignType` → `EAlignType`.
- Typedefs and template typename parameters are lowercase short names with a `_t` postfix: `plStage_t` → `plstage_t`,
  `itEntry_t` → `itentry_t`, `opEntry_t` → `opentry_t`, `weEntry_t` → `weentry_t`, `Head_T` → `head_t`, `obj_T` → `obj_t`,
  `tgt_T` → `tgt_t`, `opt_T` → `opt_t`, single `T` → `t_t`. Already conforming, keep: `abool_t`, `aflag_t`, `ai32_t`,
  `vobj_t`, `item_t`, `debpool_t`, `opp_t`, `mutex_t`, `condv_t`, `lguard_t`, `luniq_t`.
- Excluded (external, never rename): `BITMAP`, `dirent`, `DIR`, all Allegro/CRT/system APIs.
- Functions/variables: mechanical snake_case per module in `PF-1.15.2`/`PF-1.15.3` (e.g. `addLandSlide` → `add_land_slide`,
  `errorMultiplier` → `error_multiplier`); Allegro/CRT/system names excluded. No per-identifier table (thousands of names).

#### [ ] PF-1.15.2: Rename core state and entity classes

Apply the agreed mapping to global state and gameplay entities first. Pure renames, no logic changes; verify with a `make
DEBUG=YES` build plus manual in-game validation (new game, buy screen, fired shots). Split into Action Items below: type
renames are applied globally per batch (every reference tree-wide, or the build breaks), member/function snake_case goes
per module.

- [x] **PF-1.15.2.1**: Global type renames, batch 1 (state and bases)

  Rename `GLOBALDATA` → `CGlobalData`, `ENVIRONMENT` → `CEnvironment`, `VIRTUAL_OBJECT` → `CVirtualObject`, and
  `PHYSICAL_OBJECT` → `CPhysicalObject`, updating every reference tree-wide. Pure renames; verify with a build.

- [x] **PF-1.15.2.2**: Global type renames, batch 2 (entities and arsenal)

  Rename `TANK` → `CTank`, `PLAYER` → `CPlayer`, `MISSILE` → `CMissile`, `EXPLOSION` → `CExplosion`, `BEAM` → `CBeam`,
  `DECOR` → `CDecor`, `TELEPORT` → `CTeleport`, `FLOATTEXT` → `CFloatText`, `AICore` → `CAICore`, `WEAPON` → `CWeapon`,
  and `ITEM` → `CItem`, updating every reference tree-wide. Pure renames; verify with a build.

- [ ] **PF-1.15.2.3**: Mechanical global renames (enums, typedefs, small structs)

  Apply the `E`-prefix enum renames, the lowercase `_t` typedef/parameter fixes, and the small-struct renames (`sGradient`,
  `sMessage`, `sSendReceive`, `Point`, `PlayerMini`, `UpdateData`, `sBox`) tree-wide. Pure renames; verify with a build.

- [ ] **PF-1.15.2.4**: Member/function snake_case for `CGlobalData` and `CEnvironment`

  Rename members and methods (e.g. `AI_clock` → `ai_clock`, `newRound` → `new_round`) with per-member review. Pure renames;
  verify with a `make DEBUG=YES` build plus manual in-game validation.

- [ ] **PF-1.15.2.5**: Member/function snake_case for entity classes

  Rename members and methods of `CTank`, `CMissile`, `CExplosion`, `CBeam`, `CDecor`, `CTeleport`, and `CFloatText`,
  including the hierarchy virtuals (e.g. `getClass` → `get_class`), with per-member review. Pure renames; verify with a
  `make DEBUG=YES` build plus manual in-game validation.

- [ ] **PF-1.15.2.6**: Member/function snake_case for `CPlayer` and `CAICore`

  Rename members and methods of the player state and AI core with per-member review. Pure renames; verify with a `make
  DEBUG=YES` build plus manual in-game validation.

#### [ ] PF-1.15.3: Rename UI, services, and remaining modules

Apply the mapping to menus/options, persistence, text, audio, and utility modules with the same rename-only discipline and
verification as `PF-1.15.2`.

#### [ ] PF-1.15.4: Record the convention and verify the whole tree

Ensure the final convention is recorded in `README.md` (`AGENTS.md` already carries it), then run a full build across GNU
targets and DEBUG flavors plus an in-game check to confirm behavior is preserved.

### [ ] PF-1.16: Modernize config parsing, remove `MAX_CONFIG_LINE`

The constant (`src/files.h:6-7`) sizes static char arrays for configuration file loading, C89-style; replace the parsing with
modern C++17 methods (the `sscanf()` lists were already removed in commit `f3c129bd`) and drop the constant and its `@todo`.

#### [ ] PF-1.16.1: Inventory static-buffer uses and design the replacement

Grep for `MAX_CONFIG_LINE` and the static char arrays it sizes across the config/savegame loading code (`src/files.cpp`,
`src/environment.cpp`, `PLAYER::load_from_file` paths) and design the C++17 replacement (`std::string`/`getline`-style
parsing). No code changes yet.

#### [ ] PF-1.16.2: Reimplement parsing and drop the constant

Replace the static-buffer parsing, delete `MAX_CONFIG_LINE` and its `@todo` in `src/files.h`, and verify with a config
save/load round-trip (settings persist across restart; old config files still load or are cleanly rejected).

### [ ] PF-1.17: Migrate weapons/item data to a documented format (late WP)

There is no spec for the positional `text/weapons*.txt` format beyond the parser code (`Load_Weapons_Text()`, `src/files.cpp`).
As a late step of the modernization, switch to a documented format with a clear spec and a simple parser (INI or YAML), document
the spec, and migrate data, translations, shop, and AI selection code.

#### [ ] PF-1.17.1: Decide INI vs YAML with the user

Compare dependency and build impact (hand-rolled INI parser vs a YAML library) and record the decision with rationale. No code
changes yet.

#### [ ] PF-1.17.2: Write the format specification

Write the spec document for weapon/natural/item records (fields, types, sections, localization split between numeric stats and
display strings), so data edits no longer require reading parser code.

#### [ ] PF-1.17.3: Implement the parser and migrate the data

Implement the simple parser, migrate `text/weapons*.txt` data and translation display strings, and adapt shop, AI weapon
selection, and firing code. Verify parsed stats are identical to the legacy parser output and check in-game (buy screen
quantities/prices, fired shots).

### Planned follow-ups (not yet numbered; become `TODO-PF-*` after `TODO-PF-1`)

- Proper development of network functionality (currently a Linux-only first draft; `NETWORK` handling stays as-is during this
  cleanup).
- UI-framework modernization away from Allegro 4 (including removal of the then-obsolete `unicode.dat`; until then the file is
  ignored, not touched).

### Feature-Complete Checklist

- [ ] `git grep "either version 2" -- src` returns zero hits (license headers consolidated, `WP PF-1.1`).
- [ ] `git grep -i "GPL-2.0" -- io.github.EdenWorX.atanks.metainfo.xml` returns zero hits (metadata matches `LICENSE`, `WP
  PF-1.1`).
- [x] `git grep "6.5_rc1" -- vs12 vs14` returns zero hits (MSVC versions consolidated, `WP PF-1.2`).
- [ ] `git ls-files dep/` shows only `.keep_dir` (dependency files untracked, `WP PF-1.4`).
- [ ] `cmake -S . -B <dir> -G Ninja` configures and `cmake --build <dir>` links `atanks` (CMake build works, `WP PF-1.9`).
- [ ] `make test` and `make test-all` run green (unit suite in release and debug dirs, `WP PF-1.11`).
- [ ] `make -n` maps each goal to its `cmake-build-*` directory (`-release`/`-debug`/`-asan`/`-tsan`/`-usan`, `WP PF-1.9`).
- [x] `make install` populates `bin/atanks`, metainfo, desktop file, icons, and data with no stray files (`WP PF-1.12`).
