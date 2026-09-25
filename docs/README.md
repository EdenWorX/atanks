# Atomic Tanks

> Technical developer documentation — for the game itself, see [../README.md](../README.md).

## Overview

Atomic Tanks is a turn-based artillery game in the Scorched Earth / Worms tradition. Each player controls a tank, buys weapons
and defensive items between rounds, and fires projectiles in turn-based order. The last tank standing wins the round. The game
supports human players, AI bots (multiple difficulty levels), teams (Bastion / Rogue / Neutral), destructible terrain, wind and
weather, network play (host plus clients), and localized in-game text.

- Language: C++ (built with `-std=c++17`, see `CMAKE_CXX_STANDARD 17` in `CMakeLists.txt`).
- Graphics/audio/input library: Allegro 4 (`#include <allegro.h>` in `src/main.h`; the CMake build queries
  `allegro-config --cppflags/--libs`). Allegro must be installed separately; it is not vendored in this repository.
- Concurrency: POSIX threads on Linux and BSD (`Threads::Threads` in `CMakeLists.txt`),
  `std::thread`/`std::mutex`/`std::condition_variable` in game code, plus a custom spinlock (`src/spinlock.h`).
- Version: the `VERSION` variable in `Makefile` (currently `6.7.2`, derived from `project(VERSION ...)` in
  `CMakeLists.txt`) is the single source of truth. Older version strings elsewhere were synchronized during the 6.7.1 Cleanup
  and Modernization.
- License: `LICENSE` is the single source of truth. Formerly contradicting license information (old GPLv2 `COPYING` text,
  `either version 2 ... or later` source headers, `io.github.EdenWorX.atanks.metainfo.xml:5` declaring `GPL-2.0-or-later`) was
  consolidated during the 6.7.1 Cleanup and Modernization: headers now say version 3, the metainfo declares
  `GPL-3.0-or-later`, and `COPYING` is a pointer to `LICENSE`.
- Issue reports go to `https://github.com/EdenWorX/atanks/issues`. This is a manual fork moved from SourceForge to GitHub;
  updating the remaining SourceForge references was done during the 6.7.1 Cleanup and Modernization.

## Repository Status and Documentation Scope

This document was generated from tracked files only (`git ls-files`, 496 files). The following were intentionally excluded from
analysis per `.gitignore` and `git ls-files --other`:

- Build outputs: `atanks` binary, `obj/*.o` (except the tracked placeholder `obj/.keep_dir`), `*.dep`.
- Logs: `atanks.log`, `allegro.log`, `memcheck.log`.
- Windows local config: `allegro.cfg` (tracked in git but matched by a `.gitignore` rule; known to disable vertical sync on
  Windows builds as a workaround for Allegro 4 sync problems).
- Screenshots: `screenshot_*.*` (`screenshot_0001..0007.bmp`, `screenshot_001/002.bmp` on disk).
- Agent/IDE-local state: `.opencode/` (including `node_modules`), `.aiignore`, `.idea/workspace.xml`, `talk_*.md`.
- `AGENTS.md` itself is matched by a `.gitignore` rule and is therefore untracked-by-design documentation.

External or bundled third-party material was identified by metadata only and was not deeply analyzed: `src/extern/dirent.{h,c}`
(Kevlin Henney Win32 dirent shim), the Allegro 4 system library, and the tracked Windows runtime DLLs `alleg44.dll` /
`alleg44_64.dll`. Details are in `External or Bundled Dependencies` below.

## Project Layout

Top-level tracked entries (`git ls-files`, directories sorted):

| Path | Contents / role |
|---|---|
| `src/` | All game source: ~50 `.cpp` + ~55 `.h` files, plus `src/extern/` shim |
| `src/extern/` | Bundled `dirent` shim for Windows (`dirent.h`, `dirent.c`) |
| `button/`, `misc/`, `missile/`, `stock/`, `tank/`, `tankgun/`, `title/` | Runtime bitmap assets (`*.bmp`), installed as data |
| `sound/` | Runtime sound assets (`*.wav`) |
| `text/` | Arsenal data (`weapons.toml`, `naturals.toml`, `items.toml`, `weapons_*.toml` translations) and localized in-game text files (`Help*.txt`, `ingame*.txt`, etc.) |
| `unicode.dat` | Allegro datafile used for fonts; also the probe file for data-dir detection. An old manual addition; ignored (not touched) until the post-cleanup move away from Allegro 4 makes it obsolete |
| `Makefile` | Primary GNU Make build (`VERSION 6.7.2`, the version single source of truth) |
| `vs12/`, `vs14/` | Legacy Visual Studio 2013 / 2015 solutions (retired toolsets v120/v140); Windows builds go through CMake |
| `dep/` | Ignored GCC dependency files (`*.d`, legacy make outputs) plus `.keep_dir` placeholder |
| `obj/` | Object output directory; only `.keep_dir` is tracked |
| `README.md` | Welcoming front page (user documentation) |
| `docs/Changelog.history` | Frozen release history up to 6.7 (renamed from `Changelog`; new releases go in `CHANGELOG.md`) |
| `docs/TODO.md` | Canonical planning file (per `docs/todo_planning.md`); `TODO-PF-1` Cleanup and Modernization was completed as 6.7.1 |
| `docs/` | EdenWorX planning and release rules (`todo_planning.md`, `release_process.md`) |
| `COPYING`, `LICENSE` | License pointer + full license text (`LICENSE` is the single source of truth) |
| `credits.txt` | Authors, graphics, docs, translations, sound attributions |
| `atanks.desktop` | freedesktop menu entry (`Exec=atanks`) |
| `io.github.EdenWorX.atanks.metainfo.xml` | AppStream metadata |
| `atanks.ico`, `atanks.png` | Windows icon (used by `src/atanks.rc`) and Linux menu icon |
| `alleg44.dll`, `alleg44_64.dll` | Tracked Windows Allegro runtime DLLs (32/64-bit) |
| `allegro.supp` | Valgrind suppression file for Allegro/ALSA/X11 noise |
| `do_memcheck.sh`, `do_helgrind.sh`, `gdb_memcheck.sh` | Valgrind/helgrind/gdb helpers that run `./atanks` |
| `tests/` | CppUTest unit suite for decoupled logic units, wired to `ctest` via `make test` |
| `.clang-format` | clang-format style definition (requires clang-format 19+) |
| `.gitignore`, `.idea/*`, `cb/*` | Ignore rules and IDE metadata |

## Components

### Executables

One program is built: `atanks` (`atanks.exe` on `WIN32`).

- Entry point: `int32_t main(int32_t argc, char** argv)` in `src/atanks.cpp`, closed by Allegro's `END_OF_MAIN()`.
- Startup sequence in `main`:
  1. `parse_args()`.
  2. `env.find_data_dir()`; failure exits with `EXIT_FAILURE`.
  3. `game_version` derived from `VERSION`.
  4. `env.find_config_dir()`, then `load_config()` or `create_config()`.
  5. `env.load_game_files()`; failure exits.
  6. Optional `NETWORK` threads (`Send_And_Receive`, update checker).
  7. Main-menu loop dispatching on `global.get_command()`: help, options, players, credits, network game, demo,
     or local play.
  8. `save_game_settings()`, `env.destroy()`, `global.destroy()`, `allegro_exit()`.
- Round execution funnels into `game()` declared in `src/gameloop.h` via `play_local()`, `play_demo()`,
  and `play_networked()` (without `-DNETWORK` this path reports an error).

### Libraries

There are no separate built libraries. Every `src/*.cpp` compiles and links into the single `atanks` binary
(`add_executable(atanks ...)` in `CMakeLists.txt`). The closest thing to internal libraries is a set of service modules with
narrow responsibilities:

| Module | Files | Responsibility |
|---|---|---|
| Debug/platform layer | `src/debug.h`, `src/debug.cpp` | OS detection (`ATANKS_IS_WINDOWS/LINUX/BSD/MSVC`), `snprintf`/`localtime` shims, `DEBUG_LOG*` macros |
| Global state | `src/globals.h`, `src/externs.h`, `src/globaldata.h/.cpp`, `src/environment.h/.cpp`, `src/globaltypes.h/.cpp` | The two globals (`CGlobalData global`, `CEnvironment env`), enums, constants |
| Persistence | `src/files.h`, `src/files.cpp` | Config, savegames, weapon-text loading, directory scans |
| Text/localization | `src/text.h`, `src/text.cpp` | `TEXTBLOCK` file loading, line selection, text rendering helpers |
| Audio | `src/sound.h`, `src/sound.cpp` | Thin wrappers over `env.sounds` / `env.background_music` |
| Clock | `src/clock.h`, `src/clock.cpp`, `src/winclock.h` | Game/menu timers, MSVC clock workaround |
| Locking | `src/spinlock.h`, `src/spinlock.cpp` | `CSpinLock` (atomic-flag, non-recursive) |
| Z-buffer | `src/zbuffer.h`, `src/zbuffer.cpp` | 1-bit-per-pixel `ZBuffer::set/test` over `vector<bool>` |
| Update protocol | `src/update.h`, `src/update.cpp` | `Update_Data` / update-checker structures |
| Network transport | `src/network.h`, `src/network.cpp` | `sMessage`/`CMessageQueue`, sockets (only when `NETWORK` is defined) |
| Network client | `src/client.h`, `src/client.cpp` | Client-side protocol constants and handling |

### Shared/Internal Utility Code

- `src/main.h` is the common include hub: it requires `debug.h` first, then Allegro headers,
  then `globaltypes.h` and `wrap_dirent.h`, then the C++ standard headers. It defines `BUFFER_SIZE 256`, `HOME_DIR`
  (`"AppData"` on Windows, `"HOME"` on Linux), the `DATA_DIR` fallback `"."`, math helpers (`SIGN`,
  `SIGNd`, `ROUND`, `ROUNDu`, `FABSDISTANCE2`), sleep helpers (`USLEEP`, `MSLEEP`, `LINUX_SLEEP`, `LINUX_REST`),
  and MSVC portability shims.
- `src/externs.h` re-exports the two globals as `extern` for every translation unit except `src/atanks.cpp` (guarded by
  `ATANKS_ATANKS_CPP`). It also declares shared scalars and the three content catalogs.
- Content catalogs, defined in `src/files.cpp`: `CWeapon weapon[WEAPONS]`, `CWeapon naturals[NATURALS]`, `CItem item[ITEMS]`,
  with sizes `WEAPONS 56`, `NATURALS 6`, `ITEMS 24` (see `src/main.h`).
- `src/bitmap.h` is a forwarder declaring `struct BITMAP; struct sGradient;`.
- `src/gfxData.h` (`sGfxData`) owns generated sGradient strips and explosion graphics.
- `src/random.h` / `src/perlin.cpp` provide random numbers (`CHANGELOG.md` 6.7 entry notes thread-local modernized RNG) and
  noise for terrain/sky.
- `src/box.h`, `src/button.h`, `src/menu.h`, `src/optiontypes.h`, `src/optioncontent.h`, `src/optionitem*.h`,
  `src/optionscreens.h` form the menu/options UI framework (self-managing `CMenu` of `TOptionItem` entries).

### Gameplay Modules

| Area | Key files | Notes |
|---|---|---|
| Object hierarchy root | `src/virtobj.h/.cpp` (`CVirtualObject`) | List node (`prev`/`next`), position, dirty-rect updates, virtual `applyPhysics/draw/initialise`, pure `get_class()` |
| Physics mixin | `src/physobj.h/.cpp` (`CPhysicalObject : CVirtualObject`) | Gravity/drag/mass, bounces, `weapType`, angle macros |
| Tank avatar | `src/tank.h/.cpp` (`CTank final : CPhysicalObject`) | Aim/power/selection, health/shield, `move_tank`, `add_damage/apply_damage/explode/repair`, `CSpinLock damage_lock` |
| Projectile | `src/missile.h/.cpp` (`CMissile final : CPhysicalObject`) | `EMissileType{MT_WEAPON,MT_ITEM,MT_NATURAL,MT_MIND_SHOT}`, SDI/cluster/roller handling |
| Detonation | `src/explosion.h/.cpp` (`CExplosion final : CPhysicalObject`) | Terrain deformation, throwing, damage, napalm/debris |
| Beams (lasers) | `src/beam.h/.cpp` | Laser-class weapons parallel to ballistic missiles |
| Arsenal data | `src/weapon.h/.cpp`, `src/item.h/.cpp` | Plain records: 56 weapons + 6 naturals + 24 items; unified index `THINGS = WEAPONS+ITEMS` |
| Player state | `src/player.h/.cpp` | Economy, inventories `nm[WEAPONS]/ni[ITEMS]`, personality, opponent memory, shop prefs, save/load, speech-line selection |
| Player/AI types | `src/player_types.h/.cpp` | `EPlayerType` (HUMAN..DEADLY..NETWORK_CLIENT..), `EPlayerStages`, `ETeamTypes{ROGUE,NEUTRAL,BASTION}`, modular enum arithmetic |
| AI | `src/aicore.h/.cpp` (`CAICore`) | Background-thread bot with documented pipeline: initialize, target/weapon selection, attack calculation, aiming traces, writeback |
| Shop | `src/shop.h/.cpp` (`bool shop(CLevelCreator*)`) | Inter-round buy/sell UI |
| Scoring | `src/score.h/.cpp` (`sScore`, `sort_scores()`) | Caller deletes the returned array |
| Terrain/sky | `src/land.h/.cpp`, `src/sky.h/.cpp`, `src/levelcreator.h/.cpp`, `src/moon.h/.cpp`, `src/satellite.h/.cpp`, `src/teleport.h/.cpp`, `src/decor.h/.cpp`, `src/debris_pool.h/.cpp`, `src/floattext.h/.cpp` | 16 land + 16 sky gradients each (8 classic + 8 crispy), generators, decor, debris, floating text |
| Round driver | `src/gameloop.h/.cpp` (`game()`) | Round phases, AI thread, per-class `ObjectUpdater` threads, input, firing, winner detection |

### Data, Templates, and Resources

Installed by the `install` rules in `CMakeLists.txt` (mirroring the old `make install` layout) under `${INSTALLDIR}`:

- `button/*.bmp` (28 files, `0..27`), `misc/*.bmp` (18 files), `missile/*.bmp` (32 files, `0..31`), `stock/*.bmp` (80 files,
  `0..79`), `tank/*.bmp` (17 files), `tankgun/*.bmp` (10 files), `title/*.bmp` (4 files).
- `sound/*.wav` (27 files: `00-07`, `10-22`, `30-32`, `40`).
- `text/*.txt` (~75 files): per-topic per-language matrix for `gloat`, `ingame`, `instr`, `panic`, `kamikaze`, `retaliation`,
  `revenge`, `suicide`, `war_quotes`, `Help`, with language suffixes `_de`, `_fr`, `_it`, `_ru`, `_sk`, `_ES`,
  `.pt_BR` plus the English base file.
- `unicode.dat` (5604 bytes, `file` reports `Allegro datafile`), `COPYING`, `README.md`, `Changelog.history`, `*.txt`.
- `atanks.png` is installed to `.../share/icons/hicolor/48x48/apps`; `atanks.ico` is consumed by `src/atanks.rc` for the
  Windows build.

Weapon/item stats come from `text/weapons.toml`, `text/naturals.toml`, `text/items.toml` (see `Configuration` and
`docs/weapons_toml_spec.md`). Speech/quote text comes from the other `text/*.txt` files
via `TEXTBLOCK`.

### Platform-Specific Code

- `src/debug.h` detects `ATANKS_IS_WINDOWS`, `ATANKS_IS_MSVC` (including the `ATANKS_HAS_MSVC12_BUG` workaround for
  `_MSC_VER < 1900`), `ATANKS_IS_BSD`, and `ATANKS_IS_LINUX`; anything else is a compile `#error`.
- `src/main.h` handles `ALLEGRO_NO_MAGIC_MAIN`, `ALLEGRO_HAVE_STDINT_H`, and `winalleg.h` inclusion on Windows;
  handles MSVC `PATH_MAX`, `_USE_MATH_DEFINES`, and POSIX-vs-MSVC headers.
- `src/winclock.h` plus `USLEEP`/`MSLEEP` in `src/main.h` work around an MSVC12 chrono problem.
- `src/wrap_dirent.h` includes `extern/dirent.h` on MSVC and the system `<dirent.h>` elsewhere. `src/files.cpp` has
  separate `scandir` paths for Win32 vs POSIX.
- `src/atanks.rc` (Windows resources: `A ICON ../atanks.ico`, Allegro icon, `VERSIONINFO`) and `src/resource.h` (MSVC-generated
  defines) are used only by the Visual Studio builds.

### External or Bundled Dependencies

The following were classified as external by metadata inspection; their internals were not analyzed:

- **Allegro 4 (system dependency).** Evidence: `README.md` requires the Allegro (development) package; `Makefile`
  queries `allegro-config` for Linux/macOS builds; `vs12|vs14/README_allegro.txt` explain how to repoint
  include/lib paths to a local Allegro; `io.github.EdenWorX.atanks.metainfo.xml` states the game "runs on any platform
  Allegro4 runs on". Role: graphics, sound, input,
 timers. The build fails without it.
- **`src/extern/dirent.{h,c}` (bundled shim).** Evidence: header comment `Declaration of POSIX directory browsing functions and
  types for Win32. Author: Kevlin Henney ... Created March 1997. Updated June 2003` (`src/extern/dirent.h`) with a
  permissive use/copy/modify/distribute grant. Role: POSIX `opendir/readdir` for MSVC builds only, selected by
  `src/wrap_dirent.h` and compiled in the `.vcxproj` files; the GNU build uses the system `<dirent.h>`.
- **Windows Allegro runtime DLLs.** `alleg44.dll` (832512 bytes) and `alleg44_64.dll` (998400 bytes) are tracked release
  runtimes for 32/64-bit Windows (per `vs12|vs14/README_allegro.txt`, only Release DLLs are kept in git; Debug/import libraries
  are user-supplied and matched by `.gitignore` rules `alleg44*.lib`, `alleg44*-debug.*`, `alleg44*_d.*`).
- **Build/analysis tools (not vendored):** `clang++`, `cmake` 3.25+, `ninja`, Visual Studio 2026 (C++17 + CMake support),
  and `valgrind` for the `do_*.sh` helpers.

## Architecture

```
 CLI (atanks.cpp: parse_args)
   |
   v
 CEnvironment env ......... fixed config + asset registry (environment.h)
 CGlobalData global ........ per-round mutable state (globaldata.h)
   |                               |
   +-- main-menu loop .............+-- round driver game() (gameloop.cpp)
   |    (menu/options/players/       |    |
   |     shop/select_players)         |    +-- CAICore thread (aicore.h) per AI tank
   |                                 |    +-- ObjectUpdater threads per EClass
   |                                 |    +-- object lists: CTank / CMissile / CBeam /
   |                                 |         CExplosion / CTeleport / CDecor / CFloatText
   v                                 v
 files.cpp ................. config/save/weapon-text loading
 text.cpp .................. TEXTBLOCK localization
 sound.cpp ................. audio triggers
 network.cpp / client.cpp .. host/client transport (NETWORK builds)
```

- Draw/update ordering follows `EClass` in `src/globaltypes.h` (`CLASS_MISSILE, CBeam, CTank, CTeleport, DECOR_DIRT, SMOKE,
  CExplosion, CFloatText, COUNT`).
- Round stages follow `ERoundStages` (`STAGE_AIM, STAGE_FIRE, STAGE_SCOREBOARD, STAGE_ENDGAME`).
- Threading: `CAICore` runs bot planning off the main thread (`mutex`/`condition_variable`, `start/stop/status` in
  `src/aicore.h`); `gameloop.cpp` spawns one `ObjectUpdater` thread per class behind `updMutex/updCondition`,
  synchronizes with them once per frame in `update_objects()`, and finishes and joins them when the round ends.
  `SANITIZE_THREAD=YES` builds define `USE_MUTEX_INSTEAD_OF_SPINLOCK` (thread-sanitizer logic in
  `CMakeLists.txt`).
- Data flow for content: `text/weapons.toml` + `naturals.toml` + `items.toml` -> `load_weapons_text()` (`src/files.cpp`) -> `weapon[]/naturals[]/item[]` globals
  -> shop UI, AI planning, firing, explosions. `text/*.txt` (speech/help) -> `CEnvironment::load_text_files()`
  (`src/environment.cpp`) -> `TEXTBLOCK*` fields -> menus, AI taunts, help screens.

## Build System

### Supported Build Paths

| Goal / command | Platform / result | Notes |
|---|---|---|
| `make` then `make install` | Linux system install | Thin cmake+ninja wrapper: configures `./cmake-build-release`, installs the binary to `$(PREFIX)/bin` (`/usr/bin` default) and data to `/usr/share/atanks`; honors `PREFIX=` and `DESTDIR=` |
| `make user` | Linux local run | Binary at `./cmake-build-release/atanks`, `DATA_DIR="."` (run from the project root) |
| `make osxuser` (`gmake osxuser`) | macOS local run | Via the wrapper into `./cmake-build-release` (CMake detects macOS automatically) |
| `make bsduser` | BSD local run | Via the wrapper into `./cmake-build-release` (CMake detects BSD automatically) |
| `make debug` / `aidebug` / `fulldebug` | Debug variants | Via the wrapper into `./cmake-build-debug`; `aidebug` adds AICORE logging, `fulldebug` adds finance/objects/physics |
| `make clean` / `veryclean` | Cleanup | Removes `cmake-build*` directories (plus legacy `obj/*` and `atanks` leftovers) |
| `make dist` / `source-dist` / `i686-dist` / `tarball` / `zipfile` | Distribution archives | Legacy targets kept as-is (`win32-dist` was retired with the MinGW path); `DISTCOMMON` still references legacy `atanks/*` paths that no longer exist |
| Direct CMake | Any Unix | `cmake -S . -B <dir> -G Ninja` (3.25+, ninja mandatory) then `cmake --build <dir>`; see `CMakeLists.txt` for options |
| `vs12/atanks.sln`, `vs14/atanks.sln` | Legacy Visual Studio 2013 / 2015 solutions | Retired toolsets; Windows builds go through CMake (VS2026); see below for file details |

Verified on this machine: `make user` configures `./cmake-build-release`, builds all 51 steps, and the binary reports
`Atomic Tanks Version 6.7.2`.

### Autotools / Make Build

There is no Autotools setup (`configure`, `configure.in`, `aclocal.m4` do not exist). The `Makefile` is a thin cmake+ninja
wrapper (introduced during the 6.7.1 cleanup); the real build lives in `CMakeLists.txt` (CMake 3.25+, Ninja mandatory):

- Sources: `file(GLOB ... src/*.cpp)` with `CONFIGURE_DEPENDS`, so new files are picked up automatically.
- Version single source of truth: `project(atanks VERSION 6.7.2 ...)`; `src/config.h.in` generates `config.h` with the
  version macros
  (`VERSION`, `DATA_DIR`, `NETWORK`, platform flags) consumed via `src/main.h`.
- Options mirror the old make knobs: `DEBUG` plus `DEBUG_AICORE/AIMING/EMOTION/FINANCE/OBJECTS/PHYSICS/LOG_TO_FILE`,
  `SANITIZE_ADDRESS/THREAD/UNDEF` (address beats thread; undefined combines; any sanitizer implies debug), `USE_LTO`,
  `GCCUSESGOLD`, `ATANKS_DATA_DIR`, install directory settings.
- Base flags: `-Wall -Wextra -Wpedantic -std=c++17 -fexceptions`, C++17 required.
- Per-platform: Linux and BSD define `NETWORK` and link `allegro-config` flags, `-pthread`, `-lm -lpthread` (BSD also gets
  `-I/usr/local/include`, `-Wno-c99-extensions`); macOS adds `-I/usr/local/include` and Allegro flags (no `-DNETWORK`). There
  is no Windows support in CMake; Windows builds use `vs12/`/`vs14/`.
- Release: `-O2`, `-march=native`, optional `USE_LTO=YES` (`-flto`, optionally `-fuse-linker-plugin`).
- Debug: `-ggdb`, `-Og`, `-DATANKS_DEBUG`, `-fstack-protector-strong`, `-Wunused`, LTO blocked; sanitizer and per-flavor
  `-DATANKS_DEBUG_*` defines as above, with `DEBUG_AICORE` enabling aiming+emotions together.
- Install rules mirror the old `make install` layout (binary, metainfo, desktop file, icons, data tree); `DESTDIR` staging works
  natively via `cmake --install`.
- The wrapper owns the build directory: base `./cmake-build` plus option postfixes (`-release`, `-debug`, `-asan`, `-tsan`,
  `-usan` suffix) and always reconfigures, so flag changes cannot go stale. Direct CMake use: `cmake -S . -B <dir> -G Ninja`
  with the same `-D` options, then `cmake --build <dir>`.

### Platform-Specific Builds

- BSD builds use the GNU `Makefile` (`make bsduser`); there is no separate BSD makefile.
- Legacy `vs12` / `vs14` solutions target retired toolsets (VS2013: `Format 12.00`, toolset v120; VS2015: toolset v140,
  `WindowsTargetPlatformVersion=8.1`). Both define `VERSION="6.7.2"` (matching `CMakeLists.txt`); `vs14` additionally defines
  `DATA_DIR="."`. Both link one of `alleg44.lib / alleg44_64.lib / alleg44_d.lib / alleg44_64_d.lib` per configuration plus the
  Win32 system libraries. `vs14` embeds `../atanks.ico`. `README_allegro.txt` in each folder explains how to repoint
  include/library paths and swap the DLL variants. Current Windows builds go through CMake (VS2026), not these solutions.
- Windows builds also pick up the tracked `allegro.cfg`, which disables vertical sync as a workaround for Allegro 4 sync
  problems there. The file is matched by `.gitignore` as Windows-local config but is kept in git deliberately — preserve it, do
  not "clean it up". Broader UI-framework modernization away from Allegro 4 is deferred to a future task.

### Visual Studio Project Files

Covered above; the `.vcxproj.filters` files only group files in Solution Explorer (German group names in `vs12`, English in
`vs14`) and carry no build semantics.

### Xcode Workspace / Configuration Files

None exist in the repository.

## Configuration

- Compile-time: `DATA_DIR` (`ATANKS_DATA_DIR` setting, default `<prefix>/share/atanks`, `"."` for `*user` goals), `VERSION`
  (`project(VERSION 6.7.2)`, via generated `config.h`), platform flags (`LINUX` / `MACOSX` in `config.h`), `NETWORK` (Linux and
  BSD only), `ATANKS_DEBUG*` flavors.
- Runtime data directory, resolved by `CEnvironment::find_data_dir()` (`src/environment.cpp`): `--datadir` if readable,
  else the compiled `DATA_DIR` (verified by probing `unicode.dat` inside it), else `./` fallback.
- Runtime config directory, resolved by `CEnvironment::find_config_dir()` (`src/environment.cpp`): `-c <path>` if given,
  else `$HOME/.atanks` (`HOME_DIR` = `HOME` on Linux, `AppData` on Windows, see `src/main.h`). `Copy_Config_File()`
  (`src/files.cpp`) migrates a legacy `$HOME/.atanks-config.txt` into the directory.
- Main settings file: `<config_dir>/atanks-config.txt`, loaded by `load_config()` (`src/atanks.cpp`, via
  `env.load_from_file()` plus per-player `CPlayer::load_from_file`) and written by `save_game_settings()`.
  `--noconfig` skips loading.
- Update checker: at startup a background thread fetches the latest release tag from the GitHub releases API
  (`https://api.github.com/repos/EdenWorX/atanks/releases/latest`, needs libcurl at build time) and shows a message
  in the menu when a newer version exists. Controlled by the `CHECKUPDATES` setting (`env.check_for_updates`,
  Options menu); failures (offline, no network) stay silent.
- Weapon/item stats: `load_weapons_text()` (`src/files.cpp`, declared in `src/files.h`) reads `<data_dir>/text/weapons.toml`,
  `naturals.toml`, `items.toml`, plus the `<data_dir>/text/weapons_*.toml` translation matching `env.language`
  (`weapons_fr.toml`, `weapons_de.toml`, `weapons_it.toml`, `weapons.pt_BR.toml`, `weapons_ru.toml`, `weapons_sk.toml`,
  `weapons_ES.toml`). English is
  always loaded first for numeric stats; a second pass overwrites only `name`/`desc` for localization. See
  `docs/weapons_toml_spec.md` for the record format.
- Speech/help text: `CEnvironment::load_text_files()` (`src/environment.cpp`) loads `text/<base><suffix>` for `gloat`,
  `ingame`, `instr`, `panic`, `kamikaze`, `retaliation`, `revenge`, `suicide` (suffixes `.txt`, `_fr`, `_de`, `_it`, `.pt_BR`,
  `_ru`, `_sk`, `_ES`) plus `war_quotes[_it|_ru|_ES].txt`, into `TEXTBLOCK*` fields (see `src/environment.h`).
- Savegames: `<config_dir>/<game_name>.sav`, format `VERSION/GLOBAL/CEnvironment/PLAYERS/***EOF***` (see `src/files.cpp`);
  listing via `find_saved_games()` (`*.sav` filter, see `src/files.cpp`).
- Music: `Create_Music_Folder()` ensures a `music/` folder in the config dir (see `src/files.cpp`); custom `*.bmp` files are
  picked up by `Find_Bitmaps()`.

## Runtime Behavior

- Default invocation `./cmake-build-release/atanks` (run from the project root) equals `--windowed --width 800 --tall 600
  --datadir . depth 32` (per the `README` user documentation; defaults `DEFAULT_SCREEN_WIDTH 800` / `DEFAULT_SCREEN_HEIGHT 600` in
  `src/globaltypes.h`).
- First run with no human player opens the player-creation screen automatically; afterwards the flow is Players -> select tanks
  (2-10) -> buy screen (left-click buys, right-click sells, `Done` confirms) -> battle (see the `README` user documentation).
- In-battle keys (see the `README` user documentation): Space fires/selects, Enter confirms, Up/Down power and menu cycling, Left/Right gun aim and
  buy/sell, Esc cancels, F1 screenshot, F10 AI-takeover (or save on the buy screen), `v`/`V` volume down/up, `~` (or `#` on
  German keyboards) scoreboard.
- Network play (still rough, see the `README` user documentation): the host enables Networking in Options -> Network and restarts; clients set
  Server Address to the host IP and choose Network Game. Client tanks are color-coded (Bastion blue, Rogue red, Neutral green,
  player purple). The `docs/TODO_Xtra.md` file records a bug: the network client must not get unlimited shots.
- Screenshot key F1 writes `screenshot_*.*` files (a `.gitignore`d artifact).
- Environment variables: `HOME` (Linux) or `AppData` (Windows) locates the config directory.

## Command-Line Tools

Main binary flags, parsed by `parse_args()` (help text printed by the binary itself):

| Flag | Effect |
|---|---|
| `-h`, `--help` | Print help, exit with `HELP_REQUESTED (-100)` |
| `-fs` | Full screen |
| `--windowed` | Windowed mode |
| `-d`, `--depth <16\|32>` | Color depth into `env.colourDepth` |
| `-w`, `--width <>=512>` | Screen width (also half/temp variants) |
| `-t`, `--tall` (`--height`) `<>=320>` | Screen height (also half/temp variants) |
| `--datadir <path>` | Data directory into `env.data_dir` (must be readable) |
| `-c <path>` | Config/save directory into `env.config_dir` |
| `--noconfig` | Do not load game settings |
| `--nosound` | Disable sound (`env.sound_enabled=false`) |
| `--noname` | Hide player names above tanks |
| `--nonetwork` | Disable networking (`allow_network=false`) |
| `--nobackground` | Hide the green menu background |
| `--nothread`, `--thread` | Accepted but ignored (deprecated) |

Missing option values print `ERROR: Missing argument` and exit `EXIT_FAILURE`.

Standalone helpers (not built by `Makefile`):

- `do_memcheck.sh`: `valgrind --tool=memcheck --trace-children=yes --track-origins=yes --leak-check=full ... ./atanks`.
- `do_helgrind.sh`: `valgrind --tool=helgrind ... ./atanks`.
- `gdb_memcheck.sh`: memcheck under `gdb` (`--vgdb=full --vgdb-error=0`).
- `allegro.supp`: suppressions for `ld.so`/`dl_*`, `install_sound` mempool, ALSA (`snd_pcm_open`, `snd_config_*`), `_al_malloc`,
  and X11/Xrm realloc noise.

## Data Flow

1. Startup resolves `data_dir` and `config_dir`, loads `atanks-config.txt`, players, weapon stats, text blocks, bitmaps, fonts,
   sounds, and background music (`CEnvironment::load_game_files()` in `src/environment.cpp`).
2. CMenu/options/player/shop screens mutate `CEnvironment` (options, rosters) and `CPlayer` objects (names, colors, teams,
   inventories, money).
3. `game()` (in `src/gameloop.cpp`) runs a round: `init_new_round()`, `set_tank_settings()`, spawn of the
   `CAICore` thread and per-class `ObjectUpdater` threads, then the frame loop over stages `STAGE_AIM -> STAGE_FIRE
   -> STAGE_SCOREBOARD -> STAGE_ENDGAME`.
4. Firing creates `CMissile`/`CBeam` objects; impacts create `CExplosion`s, which deform `global.surface`/`global.terrain`, throw
   tanks, apply damage (with `CTank::damage_lock`), spawn debris/floattext, and may trigger AI revenge/panic logic via opponent
   memory.
5. Round end credits winners (`CEnvironment::creditWinners`), sorts scores (`sort_scores()`), opens the shop for the next round,
   and persists settings and optional savegames to the config directory.

## Error Handling and Logging

- Fatal startup failures (missing data dir, unreadable game files) print an error and return `EXIT_FAILURE` from `main`
  (see `src/atanks.cpp`).
- `parse_args()` rejects missing values with `ERROR: Missing argument` (see `src/atanks.cpp`).
- `DEBUG_LOG(...)` and its flavors (`DEBUG_LOG_AIM/EMO/AI/FIN/OBJ/PHY`, see `src/debug.h`) compile to no-ops unless the
  matching `ATANKS_DEBUG*` macro is defined; with `ATANKS_DEBUG` they call `debug_log()` with `file:line|function()` position
  info (`ATANKS_GET_POS`).
- `debug_log()` (see `src/debug.cpp`) writes to `atanks.log` on Windows or when `ATANKS_DEBUG_LOGTOFILE` is set, otherwise to
  `stdout`; output is mutex-protected.
- MSVC-incompatible POSIX calls are shimmed in `src/main.h` (`snprintf`, `strncpy`, `access`, `strcasecmp`, `strdup`,
  `unlink`, `mkdir`).
- In-game failures are surfaced modally where practical (e.g. the non-`NETWORK` network-game path reports an error instead of
  crashing, see `src/atanks.cpp`).

## Testing and Validation

- Automated unit suite exists: `tests/` (CppUTest, decoupled logic units) wired to `ctest`.
  - `make test` builds and runs the suite in the flag-selected build directory; `make test-all` runs it in release and debug.
  - `make test-asan` / `test-ubsan` / `test-tsan` run the suite under sanitizers.
- The closest equivalents to tests are:
  - `make -n <target>` dry-run to validate flag expansion (verified here for `make -n user`).
  - A full `make user` build followed by a `./cmake-build-release/atanks --windowed` smoke run.
  - `make debug` / `aidebug` / `fulldebug` builds plus the Valgrind helpers (`do_memcheck.sh`, `do_helgrind.sh`,
    `gdb_memcheck.sh` with `allegro.supp`) for memory/thread validation.
  - `CHANGELOG.md` entries as regression notes (e.g. 6.7 lists fixed crashes, AI, and land-creation bugs).
  - Static-analysis and doc targets exist (`tools/run-cppcheck.sh`, `make doc`); public APIs carry full Doxygen coverage
    since 6.7.1.
- Accepted validation bar (the game is an interactive GUI application): a green `make test` (plus `make test-all` where
  affordable), plus manual validation — developers actually test their changes in-game.
- Known-issue sources: `TODO_Xtra.md` only — its `Important Issues`, `General Issues`, and `Planned Features` sections
  (including the ideas moved over from the removed legacy `TODO` file, still awaiting real to-do item numbers).
- Bug reports go to `https://github.com/EdenWorX/atanks/issues` (this fork moved from SourceForge to GitHub during the 6.7.1
  Cleanup and Modernization; the few remaining SourceForge references in `README.md`, `credits.txt`, the metainfo file, and help
  texts are historical and kept deliberately).

## Development Workflow

- Configure: install Allegro 4 development files so `allegro-config` works; for a local build nothing else is required.
- Build (Linux): `make user` (binary at `./cmake-build-release/atanks`, data in place). For a system install: `make` then
  `make install` (optionally `PREFIX=/usr/local`, `DESTDIR=<staging>`).
- Build (Windows): open the project folder in Visual Studio 2026 (built-in CMake support) after following
  `README_allegro.txt` to repoint Allegro include/lib paths.
- Build (macOS): `make osxuser` (or `gmake osxuser`).
- Build (BSD): `make bsduser` with GNU make.
- Debug: `make debug` (general), `make aidebug` (AI aiming/emotions to `atanks.log`), `make fulldebug` (all flavors, into
  `./cmake-build-debug`); or set `DEBUG=YES` with `DEBUG_AICORE/AIMING/EMOTION/FINANCE/OBJECTS/PHYSICS/LOG_TO_FILE=YES` and
  optionally `SANITIZE_ADDRESS/THREAD/UNDEF=YES` directly.
- Run: `./cmake-build-release/atanks --windowed` for a window, `./cmake-build-release/atanks -h` for options.
- Validate memory/threads: `./do_memcheck.sh`, `./do_helgrind.sh`, `./gdb_memcheck.sh`.
- Static analysis: `bash tools/run-cppcheck.sh` (triaged list in its header), and `clang-tidy -p cmake-build-release
  src/**/*.cpp` (checks from `.clang-tidy`; ensure `compile_commands.json` exists by configuring first).
- Clean: `make clean` (removes `cmake-build*` directories plus legacy `obj/*` and `atanks` leftovers).
- Format: `.clang-format` (clang-format 19 or later) is the defined style; the 6.7 `CHANGELOG.md` entry states the tree
  was uniformly formatted with it.
- Documentation files (`AGENTS.md`, `docs/*`) wrap prose at a maximum line length of 128 characters

## Coding Conventions

- Language standard: C++17 (`CMAKE_CXX_STANDARD 17`); no C++20+ features. Style is defined by `.clang-format`
  (clang-format 19 or later): 128-column limit, attached braces, one parameter per line in declarations, left-aligned
  pointers, sorted case-sensitive includes.
- Naming: variables and functions in snake_case (`var_name`, `func_name()`), classes and structs in PascalCase (`MyClass`),
  templates in PascalCase with `T` prefix (`TContainer`), constants and macros in UPPER_SNAKE (`MAX_SIZE`); prefix `C` for
  important classes (`CPlayer`, `CTank`), `T` for templates (`TOptionItem`), `s` for widely-used structs (`sScore`).
  Enumerations use the `E` prefix (`EClass`, `EPlayerType`) with UPPER_SNAKE enumerators; typedefs are lowercase short names
  with a `_t` postfix (`plstage_t`, `head_t`). External names (Allegro, CRT, system APIs) are never renamed.
- Headers use guards of the form `ATANKS_<NAME>_H_INCLUDED`; `src/globals.h` may only be included from `src/atanks.cpp`
  (other units use `src/externs.h`). New `src/*.cpp` files are picked up by CMake automatically but must be added to the
  legacy `vs12`/`vs14` projects by hand.
- Ownership is manual: mirror every allocation with a deallocation in the same module; `CSpinLock` is non-recursive.
- Log via the `DEBUG_LOG*` macros (they compile away in release builds); never `printf`-debug gameplay code.

## Adding or Modifying Code

- New gameplay entity: subclass `CVirtualObject` (or `CPhysicalObject` for ballistic behavior) in a new `src/<name>.h/.cpp`
  pair, return the matching `EClass` from `get_class()`, add the files to the VS projects' file lists (CMake picks up
  `src/*.cpp` via `file(GLOB ...)` automatically), and wire creation/update/draw into `gameloop.cpp` and teardown into
  `CGlobalData::destroy` paths.
- New weapon or item: append a record to `text/weapons.toml`, `text/naturals.toml`, or `text/items.toml` (and its
  translations for display
  strings) with all keys per `docs/weapons_toml_spec.md`, keeping the array counts (`WEAPONS`/`NATURALS`/`ITEMS`)
  exact; check AI selection (`CAICore`), shop availability
  (`CEnvironment::gen_items_list`), and
  sound/pic mappings. Note: the record layout is specified in `docs/weapons_toml_spec.md`, so data edits no longer
  require reading parser code.
- New option/menu entry: add the `EMenuClass`/`EEntryType` value in `src/optiontypes.h`, construct the item in
  `menu.cpp`/`optionscreens.cpp`, and persist it in `CEnvironment::save_to_file/load_from_file`.
- New language: copy the `text/*.txt` matrix with the new suffix, add a `text/weapons_<suffix>.toml` translation
  (name/desc only, see `docs/weapons_toml_spec.md`), extend the suffix lists in `CEnvironment::load_text_files()`
  and `load_weapons_text()`, and add the language to the `ELanguages` enum.
- New asset: drop the numbered `N.bmp` / `N.wav` into the right folder (renumber existing files upward by hand to make room
  for inserted frames) and update the loader ranges in `environment.cpp` (`loadBitmaps`/`loadSounds`) and the `Makefile` install
  lists if a new folder is introduced.

## Important Technical Nuances

- `src/main.h` include order is load-bearing: `debug.h` must precede Allegro headers on Windows (see `src/main.h`).
- `globals.h` may only be included from `src/atanks.cpp`; every other unit uses `externs.h` (see `src/globals.h`,
  `src/externs.h`).
- `CWeapon::get_delay_div()` guards volley weapons whose `delay` is zero (avoids division by zero for multi-shot weapons).
- The `NETWORK` define reaches the code via generated `config.h` on Linux and BSD builds; macOS builds do not get it, and
  there is no CMake Windows build. Network play is currently a Linux-only first draft; proper network development is deferred
  to a future task.
- `allegro.cfg` (tracked) disables vertical sync on Windows builds, working around Allegro 4 sync problems there. Broader
  UI-framework modernization away from Allegro 4 is deferred to a future task.
- The wrapper reconfigures on every invocation, so option changes cannot go stale in a reused build directory.
- `vs12`/`vs14` projects do not define `DATA_DIR` (`vs12`) or define it as `"."` (`vs14`), so Windows builds read data from the
  working directory.
- `dep/*.d` files are untracked legacy outputs from `make`-driven builds; the CMake build tracks dependencies internally
  via ninja. Refresh with a build rather than hand editing.
- `CSpinLock` is non-recursive and records its owner; taking it twice from the same thread deadlocks. Thread-sanitizer builds
  replace it with a mutex (`USE_MUTEX_INSTEAD_OF_SPINLOCK`).

## Known Issues and TODO Sources

- Open known issues live in `TODO_Xtra.md`: under `Important Issues`, the load-failure teardown segfault
  (`CEnvironment::destroy()` on partially loaded bitmaps) and the `src/missile.cpp` null-pointer exposure for new weapon
  data; under `General Issues`, the cppcheck triage items (`tank.cpp` `cur_x`/`cur_y`, teleport conditions, plus
  low-priority cleanups). Fixed entries stay there as regression notes (CppUTest environment, frame-rate physics).
- Planned work also lives in `TODO_Xtra.md` (`Planned Features`): the post-PF-1 upgrades (modern update checker, `atanks2`
  rename, full networking, including the network client shot limit) and the ideas moved over from the removed legacy `TODO`
  file (buy-screen scrollbar and randomize button, field repair kit, radar-resistant missile, semi-destructible rocks,
  underground mines, fireworks, shootable UFO, scalable main window, harder ground, high-voltage missiles, tornadoes,
  another armor level). None has a real to-do item number yet; `TODO.md` holds only the planning overview table (`PF-1`
  completed as 6.7.1).
- The `README` user documentation: buggy network client side.
- `CHANGELOG.md` (`## 6.7.1`) lists the completed Cleanup and Modernization; older entries in `docs/Changelog.history`
  document recurring AI-strength and SDI-tuning adjustments.

## Files and Directories Reference

| Path | Kind | Description |
|---|---|---|
| `src/atanks.cpp` | entry point + menus | `main()`, `parse_args()`, menu dispatch, config load/save |
| `src/gameloop.h/.cpp` | round driver | `game()`, `ObjectUpdater` threads, frame loop, winner logic |
| `src/globaldata.h/.cpp` | per-round state | `CGlobalData`: canvases, surface, turn order, object lists, locks |
| `src/environment.h/.cpp` | config + assets | `CEnvironment`: options, players, bitmaps/sounds/text, dir resolution |
| `src/globaltypes.h/.cpp` | enums/types | Stages, classes, wall/landslide/box modes, enum operators |
| `src/globals.h`, `src/externs.h` | state wiring | Single definitions vs `extern` declarations |
| `src/main.h` | common hub | Includes, shims, macros, catalog sizes |
| `src/files.h/.cpp` | persistence | Config, savegames, weapon text, directory scans |
| `src/text.h/.cpp` | localization | `TEXTBLOCK` loading/selection/rendering |
| `src/player.h/.cpp`, `src/player_types.h/.cpp` | players/AI types | State, economy, memory, enums |
| `src/aicore.h/.cpp` | AI | Background bot, documented plan/aim pipeline |
| `src/tank.h/.cpp`, `src/missile.h/.cpp`, `src/explosion.h/.cpp`, `src/beam.h/.cpp` | entities | Tank, projectile, detonation, laser |
| `src/weapon.h/.cpp`, `src/item.h/.cpp` | arsenal | Data records, 56 + 6 + 24 catalog sizes |
| `src/land.h/.cpp`, `src/sky.h/.cpp`, `src/levelcreator.h/.cpp` | world gen | Gradients, terrain/sky generation |
| `src/shop.h/.cpp`, `src/score.h/.cpp` | meta | Buy/sell UI, score sorting |
| `src/menu.h/.cpp`, `src/optionscreens.h/.cpp`, `src/option*.h/.cpp` | UI | CMenu framework, option screens/items |
| `src/network.h/.cpp`, `src/client.h/.cpp` | net | Transport + client protocol (`NETWORK` builds) |
| `src/sound.h/.cpp`, `src/clock.h/.cpp`, `src/spinlock.h/.cpp`, `src/zbuffer.h/.cpp`, `src/debug.h/.cpp`, `src/update.h/.cpp` | services | Audio, timers, locking, z-buffer, logging, updater |
| `src/bitmap.h`, `src/gfxData.h/.cpp`, `src/box.h/.cpp`, `src/button.h/.cpp` | gfx/UI bits | Forward decls, sGradient strips, boxes, buttons |
| `src/moon.h/.cpp`, `src/satellite.h/.cpp`, `src/teleport.h/.cpp`, `src/decor.h/.cpp`, `src/debris_pool.h/.cpp`, `src/floattext.h/.cpp`, `src/perlin.cpp`, `src/random.h/.cpp` | world extras | Moon, UFO, teleports, decor, debris, float text, noise, RNG |
| `src/atanks.rc`, `src/resource.h` | windows-only | Icon/version resources, MSVC defines |
| `src/winclock.h`, `src/wrap_dirent.h`, `src/optioncontent.h`, `src/optionitem.h` | shims/decls | Clock fix, dirent selector, option declarations |
| `Makefile` | build | GNU build (primary) |
| `vs12/`, `vs14/` | IDE | Legacy VS2013 / VS2015 solutions (retired toolsets) |
| `dep/`, `obj/.keep_dir` | build dirs | Ignored dependency files (legacy make outputs); object dir placeholder |
| `README.md`, `docs/` | docs | Front page plus technical documentation (planning files, release rules, specs) |
| `COPYING`, `LICENSE` | legal | Pointer + full license text (`LICENSE` is the single source of truth; consolidated in 6.7.1) |
| `atanks.desktop`, `io.github.EdenWorX.atanks.metainfo.xml` | packaging | Desktop entry / AppStream metadata |
| `allegro.supp`, `do_*.sh`, `gdb_memcheck.sh` | diagnostics | Valgrind suppressions and runners |
| `.clang-format` | style | Formatter definition (clang-format 19+) |
