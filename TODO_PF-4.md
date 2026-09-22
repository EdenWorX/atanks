# TODO-PF-4: Full networking system

Replace the current slim networking capabilities (a Linux-only first draft; `NETWORK` handling stays as-is until this
item is tackled) with a full networking system that allows true multiplayer games over local network and the internet.
The network client must not get unlimited shots.

Source: `TODO_Xtra.md` (`Planned Features`). Status: see the overview table in `TODO.md`.

## [ ] PF-4.1: Analysis

Inventory the current transport (`src/network.h/.cpp`, `src/client.h/.cpp`), the `NETWORK` guards, the client shot bug,
and the missing pieces for true multiplayer (client buy screen, ground-surface updates, protocol robustness). Gather the
requirements for local-network and internet play.

### [ ] PF-4.1.1: Inventory the current transport, client code, and `NETWORK` guards
### [ ] PF-4.1.2: Document the protocol gaps (robustness, versioning, shot limiting)
### [ ] PF-4.1.3: List the client capability gaps (buy screen, ground-surface updates)

## [ ] PF-4.2: Discussion

Agree the protocol scope, the client-capability set, and the compatibility story (savegames, protocol versioning) with
the user.

## [ ] PF-4.3: Implementation, part A (scope fixed by the discussion)

Build the first agreed slice (e.g. the shot-limit fix plus protocol hardening). Further parts (client screens, update
rates, internet play) become additional implementation Work Packages inserted before Tests once the analysis fixes the scope.

## [ ] PF-4.4: Tests

Protocol unit tests where separable; multi-instance smoke tests on local network.

## [ ] PF-4.5: Documentation

Document setup and limits in `README.md`; note the feature in `CHANGELOG.md` on release.

## [ ] PF-4.6: Final testing and finalization

Multiplayer validation (host plus clients, all tank colors and teams), then close the item per `docs/todo_planning.md`.
