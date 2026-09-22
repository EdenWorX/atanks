# TODO-PF-3: atanks2 rename and Allegro move

Transition the project to `atanks2` ("Atomic Tanks 2") as part of the big post-PF-1 updates, notably the UI-framework
modernization away from Allegro 4 (including removal of the then-obsolete `unicode.dat`; until then the file is ignored,
not touched). Rationale: this fork coexists with the still-active upstream project, so a distinct project name resolves
the remaining install collisions the AppStream rename could not — the `atanks` binary, `atanks.desktop`, icons,
save/config paths, and user-visible titles.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-3.1: Analysis

Inventory every collision point (binary and desktop-entry names, AppStream ID, build/install rules, docs and metadata,
user-facing strings, save/config paths) and survey what the Allegro 4 replacement must cover (graphics, sound, input,
timers) before any rename lands.

### [ ] PF-3.1.1: Inventory every collision point (binary, desktop entry, AppStream ID, paths, strings, metadata)
### [ ] PF-3.1.2: Survey what the Allegro 4 replacement must cover (graphics, sound, input, timers)

## [ ] PF-3.2: Discussion

Decide with the user whether `atanks2` also marks a compatibility break (savegames, network protocol), the rename scope
and order, and the Allegro successor.

## [ ] PF-3.3: Implementation, part A (rename scope fixed by the discussion)

Execute the first agreed rename/move slice. Further parts (remaining rename areas, framework migration steps including
`unicode.dat` removal) become additional implementation Work Packages inserted before Tests once the analysis fixes the scope.

## [ ] PF-3.4: Tests

`make test` stays green per slice; smoke-test install layout and data/config path resolution.

## [ ] PF-3.5: Documentation

Update `README.md`, packaging metadata, and the planning rules on release; note the change in `CHANGELOG.md`.

## [ ] PF-3.6: Final testing and finalization

Full install/run validation per slice, then close the item per `docs/todo_planning.md`.
