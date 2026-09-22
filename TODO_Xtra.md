# TODO Extra: Issues and Planned Features

This file tracks issues and planned features discovered while decomposing `TODO-PF-1` in `TODO.md` that fall outside any Work
Package scope there. `TODO.md` remains the canonical planning overview; the deferred post-PF-1 follow-ups (network
functionality, UI-framework modernization) are merged into TODO-PF-4 and TODO-PF-3 below. The legacy `TODO`
file was removed after the 6.7.1 Cleanup and Modernization (its ideas preserved in the `Planned Features` section below).

## Important Issues

- [x] **High**: load-failure teardown segfault — fixed 2026-09-22 (arsenal loads before graphics init, fail fast).
- [x] **High**: `make test*` env failure (system CppUTest gone, v4.0 vs CMake 4.3 floor) — fixed 2026-09-21 (shim).
- [x] **High**: missile null-pointer exposure — fixed 2026-09-22 (debug assert plus skip-homing fallback).

## General Issues

- [x] **Medium**: `tank.cpp` `cur_x`/`cur_y` cleanup — fixed 2026-09-22 (declarations moved into the loop body).
- [x] **Medium**: `teleport.cpp` condition review — fixed 2026-09-22 (remote-end null guard, non-copyable, else cleanup).
- [x] **Low**: redundant conditional assignments — fixed 2026-09-22 (plain assignments, plus `set_color`).
- [x] **Low**: `shop.cpp` findings — fixed 2026-09-22 (`std::fill`, `weap` const, bare `teamFee`).
- [x] **Low**: `atanks.rc` COPYING.txt reference — fixed 2026-09-22 (now `COPYING`).
- [ ] **Low**: `src/optiontypes.h:10` says "or (at your menu) any later version" — "menu" is a typo for "option" in the
  license header. Left untouched because header normalization belonged to completed `WP PF-1.1`; fix with any future edit of
  that header.
  Planned as TODO-GI-6 (see TODO.md).
- [ ] **Low**: the 12 screenshot URLs in `io.github.EdenWorX.atanks.metainfo.xml` (`<image>https://atanks.sourceforge.io/...`)
  still point at the old SourceForge site, which this fork no longer controls. Out of scope for `WP PF-1.6` (screenshots were
  not listed there); decide whether to re-host the images (e.g. in the GitHub repo) and update or drop the block.
  Planned as TODO-GI-7 (see TODO.md).
- [ ] **Low**: `README.md` claims `allegro-config` is absent on the documenting machine, but Allegro 4.4.3 is installed in the
  current environment. Reword to drop the machine-specific absence claim when that section is next touched.
  Planned as TODO-GI-8 (see TODO.md).
- [x] **High**: game speed doubled above 60 FPS — fixed 2026-09-21 via frame-rate scaling, user-verified.
- [x] **High**: FPS follow-ups (debris speed, quadratic gravity) — fixed 2026-09-21, user-verified at 60 vs 120 FPS.

## Planned Features

- [ ] **Upgrade**: develop a modern update checker to replace the disabled legacy SourceForge checker in `src/atanks.cpp`
  (masked with `#if 0`, with its farewell URL). The replacement needs a version endpoint on project infrastructure the fork
  controls (e.g. GitHub) and should reuse the `env.check_for_updates` option and the `update_data`/`update_string` plumbing
  where practical. Decided with the user during `WP PF-1.6`; no post-PF-1 item number assigned yet.
  Planned as TODO-PF-2 (see TODO.md).
- [ ] **Upgrade**: transition the project to `atanks2` ("Atomic Tanks 2") as part of the big post-PF-1 updates, notably
  the UI-framework modernization away from Allegro 4 (including removal of the then-obsolete `unicode.dat`; until then the
  file is ignored, not touched). Rationale: this fork coexists with the
  still-active upstream project, so a distinct project name resolves the remaining install collisions the AppStream rename
  (`WP PF-1.6.4`) could not — the `atanks` binary, `atanks.desktop`, icons, save/config paths, and user-visible titles. Scope
  includes the binary and desktop-entry names, the AppStream ID, build/install rules, docs and metadata, and user-facing
  strings; decide with the user whether `atanks2` also marks a compatibility break (savegames, network protocol).
  Planned as TODO-PF-3 (see TODO.md).
- [ ] **Upgrade**: replace the current slim networking capabilities (a Linux-only first draft; `NETWORK` handling stays
  as-is until this item is tackled) with a full networking system that allow true mult-player games over local network
  and the internet. _Moved from `TODO`_: Make sure network client doesn't get unlimited shots.
  Planned as TODO-PF-4 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add scroll bar to buying screen.
  Planned as TODO-PF-5 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add randomize button to buying screen to have items automatically purchased.
  Planned as TODO-PF-6 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add a "field repair kit" item: Spend a turn to repair your tank rather than fire.
  Limited uses, heals more than Auto Repair Kit.
  Planned as TODO-PF-7 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add a radar resistant missile: A missile designed to avoid the missile defence system
  by masking its heat signature. Yield: Large missile
  Planned as TODO-PF-8 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add rocks as semi-destructable items. Explosions look normal, but the carving radius
  is halved, so that the amount of debris and the size of the chunk taken out are quartered.
  Planned as TODO-PF-9 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add underground mines
  - This feature needs a discussion about what it actually means and what the effect on the game will be
  Planned as TODO-PF-10 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: rocket-like fireworks
  - This feature needs a discussion about what it actually means and what the effect on the game will be
  Planned as TODO-PF-11 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Make it possible to shoot down the UFO
  Planned as TODO-PF-12 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Make main window scalable
  - This feature is blocked until the transition away from Allegro 4 is done
  Planned as TODO-PF-13 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add an option to make the ground harder
  - Possible options, like "hard", "rock" and "steel" could cause the carving radius be multiplied by 0.75, 0.5 and 0.1
  - The carving radius is the radius of the ground being removed by an explosion and the base for the amount of debris sent flying
  - This feature needs a discussion whether and how this effects the riot weapons
  - This feature needs a discussion whether the dirt weapons generate a small amount of damage
  Planned as TODO-PF-14 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add high voltage missiles (discharge on impact or when within range)
  - (Idea by Bharat Dhareshwar)
  Planned as TODO-PF-15 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add tornadoes which pick up tanks and objects. Objects could be the new rocks
  - (Idea by Bharat Dhareshwar)
  Planned as TODO-PF-16 (see TODO.md).
- [ ] **Feature**: _Moved from `TODO`_: Add Another level of armour improvement
  - This feature needs a discussion about what it actually means and what the effect on the game will be
  Planned as TODO-PF-17 (see TODO.md).
