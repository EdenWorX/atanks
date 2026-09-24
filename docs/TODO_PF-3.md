# TODO-PF-3: atanks2 rename and Allegro move

Transition the project to `atanks2` ("Atomic Tanks 2") as part of the big post-PF-1 updates. Rationale: this fork
coexists with the still-active upstream project, so a distinct project name resolves the remaining install collisions
the AppStream rename could not. Strategy is abstraction-first: Allegro 4 (deeply embedded — drawing primitives,
`BITMAP`/`SAMPLE`/`FONT` types, input, sound, and timers across ~15 files, reached through the single `main.h` hub)
is abstracted behind a plugin interface first; the replacement framework is chosen in a follow-up plan. Decided:
`USE_ALLEGRO_4` defaults ON and CMake fails fast when set OFF; the rename stays in this item; `unicode.dat` moves to
an Allegro 4 assets subdirectory (Allegro 4 stays, so no removal); derived-class tests run headless in CI with
interactive checks manual in-game.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-3.1: Analysis, map Allegro 4 usage

Inventory every Allegro 4 call, type, and macro per area: functions used, types owned, and files involved. Areas:

### [ ] PF-3.1.1: Window and screen system
### [ ] PF-3.1.2: Keyboard input
### [ ] PF-3.1.3: Mouse input
### [ ] PF-3.1.4: Graphics output
### [ ] PF-3.1.5: Collision detection
### [ ] PF-3.1.6: Audio output
### [ ] PF-3.1.7: Font usage
### [ ] PF-3.1.8: Text output
### [ ] PF-3.1.9: Colors and bitmap masks
### [ ] PF-3.1.10: Networking functionality (raw sockets today; adapter scope versus TODO-PF-4)
### [ ] PF-3.1.11: Timers
### [ ] PF-3.1.12: Events (Allegro 4 is polling-based; the abstract interface is implemented by polling)
### [ ] PF-3.1.13: Other areas the list above misses

## [ ] PF-3.2: Discussion, abstraction plan

Develop the per-area abstraction plan from the WP PF-3.1 inventory, and agree the design conventions with the user.

### [ ] PF-3.2.1: Agree the class-versus-helper convention for the new system with the user
### [ ] PF-3.2.2: Agree the event interface shape over the polling model with the user
### [ ] PF-3.2.3: Agree the networking adapter scope versus TODO-PF-4 with the user

## [ ] PF-3.3: Implementation, area grouping

Group the inventoried Allegro 4 areas into logical classes and global function helpers per the agreed convention.
Tasks emerge from WP PF-3.2.

## [ ] PF-3.4: Implementation, abstract base classes

Develop the abstract base classes for the covered areas per the agreed plan. Tasks emerge from WP PF-3.2.

## [ ] PF-3.5: Implementation, Allegro 4 derived classes

Develop the explicit Allegro 4 classes inheriting from the new abstract base classes. Tasks emerge from WP PF-3.2.

## [ ] PF-3.6: Implementation, substitute usage and wire the option

Substitute Allegro 4 usage throughout the source base with the new class system (nothing Allegro 4 specific may
remain in game code). Add the `USE_ALLEGRO_4` option, enabled by default, with CMake failing fast when disabled;
wire the Allegro 4 abstraction classes to the enabled option. Tasks emerge from WP PF-3.2.

## [ ] PF-3.7: Tests, abstraction and parity

Unit-test the abstract base classes where UI-framework-independent algorithms are involved, and the derived Allegro 4
classes where no interaction is involved (memory bitmaps, no display); the headless set runs in CI, display and
interactive checks stay manual in-game. Verify behavior parity per area plus frame-loop performance (no regression
from the abstraction). Test lists emerge from WP PF-3.2.

## [ ] PF-3.8: Implementation, atanks2 rename

Rename binary, desktop entry, AppStream ID, save/config paths, build/install rules, docs, metadata, and user-facing
strings to `atanks2`.

### [ ] PF-3.8.1: Rename the binary and desktop entry
### [ ] PF-3.8.2: Rename the AppStream ID
### [ ] PF-3.8.3: Rename save/config paths
### [ ] PF-3.8.4: Update build/install rules
### [ ] PF-3.8.5: Update docs, metadata, and user-facing strings
### [ ] PF-3.8.6: Decide compatibility break (savegames, network protocol) with the user

## [ ] PF-3.9: Implementation, relocate unicode.dat

Move `unicode.dat` into an Allegro 4 assets subdirectory and update the data-dir probe (`find_data_dir`), install
rules, and docs that reference it.

### [ ] PF-3.9.1: Choose the subdirectory and move the file
### [ ] PF-3.9.2: Update the probe path, install rules, and docs

## [ ] PF-3.10: Documentation, plugin system

Document the new UI-framework plugin system for backend implementers. Structure emerges from WP PF-3.2.

## [ ] PF-3.11: Final testing and finalization

Full install/run validation, behavior parity per area, and performance check in-game, then close the item per
`docs/todo_planning.md`.
