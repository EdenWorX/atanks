# TODO-PF-13: Scalable main window

Make the main window scalable. Blocked until the transition away from Allegro 4 is done (see TODO-PF-3).

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-13.1: Analysis

Inventory the fixed-size assumptions (800x600 defaults, menu coordinates, canvas and terrain buffers, asset sizes) and
track TODO-PF-3 for the framework move that unblocks this item.

## [ ] PF-13.2: Discussion

Agree the scaling model with the user (resizable window, resolution options, asset scaling versus fixed canvas) once the
successor framework is known.

## [ ] PF-13.3: Implementation, part A (blocked on TODO-PF-3)

No implementation until the Allegro 4 transition unblocks it. Further parts become additional implementation Work
Packages inserted before Tests.

## [ ] PF-13.4: Tests

Smoke-test window sizes and resolutions in-game once implemented.

## [ ] PF-13.5: Documentation

Document the options in `README.md`; `CHANGELOG.md` on release.

## [ ] PF-13.6: Final testing and finalization

Full validation across resolutions, then close the item per `docs/todo_planning.md`.
