# TODO-II-1: Load-failure teardown segfault

When arsenal loading fails, `main` (`src/atanks.cpp`) prints the error and returns `EXIT_FAILURE` without calling
`env.destroy()`. The global `CEnvironment` destructor then tears down partially loaded bitmap arrays, crashing with
SIGSEGV in `destroy_bitmap()`. The bug is pre-existing: it reproduces identically with the legacy positional loader on a
missing `weapons.txt`, so it is unrelated to the TOML migration. Fix with a guard (skip destruction of unloaded assets
or exit cleanly before teardown).

Source: `TODO_Xtra.md` (`Important Issues`). Status: see the overview table in `TODO.md`.

## [x] II-1.1: Analysis

Reproduce the crash with a deliberately broken arsenal file and trace the exit path: early return in `main`, the global
`~CEnvironment()` call, and which partially loaded arrays (`title`, `button`, `misc`, `missile`, `stock`, `tank`,
`tank_gun`) reach `destroy_bitmap()` in a bad state. Record which slots hold garbage versus null.

### [x] II-1.1.1: Reproduce the crash with a broken arsenal file and capture the backtrace
### [x] II-1.1.2: Trace the teardown path and inventory every partially loaded array

Findings (2026-09-22 session, verified with gdb plus Valgrind memcheck; detail logs in `/tmp/repro_bt.log`,
`/tmp/repro_fresh.log`, `/tmp/vg.log` — outside the repo by design):

- Reproduced deterministically: broken `text/weapons.toml` prints the parse error plus
  `ERROR: An error occurred trying to read weapons file.`, then dies with SIGSEGV (exit 139) instead of `EXIT_FAILURE`.
  Missing and empty arsenal files crash the same way.
- Exit path: `main` returns `EXIT_FAILURE` (`src/atanks.cpp:1491`) → exit handlers → `~CEnvironment()`
  (`src/environment.cpp:60`) → `CEnvironment::destroy()` → `destroy_bitmap()` (`src/environment.cpp:190`, the `sky`
  call) → SIGSEGV inside Allegro (`graphics.c:1482`, invalid size-8 read of address `0x90`).
- Surprise: `load_config` → `init_game_settings` already performs the full init (Allegro, graphics mode, `first_init`,
  `load_bitmaps`, `load_sounds`, `load_fonts`) *before* `load_game_files` runs. So at crash time `sky`, the bitmap
  arrays, sounds, fonts, TEXTBLOCKs, and players are all allocated — not null. The bitmap arrays are null-safe
  (`calloc` plus null-terminated loops); the crashing pointer is `sky` itself, whose struct holds garbage (a `0x90`
  field) with no prior Valgrind-invalid write, pointing at an Allegro-side/memory-bitmap lifecycle problem rather than
  a missing null guard. Root-cause options go to WP II-1.2.
- Early `EXIT_FAILURE` paths before any allocation (`parse_args`, `find_data_dir`) need no guard; both config paths
  (`load_config`, `create_config` via `init_game_settings`) fully allocate and share the teardown.

## [x] II-1.2: Discussion

Agreed the guard strategy with the user: skip destruction of unloaded assets inside `CEnvironment::destroy()`, or return
from `main` through a clean teardown path before the exit. Clarify whether other early-`EXIT_FAILURE` paths need the same
treatment.

Decision (2026-09-22 session): **reorder startup** — load the arsenal before graphics/bitmap/sound/font init so load
failures exit while almost nothing is allocated. Implementation: split the Allegro/graphics/asset tail out of
`init_game_settings()` into `init_graphics_and_assets()`, call it from `main` after `load_game_files()` succeeds.
Both config paths share the teardown, so both are covered; pre-allocation exits need no guard.

## [x] II-1.3: Implementation, guard the teardown

Implemented the agreed reorder (see II-1.2 decision): `init_game_settings()` keeps settings plus Allegro core and colour
depth (needed for correct `makecol` depths in `CPlayer` ctors); new `init_graphics_and_assets()` holds graphics mode and
all asset loading and runs from `main` after `load_game_files()` succeeds. First-run player creation moved after the
graphics init (the editor needs `misc` bitmaps); further parts none.

## [x] II-1.4: Tests

Headless reproduction (broken arsenal file must exit with `EXIT_FAILURE` and no crash); existing `make test` stays green.

Verified 2026-09-22: broken/missing arsenal exits 1 with no crash; intact run reaches the menu loop; `--noconfig`
reaches the player editor (pre-existing interactive behavior, no crash); `make test` green.

## [x] II-1.5: Documentation

Record the fixed behavior in `CHANGELOG.md` if it triggers a release; no user-doc changes expected.

No release triggered (PATCH-level crash fix, version untouched); recorded at the next release instead.

## [x] II-1.6: Final testing and finalization

Validate with `./do_memcheck.sh`, re-run the reproduction, and close the item per `docs/todo_planning.md`.

Valgrind memcheck on the failing case reports 0 errors (was: invalid read plus SIGSEGV); reproduction re-run clean.
Item finished; removal from `TODO.md` plus plan-file deletion waits for the next release per the Removal Rule.
