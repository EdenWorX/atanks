# Atomic Tanks

## Overview

Atomic Tanks is a turn-based artillery game in the Scorched Earth / Worms tradition. Each player controls a tank, buys weapons
and defensive items between rounds, and fires projectiles in turn-based order. The last tank standing wins the round. The game
supports human players, AI bots (multiple difficulty levels), teams (Jedi / Sith / Neutral), destructible terrain, wind and
weather, network play (host plus clients), and localized in-game text.

- Language: C++ (built with `-std=c++17`, see `CMAKE_CXX_STANDARD 17` in `CMakeLists.txt`).
- Graphics/audio/input library: Allegro 4 (`#include <allegro.h>` in `src/main.h:56`; the CMake build queries
  `allegro-config --cppflags/--libs`). Allegro must be installed separately; it is not vendored in this repository.
- Concurrency: POSIX threads on Linux and BSD (`Threads::Threads` in `CMakeLists.txt`),
  `std::thread`/`std::mutex`/`std::condition_variable` in game code, plus a custom spinlock (`src/spinlock.h`).
- Version: the `VERSION` variable in `Makefile` (currently `6.7`) is the single source of truth. Older version strings in other
  files will be synchronized or removed in the Cleanup and Modernization task (`TODO.md`, `WP PF-1.2`–`WP PF-1.4`).
- License: `LICENSE` is the single source of truth. Formerly contradicting license information (old GPLv2 `COPYING` text,
  `either version 2 ... or later` source headers, `io.github.EdenWorX.atanks.metainfo.xml:5` declaring `GPL-2.0-or-later`) was
  consolidated in the Cleanup and Modernization task (`TODO.md`, `WP PF-1.1`): headers now say version 3, the metainfo declares
  `GPL-3.0-or-later`, and `COPYING` is a pointer to `LICENSE`.
- Issue reports go to `https://github.com/EdenWorX/atanks/issues`. This is a manual fork moved from SourceForge to GitHub;
  updating the remaining SourceForge references is part of the Cleanup and Modernization task (`TODO.md`, `WP PF-1.6`).

## Repository Status and Documentation Scope

This document was generated from tracked files only (`git ls-files`, 496 files). The following were intentionally excluded from
analysis per `.gitignore` and `git ls-files --other`:

- Build outputs: `atanks` binary, `obj/*.o` (except the tracked placeholder `obj/.keep_dir`), `*.dep`.
- Logs: `atanks.log`, `allegro.log`, `memcheck.log`.
- Windows local config: `allegro.cfg` (tracked in git but matched by a `.gitignore` rule; known to disable vertical sync on
  Windows builds as a workaround for Allegro 4 sync problems).
- Screenshots: `screenshot_*.*` (`screenshot_0001..0007.bmp`, `screenshot_001/002.bmp` on disk).
- Agent/IDE-local state: `.opencode/` (including `node_modules`), `.aiignore`, `.idea/workspace.xml`, `talk_*.md`.
- `AGENTS.md` itself is matched by `.gitignore:79` and is therefore untracked-by-design documentation.

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
| `text/` | Localized in-game text files (`weapons*.txt`, `Help*.txt`, `ingame*.txt`, etc.) |
| `unicode.dat` | Allegro datafile used for fonts; also the probe file for data-dir detection. An old manual addition; ignored (not touched) until the post-cleanup move away from Allegro 4 makes it obsolete |
| `Makefile` | Primary GNU Make build (`VERSION 6.7`, the version single source of truth) |
| `vs12/`, `vs14/` | Legacy Visual Studio 2013 / 2015 solutions (retired toolsets v120/v140); Windows builds go through CMake |
| `dep/` | Ignored GCC dependency files (`*.d`, legacy make outputs) plus `.keep_dir` placeholder |
| `obj/` | Object output directory; only `.keep_dir` is tracked |
| `README`, `README_ru.txt` | Original user documentation (English + Russian) |
| `docs/Changelog.history` | Frozen release history up to 6.7 (renamed from `Changelog`; new releases go in `CHANGELOG.md`) |
| `TODO` | Legacy prioritized bug/feature list (almost a decade old; explicitly frozen — ignore it for now, `TODO.md` `WP PF-1.8`) |
| `TODO.md` | Canonical planning file (per `docs/todo_planning.md`); first item is `TODO-PF-1` Cleanup and Modernization |
| `docs/` | EdenWorX planning and release rules (`todo_planning.md`, `release_process.md`) |
| `COPYING`, `LICENSE` | License pointer + full license text (`LICENSE` is the single source of truth) |
| `credits.txt` | Authors, graphics, docs, translations, sound attributions |
| `atanks.desktop` | freedesktop menu entry (`Exec=atanks`) |
| `io.github.EdenWorX.atanks.metainfo.xml` | AppStream metadata |
| `atanks.ico`, `atanks.png` | Windows icon (used by `src/atanks.rc`) and Linux menu icon |
| `alleg44.dll`, `alleg44_64.dll` | Tracked Windows Allegro runtime DLLs (32/64-bit) |
| `allegro.supp` | Valgrind suppression file for Allegro/ALSA/X11 noise |
| `do_memcheck.sh`, `do_helgrind.sh`, `gdb_memcheck.sh` | Valgrind/helgrind/gdb helpers that run `./atanks` |
| `.clang-format` | clang-format style definition (requires clang-format 19+) |
| `.gitignore`, `.idea/*`, `cb/*` | Ignore rules and IDE metadata |

There is no test directory and no test target in any build file.

## Components

### Executables

One program is built: `atanks` (`atanks.exe` on `WIN32`).

- Entry point: `int32_t main(int32_t argc, char** argv)` in `src/atanks.cpp:1494`, closed by Allegro's `END_OF_MAIN()` in
  `src/atanks.cpp:1648`.
- Startup sequence in `main` (`src/atanks.cpp:1494-1647`):
  1. `parse_args()` (`src/atanks.cpp:1134`), called at `:1498`.
  2. `env.find_data_dir()` (`:1508`); failure exits with `EXIT_FAILURE`.
  3. `game_version` derived from `VERSION` (`:1513-1518`).
  4. `env.find_config_dir()` (`:1522`), then `loadConfig()` or `createConfig()` (`:1525-1526`).
  5. `env.loadGameFiles()` (`:1530`); failure exits.
  6. Optional `NETWORK` threads (`Send_And_Receive`, update checker, `:1535-1562`).
  7. Main-menu loop (`:1568-1613`) dispatching on `global.get_command()`: help, options, players, credits, network game, demo,
     or local play.
  8. `Save_Game_Settings()`, `env.destroy()`, `global.destroy()`, `allegro_exit()` (`:1633-1642`).
- Round execution funnels into `game()` declared in `src/gameloop.h:7` via `play_local()` (`src/atanks.cpp:1313`), `play_demo()`
  (`:1257`), and `play_networked()` (`:1383`; without `-DNETWORK` this path reports an error, `:1398-1410`).

### Libraries

There are no separate built libraries. Every `src/*.cpp` compiles and links into the single `atanks` binary
(`add_executable(atanks ...)` in `CMakeLists.txt`). The closest thing to internal libraries is a set of service modules with
narrow responsibilities:

| Module | Files | Responsibility |
|---|---|---|
| Debug/platform layer | `src/debug.h`, `src/debug.cpp` | OS detection (`ATANKS_IS_WINDOWS/LINUX/BSD/MSVC`), `snprintf`/`localtime` shims, `DEBUG_LOG*` macros |
| Global state | `src/globals.h`, `src/externs.h`, `src/globaldata.h/.cpp`, `src/environment.h/.cpp`, `src/globaltypes.h/.cpp` | The two globals (`GLOBALDATA global`, `ENVIRONMENT env`), enums, constants |
| Persistence | `src/files.h`, `src/files.cpp` | Config, savegames, weapon-text loading, directory scans |
| Text/localization | `src/text.h`, `src/text.cpp` | `TEXTBLOCK` file loading, line selection, text rendering helpers |
| Audio | `src/sound.h`, `src/sound.cpp` | Thin wrappers over `env.sounds` / `env.background_music` |
| Clock | `src/clock.h`, `src/clock.cpp`, `src/winclock.h` | Game/menu timers, MSVC clock workaround |
| Locking | `src/spinlock.h`, `src/spinlock.cpp` | `CSpinLock` (atomic-flag, non-recursive) |
| Z-buffer | `src/zbuffer.h`, `src/zbuffer.cpp` | 1-bit-per-pixel `ZBuffer::set/test` over `vector<bool>` |
| Update protocol | `src/update.h`, `src/update.cpp` | `Update_Data` / update-checker structures |
| Network transport | `src/network.h`, `src/network.cpp` | `MESSAGE`/`MESSAGE_QUEUE`, sockets (only when `NETWORK` is defined) |
| Network client | `src/client.h`, `src/client.cpp` | Client-side protocol constants and handling |

### Shared/Internal Utility Code

- `src/main.h` is the common include hub: it requires `debug.h` first (comment at `src/main.h:39-42`), then Allegro headers,
  then `globaltypes.h` and `wrap_dirent.h`, then the C++ standard headers. It defines `BUFFER_SIZE 256` (`:28`), `HOME_DIR`
  (`"AppData"` on Windows, `"HOME"` on Linux, `:137-141`), the `DATA_DIR` fallback `"."` (`:143-145`), math helpers (`SIGN`,
  `SIGNd`, `ROUND`, `ROUNDu`, `FABSDISTANCE2`, `:150-160`), sleep helpers (`USLEEP`, `MSLEEP`, `LINUX_SLEEP`, `LINUX_REST`,
  `:119-127`), and MSVC portability shims (`:102-115`).
- `src/externs.h:48-49` re-exports the two globals as `extern` for every translation unit except `src/atanks.cpp` (guarded by
  `ATANKS_SRC_ATANKS_CPP`, `:45-80`). It also declares shared scalars and the three content catalogs (`:53-69`).
- Content catalogs, defined in `src/files.cpp:26-28`: `WEAPON weapon[WEAPONS]`, `WEAPON naturals[NATURALS]`, `ITEM item[ITEMS]`,
  with sizes `WEAPONS 56`, `NATURALS 6`, `ITEMS 24` (`src/main.h:264-267`).
- `src/bitmap.h` is a forwarder declaring `struct BITMAP; struct gradient;`.
- `src/gfxData.h` (`sGfxData`) owns generated gradient strips and explosion graphics.
- `src/random.h` / `src/perlin.cpp` provide random numbers (`CHANGELOG.md` 6.7 entry notes thread-local modernized RNG) and
  noise for terrain/sky.
- `src/box.h`, `src/button.h`, `src/menu.h`, `src/optiontypes.h`, `src/optioncontent.h`, `src/optionitem*.h`,
  `src/optionscreens.h` form the menu/options UI framework (self-managing `Menu` of `OptionItem` entries).

### Gameplay Modules

| Area | Key files | Notes |
|---|---|---|
| Object hierarchy root | `src/virtobj.h/.cpp` (`VIRTUAL_OBJECT`) | List node (`prev`/`next`), position, dirty-rect updates, virtual `applyPhysics/draw/initialise`, pure `getClass()` |
| Physics mixin | `src/physobj.h/.cpp` (`PHYSICAL_OBJECT : VIRTUAL_OBJECT`) | Gravity/drag/mass, bounces, `weapType`, angle macros |
| Tank avatar | `src/tank.h/.cpp` (`TANK final : PHYSICAL_OBJECT`) | Aim/power/selection, health/shield, `moveTank`, `addDamage/applyDamage/explode/repair`, `CSpinLock damage_lock` |
| Projectile | `src/missile.h/.cpp` (`MISSILE final : PHYSICAL_OBJECT`) | `eMissileType{MT_WEAPON,MT_ITEM,MT_NATURAL,MT_MIND_SHOT}`, SDI/cluster/roller handling |
| Detonation | `src/explosion.h/.cpp` (`EXPLOSION final : PHYSICAL_OBJECT`) | Terrain deformation, throwing, damage, napalm/debris |
| Beams (lasers) | `src/beam.h/.cpp` | Laser-class weapons parallel to ballistic missiles |
| Arsenal data | `src/weapon.h/.cpp`, `src/item.h/.cpp` | Plain records: 56 weapons + 6 naturals + 24 items; unified index `THINGS = WEAPONS+ITEMS` |
| Player state | `src/player.h/.cpp` | Economy, inventories `nm[WEAPONS]/ni[ITEMS]`, personality, opponent memory, shop prefs, save/load, speech-line selection |
| Player/AI types | `src/player_types.h/.cpp` | `playerType` (HUMAN..DEADLY..NETWORK_CLIENT..), `ePlayerStages`, `eTeamTypes{SITH,NEUTRAL,JEDI}`, modular enum arithmetic |
| AI | `src/aicore.h/.cpp` (`AICore`) | Background-thread bot with documented pipeline: initialize, target/weapon selection, attack calculation, aiming traces, writeback |
| Shop | `src/shop.h/.cpp` (`bool shop(LevelCreator*)`) | Inter-round buy/sell UI |
| Scoring | `src/score.h/.cpp` (`sScore`, `sort_scores()`) | Caller deletes the returned array |
| Terrain/sky | `src/land.h/.cpp`, `src/sky.h/.cpp`, `src/levelcreator.h/.cpp`, `src/moon.h/.cpp`, `src/satellite.h/.cpp`, `src/teleport.h/.cpp`, `src/decor.h/.cpp`, `src/debris_pool.h/.cpp`, `src/floattext.h/.cpp` | 16 land + 16 sky gradients each (8 classic + 8 crispy), generators, decor, debris, floating text |
| Round driver | `src/gameloop.h/.cpp` (`game()`) | Round phases, AI thread, per-class `ObjectUpdater` threads, input, firing, winner detection |

### Data, Templates, and Resources

Installed by the `install` rules in `CMakeLists.txt` (mirroring the old `make install` layout) under `${INSTALLDIR}`:

- `button/*.bmp` (28 files, `0..27`), `misc/*.bmp` (18 files), `missile/*.bmp` (32 files, `0..31`), `stock/*.bmp` (80 files,
  `0..79`), `tank/*.bmp` (17 files), `tankgun/*.bmp` (10 files), `title/*.bmp` (4 files).
- `sound/*.wav` (27 files: `00-07`, `10-22`, `30-32`, `40`).
- `text/*.txt` (~90 files): per-topic per-language matrix for `gloat`, `ingame`, `instr`, `panic`, `kamikaze`, `retaliation`,
  `revenge`, `suicide`, `weapons`, `war_quotes`, `Help`, with language suffixes `_de`, `_fr`, `_it`, `_ru`, `_sk`, `_ES`,
  `.pt_BR` plus the English base file.
- `unicode.dat` (5604 bytes, `file` reports `Allegro datafile`), `COPYING`, `README`, `TODO`, `Changelog.history`, `*.txt`.
- `atanks.png` is installed to `.../share/icons/hicolor/48x48/apps`; `atanks.ico` is consumed by `src/atanks.rc` for the
  Windows build.

Weapon/item stats come from `text/weapons*.txt` (see `Configuration`). Speech/quote text comes from the other `text/*.txt` files
via `TEXTBLOCK`.

### Platform-Specific Code

- `src/debug.h:9-32` detects `ATANKS_IS_WINDOWS`, `ATANKS_IS_MSVC` (including the `ATANKS_HAS_MSVC12_BUG` workaround for
  `_MSC_VER < 1900`), `ATANKS_IS_BSD`, and `ATANKS_IS_LINUX`; anything else is a compile `#error`.
- `src/main.h:46-60` handles `ALLEGRO_NO_MAGIC_MAIN`, `ALLEGRO_HAVE_STDINT_H`, and `winalleg.h` inclusion on Windows; `:64-74`
  handles MSVC `PATH_MAX`, `_USE_MATH_DEFINES`, and POSIX-vs-MSVC headers.
- `src/winclock.h` plus `USLEEP`/`MSLEEP` in `src/main.h:126-134` work around an MSVC12 chrono problem.
- `src/wrap_dirent.h:9-16` includes `extern/dirent.h` on MSVC and the system `<dirent.h>` elsewhere. `src/files.cpp:786-840` has
  separate `scandir` paths for Win32 vs POSIX.
- `src/atanks.rc` (Windows resources: `A ICON ../atanks.ico`, Allegro icon, `VERSIONINFO`) and `src/resource.h` (MSVC-generated
  defines) are used only by the Visual Studio builds.

### External or Bundled Dependencies

The following were classified as external by metadata inspection; their internals were not analyzed:

- **Allegro 4 (system dependency).** Evidence: `README:23-26,32-33` requires the Allegro (development) package; `Makefile`
  queries `allegro-config` for Linux/macOS builds (`:134,140,148,150`); `vs12|vs14/README_allegro.txt` explain how to repoint
  include/lib paths to a local Allegro; `io.github.EdenWorX.atanks.metainfo.xml:19` states the game "runs on any platform
  Allegro4 runs on". Role: graphics, sound, input,
  timers. The build fails without it (verified: `allegro-config` is absent on this machine, and even `make -n user` prints
  `allegro-config: Datei oder Verzeichnis nicht gefunden`).
- **`src/extern/dirent.{h,c}` (bundled shim).** Evidence: header comment `Declaration of POSIX directory browsing functions and
  types for Win32. Author: Kevlin Henney ... Created March 1997. Updated June 2003` (`src/extern/dirent.h:4-12`) with a
  permissive use/copy/modify/distribute grant (`:31-41`). Role: POSIX `opendir/readdir` for MSVC builds only, selected by
  `src/wrap_dirent.h:10-14` and compiled in the `.vcxproj` files; the GNU build uses the system `<dirent.h>`.
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
 ENVIRONMENT env ......... fixed config + asset registry (environment.h)
 GLOBALDATA global ........ per-round mutable state (globaldata.h)
   |                               |
   +-- main-menu loop .............+-- round driver game() (gameloop.cpp)
   |    (menu/options/players/       |    |
   |     shop/selectPlayers)         |    +-- AICore thread (aicore.h) per AI tank
   |                                 |    +-- ObjectUpdater threads per eClass
   |                                 |    +-- object lists: TANK / MISSILE / BEAM /
   |                                 |         EXPLOSION / TELEPORT / DECOR / FLOATTEXT
   v                                 v
 files.cpp ................. config/save/weapon-text loading
 text.cpp .................. TEXTBLOCK localization
 sound.cpp ................. audio triggers
 network.cpp / client.cpp .. host/client transport (NETWORK builds)
```

- Draw/update ordering follows `eClass` in `src/globaltypes.h` (`CLASS_MISSILE, BEAM, TANK, TELEPORT, DECOR_DIRT, SMOKE,
  EXPLOSION, FLOATTEXT, COUNT`).
- Round stages follow `eRoundStages` (`STAGE_AIM, STAGE_FIRE, STAGE_SCOREBOARD, STAGE_ENDGAME`).
- Threading: `AICore` runs bot planning off the main thread (`mutex`/`condition_variable`, `start/stop/status` in
  `src/aicore.h`); `gameloop.cpp:89-243` spawns one `ObjectUpdater` thread per class behind `updMutex/updCondition` (`:85-86`)
  and joins them at `:497-541`. `SANITIZE_THREAD=YES` builds define `USE_MUTEX_INSTEAD_OF_SPINLOCK` (thread-sanitizer logic in
  `CMakeLists.txt`).
- Data flow for content: `text/weapons*.txt` -> `Load_Weapons_Text()` (`src/files.cpp`) -> `weapon[]/naturals[]/item[]` globals
  -> shop UI, AI planning, firing, explosions. `text/*.txt` (speech/help) -> `ENVIRONMENT::load_text_files()`
  (`src/environment.cpp:966ff`) -> `TEXTBLOCK*` fields -> menus, AI taunts, help screens.

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
`Atomic Tanks Version 6.7`.

### Autotools / Make Build

There is no Autotools setup (`configure`, `configure.in`, `aclocal.m4` do not exist). The `Makefile` is a thin cmake+ninja
wrapper (`TODO.md`, `WP PF-1.9`); the real build lives in `CMakeLists.txt` (CMake 3.25+, Ninja mandatory):

- Sources: `file(GLOB ... src/*.cpp)` with `CONFIGURE_DEPENDS`, so new files are picked up automatically.
- Version single source of truth: `project(atanks VERSION 6.7 ...)`; `src/config.h.in` generates `config.h` with the version macros
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
  `WindowsTargetPlatformVersion=8.1`). Both define `VERSION="6.7"` (matching `CMakeLists.txt`); `vs14` additionally defines
  `DATA_DIR="."`. Both link one of `alleg44.lib / alleg44_64.lib / alleg44_d.lib / alleg44_64_d.lib` per configuration plus the
  Win32 system libraries. `vs14` embeds `../atanks.ico`. `README_allegro.txt` in each folder explains how to repoint
  include/library paths and swap the DLL variants. Current Windows builds go through CMake (VS2026), not these solutions.
- Windows builds also pick up the tracked `allegro.cfg`, which disables vertical sync as a workaround for Allegro 4 sync
  problems there. The file is matched by `.gitignore` as Windows-local config but is kept in git deliberately — preserve it, do
  not "clean it up". Broader UI-framework modernization away from Allegro 4 is deferred until after the Cleanup and
  Modernization task (`TODO.md`).

### Visual Studio Project Files

Covered above; the `.vcxproj.filters` files only group files in Solution Explorer (German group names in `vs12`, English in
`vs14`) and carry no build semantics.

### Xcode Workspace / Configuration Files

None exist in the repository.

## Configuration

- Compile-time: `DATA_DIR` (`ATANKS_DATA_DIR` setting, default `<prefix>/share/atanks`, `"."` for `*user` goals), `VERSION`
  (`project(VERSION 6.7)`, via generated `config.h`), platform flags (`LINUX` / `MACOSX` in `config.h`), `NETWORK` (Linux and
  BSD only), `ATANKS_DEBUG*` flavors.
- Runtime data directory, resolved by `ENVIRONMENT::find_data_dir()` (`src/environment.cpp:387-416`): `--datadir` if readable,
  else the compiled `DATA_DIR` (verified by probing `unicode.dat` inside it), else `./` fallback.
- Runtime config directory, resolved by `ENVIRONMENT::find_config_dir()` (`src/environment.cpp:363-384`): `-c <path>` if given,
  else `$HOME/.atanks` (`HOME_DIR` = `HOME` on Linux, `AppData` on Windows, `src/main.h:144-148`). `Copy_Config_File()`
  (`src/files.cpp:334-390`) migrates a legacy `$HOME/.atanks-config.txt` into the directory.
- Main settings file: `<configDir>/atanks-config.txt`, loaded by `loadConfig()` (`src/atanks.cpp:727-757`, via
  `env.load_from_file()` plus per-player `PLAYER::load_from_file`) and written by `Save_Game_Settings()` (`:1448-1463`).
  `--noconfig` skips loading.
- Weapon/item stats: `Load_Weapons_Text()` (`src/files.cpp`, declared in `src/files.h:27`) reads `<dataDir>/text/weapons*.txt`,
  selecting the suffix by `env.language` (`weapons.txt`, `weapons_{fr,de,sk,ru,ES,it}.txt`, `weapons.pt_BR.txt`). English is
  always loaded first for numeric stats; a second pass overwrites only `name`/`desc` for localization. Sections `*WEAPONS*` /
  `*NATURALS*` / `*ITEMS*` carry `DS_NAME`/`DS_DESC`/`DS_DATA` triples (`eDataStage`, `src/globaltypes.h:81-86`).
- Speech/help text: `ENVIRONMENT::load_text_files()` (`src/environment.cpp:966ff`) loads `text/<base><suffix>` for `gloat`,
  `ingame`, `instr`, `panic`, `kamikaze`, `retaliation`, `revenge`, `suicide` (suffixes `.txt`, `_fr`, `_de`, `_it`, `.pt_BR`,
  `_ru`, `_sk`, `_ES`) plus `war_quotes[_it|_ru|_ES].txt`, into `TEXTBLOCK*` fields (`src/environment.h:248-257`).
- Savegames: `<configDir>/<game_name>.sav`, format `VERSION/GLOBAL/ENVIRONMENT/PLAYERS/***EOF***` (`src/files.cpp:43-77`);
  listing via `Find_Saved_Games()` (`*.sav` filter, `src/files.cpp:786-840`).
- Music: `Create_Music_Folder()` ensures a `music/` folder in the config dir (`src/files.cpp:395-412`); custom `*.bmp` files are
  picked up by `Find_Bitmaps()` (`:848-893`).

## Runtime Behavior

- Default invocation `./cmake-build-release/atanks` (run from the project root) equals `--windowed --width 800 --tall 600
  --datadir . depth 32` (`README:116-119`; defaults `DEFAULT_SCREEN_WIDTH 800` / `DEFAULT_SCREEN_HEIGHT 600` in
  `src/globaltypes.h:23-24`).
- First run with no human player opens the player-creation screen automatically; afterwards the flow is Players -> select tanks
  (2-10) -> buy screen (left-click buys, right-click sells, `Done` confirms) -> battle (`README:122-152`).
- In-battle keys (`README:155-176`): Space fires/selects, Enter confirms, Up/Down power and menu cycling, Left/Right gun aim and
  buy/sell, Esc cancels, F1 screenshot, F10 AI-takeover (or save on the buy screen), `v`/`V` volume down/up, `~` (or `#` on
  German keyboards) scoreboard.
- Network play (still rough per `README:179-214`): the host enables Networking in Options -> Network and restarts; clients set
  Server Address to the host IP and choose Network Game. Client tanks are color-coded (Jedi green, Sith purple, Neutral blue,
  player red). `TODO:5` records a bug: the network client must not get unlimited shots.
- Screenshot key F1 writes `screenshot_*.*` files (a `.gitignore`d artifact).
- Environment variables: `HOME` (Linux) or `AppData` (Windows) locates the config directory.

## Command-Line Tools

Main binary flags, parsed by `parse_args()` (`src/atanks.cpp:1134-1255`, help text at `:1413-1429`):

| Flag | Effect |
|---|---|
| `-h`, `--help` | Print help, exit with `HELP_REQUESTED (-100)` |
| `-fs` | Full screen |
| `--windowed` | Windowed mode |
| `-d`, `--depth <16\|32>` | Color depth into `env.colourDepth` |
| `-w`, `--width <>=512>` | Screen width (also half/temp variants) |
| `-t`, `--tall` (`--height`) `<>=320>` | Screen height (also half/temp variants) |
| `--datadir <path>` | Data directory into `env.dataDir` (must be readable) |
| `-c <path>` | Config/save directory into `env.configDir` |
| `--noconfig` | Do not load game settings |
| `--nosound` | Disable sound (`env.sound_enabled=false`) |
| `--noname` | Hide player names above tanks |
| `--nonetwork` | Disable networking (`allow_network=false`) |
| `--nobackground` | Hide the green menu background |
| `--nothread`, `--thread` | Accepted but ignored (deprecated) |

Missing option values print `ERROR: Missing argument` and exit `EXIT_FAILURE` (`:1248-1251`).

Standalone helpers (not built by `Makefile`):

- `do_memcheck.sh`: `valgrind --tool=memcheck --trace-children=yes --track-origins=yes --leak-check=full ... ./atanks`.
- `do_helgrind.sh`: `valgrind --tool=helgrind ... ./atanks`.
- `gdb_memcheck.sh`: memcheck under `gdb` (`--vgdb=full --vgdb-error=0`).
- `allegro.supp`: suppressions for `ld.so`/`dl_*`, `install_sound` mempool, ALSA (`snd_pcm_open`, `snd_config_*`), `_al_malloc`,
  and X11/Xrm realloc noise.

## Data Flow

1. Startup resolves `dataDir` and `configDir`, loads `atanks-config.txt`, players, weapon stats, text blocks, bitmaps, fonts,
   sounds, and background music (`ENVIRONMENT::loadGameFiles()`, `src/environment.cpp:1311`).
2. Menu/options/player/shop screens mutate `ENVIRONMENT` (options, rosters) and `PLAYER` objects (names, colors, teams,
   inventories, money).
3. `game()` (`src/gameloop.cpp:246`) runs a round: `init_new_round()` (`:261`), `set_tank_settings()` (`:270`), spawn of the
   `AICore` thread and per-class `ObjectUpdater` threads, then the frame loop (`:316-489`) over stages `STAGE_AIM -> STAGE_FIRE
   -> STAGE_SCOREBOARD -> STAGE_ENDGAME`.
4. Firing creates `MISSILE`/`BEAM` objects; impacts create `EXPLOSION`s, which deform `global.surface`/`global.terrain`, throw
   tanks, apply damage (with `TANK::damage_lock`), spawn debris/floattext, and may trigger AI revenge/panic logic via opponent
   memory.
5. Round end credits winners (`ENVIRONMENT::creditWinners`), sorts scores (`sort_scores()`), opens the shop for the next round,
   and persists settings and optional savegames to the config directory.

## Error Handling and Logging

- Fatal startup failures (missing data dir, unreadable game files) print an error and return `EXIT_FAILURE` from `main`
  (`src/atanks.cpp:1508-1533`).
- `parse_args()` rejects missing values with `ERROR: Missing argument` (`src/atanks.cpp:1248-1251`).
- `DEBUG_LOG(...)` and its flavors (`DEBUG_LOG_AIM/EMO/AI/FIN/OBJ/PHY`, `src/debug.h:105-170`) compile to no-ops unless the
  matching `ATANKS_DEBUG*` macro is defined; with `ATANKS_DEBUG` they call `debug_log()` with `file:line|function()` position
  info (`AT hugeET_POS`, `:92-102`).
- `debug_log()` (`src/debug.cpp:19-68`) writes to `atanks.log` on Windows or when `ATANKS_DEBUG_LOGTOFILE` is set, otherwise to
  `stdout`; output is mutex-protected.
- MSVC-incompatible POSIX calls are shimmed in `src/main.h:109-122` (`snprintf`, `strncpy`, `access`, `strcasecmp`, `strdup`,
  `unlink`, `mkdir`).
- In-game failures are surfaced modally where practical (e.g. the non-`NETWORK` network-game path reports an error instead of
  crashing, `src/atanks.cpp:1398-1410`).

## Testing and Validation

- Automated unit suite exists: `tests/` (CppUTest, decoupled logic units) wired to `ctest`.
  - `make test` builds and runs the suite in the flag-selected build directory; `make test-all` runs it in release and debug.
  - `make test-asan` / `test-ubsan` / `test-tsan` run the suite under sanitizers (`TODO.md`, `WP PF-1.11`).
- The closest equivalents to tests are:
  - `make -n <target>` dry-run to validate flag expansion (verified here for `make -n user`).
  - A full `make user` build followed by a `./cmake-build-release/atanks --windowed` smoke run.
  - `make debug` / `aidebug` / `fulldebug` builds plus the Valgrind helpers (`do_memcheck.sh`, `do_helgrind.sh`,
    `gdb_memcheck.sh` with `allegro.supp`) for memory/thread validation.
  - `CHANGELOG.md` entries as regression notes (e.g. 6.7 lists fixed crashes, AI, and land-creation bugs).
  - Static-analysis and doc targets exist (`tools/run-cppcheck.sh`, `make doc`); full Doxygen coverage of public APIs is
    still pending as part of the Cleanup and Modernization task (`TODO.md`, `WP PF-1.12`).
- Accepted validation bar (the game is an interactive GUI application): a green `make test` (plus `make test-all` where
  affordable), plus manual validation — developers actually test their changes in-game.
- Known-issue sources: `TODO.md` itself (canonical planning file, first item `TODO-PF-1` Cleanup and Modernization). The legacy
  `TODO` file (1 bug
  + 7 features + ~10 under consideration) is almost a decade old and explicitly frozen — ignore it for now; proper `TODO-PF-*`
    entries will be created after `TODO-PF-1` (`WP PF-1.8`). Also `README:209-216` (buggy network client),
    and the `TODO`/`FIXME`/`BUG`/`HACK` grep surface, which only matches `DEBUG_LOG*` call sites rather than real markers.
- Bug reports go to `https://github.com/EdenWorX/atanks/issues` (this fork moved from SourceForge to GitHub; updating the
  remaining SourceForge references in `README`, `credits.txt`, the metainfo file, and help texts is part of `TODO.md`, `WP
  PF-1.6`).

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
- Documentation files (`README.md`, `AGENTS.md`, `TODO*.md`, `docs/*`) wrap prose at a maximum line length of 128 characters

## Adding or Modifying Code

- New gameplay entity: subclass `VIRTUAL_OBJECT` (or `PHYSICAL_OBJECT` for ballistic behavior) in a new `src/<name>.h/.cpp`
  pair, return the matching `eClass` from `getClass()`, add the files to the VS projects' file lists (CMake picks up
  `src/*.cpp` via `file(GLOB ...)` automatically), and wire creation/update/draw into `gameloop.cpp` and teardown into
  `GLOBALDATA::destroy` paths.
- New weapon or item: extend the `*WEAPONS*` / `*ITEMS*` sections of `text/weapons.txt` (and its translations for display
  strings), keep the numeric field count in sync with `Load_Weapons_Text()`, and adjust the `WEAPONS`/`ITEMS` sizes in
  `src/main.h:264-267` if the count changes; check AI selection (`AICore`), shop availability (`ENVIRONMENT::genItemsList`), and
  sound/pic mappings. Note: there is no spec for this positional format beyond the parser code; migration to a documented format
  (INI or YAML) with a clear spec and simple parser is planned as a late step (`TODO.md`, `WP PF-1.17`).
- New option/menu entry: add the `eMenuClass`/`eEntryType` value in `src/optiontypes.h`, construct the item in
  `menu.cpp`/`optionscreens.cpp`, and persist it in `ENVIRONMENT::save_to_file/load_from_file`.
- New language: copy the `text/*.txt` matrix with the new suffix, extend the suffix lists in `ENVIRONMENT::load_text_files()`
  and `Load_Weapons_Text()`, and add the language to the `eLanguages` enum.
- New asset: drop the numbered `N.bmp` / `N.wav` into the right folder (renumber existing files upward by hand to make room
  for inserted frames) and update the loader ranges in `environment.cpp` (`loadBitmaps`/`loadSounds`) and the `Makefile` install
  lists if a new folder is introduced.

## Important Technical Nuances

- `src/main.h` include order is load-bearing: `debug.h` must precede Allegro headers on Windows (`src/main.h:39-42`).
- `globals.h` may only be included from `src/atanks.cpp`; every other unit uses `externs.h` (`src/globals.h:1-3`,
  `src/externs.h:45-80`).
- `WEAPON::getDelayDiv()` guards volley weapons whose `delay` is zero (avoids division by zero for multi-shot weapons).
- The `NETWORK` define reaches the code via generated `config.h` on Linux and BSD builds; macOS builds do not get it, and
  there is no CMake Windows build. Network play is currently a Linux-only first draft; proper network development is deferred
  until after the Cleanup and Modernization task (`TODO.md`).
- `allegro.cfg` (tracked) disables vertical sync on Windows builds, working around Allegro 4 sync problems there. Broader
  UI-framework modernization away from Allegro 4 is deferred until after the Cleanup and Modernization task (`TODO.md`, `WP
  PF-1.7`).
- The wrapper reconfigures on every invocation, so option changes cannot go stale in a reused build directory.
- `vs12`/`vs14` projects do not define `DATA_DIR` (`vs12`) or define it as `"."` (`vs14`), so Windows builds read data from the
  working directory.
- `dep/*.d` files are untracked legacy outputs from `make`-driven builds; the CMake build tracks dependencies internally
  via ninja. Refresh with a build rather than hand editing.
- `CSpinLock` is non-recursive and records its owner; taking it twice from the same thread deadlocks. Thread-sanitizer builds
  replace it with a mutex (`USE_MUTEX_INSTEAD_OF_SPINLOCK`).

## Known Issues and TODO Sources

- `TODO:4-5`: network client must not get unlimited shots (bug).
- `TODO:7-18`: missing buy-screen scrollbar, missing buy-screen randomize button, field-repair-kit item, radar-resistant
  missile, more frequent client ground-surface updates, client buying screen, semi-destructible rocks.
- `TODO:29-49`: under consideration — underground mines, firework rockets, shootable UFO, scalable main window (blocked on
  Allegro 5 / a port the file calls a no-opt), an entry literally questioning its own meaning (`Harder ground -> What is that
  supposed to mean?`), high-voltage missiles, tornadoes, another armor level.
- `README:209-216`: buggy network client side.
- `CHANGELOG.md` entry (6.7) lists recently fixed crashes and AI bugs; older entries in `docs/Changelog.history` document
  recurring AI-strength and SDI-tuning adjustments.

## Files and Directories Reference

| Path | Kind | Description |
|---|---|---|
| `src/atanks.cpp` | entry point + menus | `main()`, `parse_args()`, menu dispatch, config load/save |
| `src/gameloop.h/.cpp` | round driver | `game()`, `ObjectUpdater` threads, frame loop, winner logic |
| `src/globaldata.h/.cpp` | per-round state | `GLOBALDATA`: canvases, surface, turn order, object lists, locks |
| `src/environment.h/.cpp` | config + assets | `ENVIRONMENT`: options, players, bitmaps/sounds/text, dir resolution |
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
| `src/menu.h/.cpp`, `src/optionscreens.h/.cpp`, `src/option*.h/.cpp` | UI | Menu framework, option screens/items |
| `src/network.h/.cpp`, `src/client.h/.cpp` | net | Transport + client protocol (`NETWORK` builds) |
| `src/sound.h/.cpp`, `src/clock.h/.cpp`, `src/spinlock.h/.cpp`, `src/zbuffer.h/.cpp`, `src/debug.h/.cpp`, `src/update.h/.cpp` | services | Audio, timers, locking, z-buffer, logging, updater |
| `src/bitmap.h`, `src/gfxData.h/.cpp`, `src/box.h/.cpp`, `src/button.h/.cpp` | gfx/UI bits | Forward decls, gradient strips, boxes, buttons |
| `src/moon.h/.cpp`, `src/satellite.h/.cpp`, `src/teleport.h/.cpp`, `src/decor.h/.cpp`, `src/debris_pool.h/.cpp`, `src/floattext.h/.cpp`, `src/perlin.cpp`, `src/random.h/.cpp` | world extras | Moon, UFO, teleports, decor, debris, float text, noise, RNG |
| `src/atanks.rc`, `src/resource.h` | windows-only | Icon/version resources, MSVC defines |
| `src/winclock.h`, `src/wrap_dirent.h`, `src/optioncontent.h`, `src/optionitem.h` | shims/decls | Clock fix, dirent selector, option declarations |
| `Makefile` | build | GNU build (primary) |
| `vs12/`, `vs14/` | IDE | Legacy VS2013 / VS2015 solutions (retired toolsets) |
| `dep/`, `obj/.keep_dir` | build dirs | Ignored dependency files (legacy make outputs); object dir placeholder |
| `README`, `README_ru.txt`, `TODO`, `TODO.md`, `docs/`, `credits.txt` | docs | User docs, legacy tasks (frozen, ignore for now), canonical planning file + planning/release rules, attributions |
| `COPYING`, `LICENSE` | legal | Pointer + full license text (`LICENSE` is the single source of truth; consolidated in `WP PF-1.1`) |
| `atanks.desktop`, `io.github.EdenWorX.atanks.metainfo.xml` | packaging | Desktop entry / AppStream metadata |
| `allegro.supp`, `do_*.sh`, `gdb_memcheck.sh` | diagnostics | Valgrind suppressions and runners |
| `.clang-format` | style | Formatter definition (clang-format 19+) |
