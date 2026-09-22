# TODO-II-1: Load-failure teardown segfault

When arsenal loading fails, `main` (`src/atanks.cpp`) prints the error and returns `EXIT_FAILURE` without calling
`env.destroy()`. The global `CEnvironment` destructor then tears down partially loaded bitmap arrays, crashing with
SIGSEGV in `destroy_bitmap()`. The bug is pre-existing: it reproduces identically with the legacy positional loader on a
missing `weapons.txt`, so it is unrelated to the TOML migration. Fix with a guard (skip destruction of unloaded assets
or exit cleanly before teardown).

Source: `TODO_Xtra.md` (`Important Issues`). Status: see the overview table in `TODO.md`.

## [ ] II-1.1: Analysis

Reproduce the crash with a deliberately broken arsenal file and trace the exit path: early return in `main`, the global
`~CEnvironment()` call, and which partially loaded arrays (`title`, `button`, `misc`, `missile`, `stock`, `tank`,
`tank_gun`) reach `destroy_bitmap()` in a bad state. Record which slots hold garbage versus null.

### [ ] II-1.1.1: Reproduce the crash with a broken arsenal file and capture the backtrace
### [ ] II-1.1.2: Trace the teardown path and inventory every partially loaded array

## [ ] II-1.2: Discussion

Agree the guard strategy with the user: skip destruction of unloaded assets inside `CEnvironment::destroy()`, or return
from `main` through a clean teardown path before the exit. Clarify whether other early-`EXIT_FAILURE` paths need the same
treatment.

## [ ] II-1.3: Implementation, guard the teardown

Implement the agreed guard. Further parts (if the analysis finds more unsafe early-exit paths) become additional
implementation Work Packages inserted before Tests.

## [ ] II-1.4: Tests

Headless reproduction (broken arsenal file must exit with `EXIT_FAILURE` and no crash); existing `make test` stays green.

## [ ] II-1.5: Documentation

Record the fixed behavior in `CHANGELOG.md` if it triggers a release; no user-doc changes expected.

## [ ] II-1.6: Final testing and finalization

Validate with `./do_memcheck.sh`, re-run the reproduction, and close the item per `docs/todo_planning.md`.
