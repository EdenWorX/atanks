# TODO-PF-5: Buy-screen scrollbar

Add a scroll bar to the buying screen (`src/shop.cpp`), which currently shows a fixed selection of items.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-5.1: Analysis

Read the shop UI (`src/shop.cpp`, `src/menu.cpp`, button/box primitives): how items are listed, paged, and confirmed,
and where a scrollbar fits the Allegro 4 UI framework without breaking buy/sell input (left-click buys, right-click
sells).

## [ ] PF-5.2: Discussion

Agree the scrollbar behavior with the user (dragging, wheel, keyboard; interaction with the confirm flow).

## [ ] PF-5.3: Implementation, scrollbar widget and shop wiring

Build the widget and wire it into the buy screen. No further parts expected; follow-ups become additional
implementation Work Packages inserted before Tests.

## [ ] PF-5.4: Tests

Smoke-test the buy screen with mouse and keyboard (buy, sell, scroll, confirm).

## [ ] PF-5.5: Documentation

Note the UI change in `README.md` if the buy-screen help text covers navigation; `CHANGELOG.md` on release.

## [ ] PF-5.6: Final testing and finalization

Full buy-screen validation, then close the item per `docs/todo_planning.md`.
