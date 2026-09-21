# TODO Extra: Issues and Planned Features

This file tracks issues and planned features discovered while decomposing `TODO-PF-1` in `TODO.md` that fall outside any Work
Package scope there. `TODO.md` remains the canonical source for the planned work; the deferred post-PF-1 follow-ups (network
functionality, UI-framework modernization) live in its Planned follow-ups section and are not duplicated here. The legacy `TODO`
file is frozen per `WP PF-1.8` and is not triaged here either.

## Important Issues

- [ ] **High**: potential null-pointer dereference in `src/missile.cpp:413-423` (`MISSILE::applyPhysicsFunky`). `launchWeap`
  is null for any weapon type other than `FUNKY_BOMBLET`/`FUNKY_DEATHLET`, but line 419 dereferences it unconditionally
  while lines 420-422 guard with ternaries. Verified safe with shipped data (parents map to bomblet submunitions, and AI
  mind-shots reuse those types), so any new weapon data or physics assignment putting `PT_FUNKY_FLOAT` on another type
  segfaults. Found via cppcheck `nullPointer` during `WP PF-1.12`; fix with a defensive guard when that function is next
  touched, not here (gameplay physics needs in-game validation).

## General Issues

- [ ] **Medium**: `src/tank.cpp:1066-1067` assigns `cur_x`/`cur_y` without ever reading them (cppcheck `unreadVariable`,
  `variableScope`). Found during `WP PF-1.12` triage; needs gameplay-context review to decide whether the assignments (and
  their computations) can go or something was meant to consume them.
- [ ] **Medium**: `src/teleport.cpp:91,101,207` conditions reported always-true (cppcheck `knownConditionTrueFalse`), and
  `src/teleport.cpp:112` lacks copy semantics (`noCopyConstructor`, `noOperatorEq`). Found during `WP PF-1.12` triage; needs
  gameplay-context review.
- [ ] **Low**: `if (x) x=false` patterns logically equivalent to plain assignment (cppcheck `duplicateConditionalAssign`)
  in `src/tank.cpp:632`, `src/explosion.cpp:744`, and `src/floattext.cpp:342`. Found during `WP PF-1.12` triage; simplify when
  those functions are next touched.
- [ ] **Low**: `src/shop.cpp:831,912,931` (raw loop vs `std::fill`, `weap` const-correctness, unused `teamFee`) needs
  gameplay-context review. Found during `WP PF-1.12` triage.
- [ ] **Low**: `src/atanks.rc:52` references `COPYING.txt`, but the file is named `COPYING` (no `.txt`). Left untouched because
  version-string work on that file belongs to `WP PF-1.2`; fix the filename reference when that package edits the resource.
- [ ] **Low**: `src/optiontypes.h:10` says "or (at your menu) any later version" — "menu" is a typo for "option" in the
  license header. Left untouched because header normalization belonged to completed `WP PF-1.1`; fix with any future edit of
  that header.
- [ ] **Low**: the 12 screenshot URLs in `io.github.EdenWorX.atanks.metainfo.xml` (`<image>https://atanks.sourceforge.io/...`)
  still point at the old SourceForge site, which this fork no longer controls. Out of scope for `WP PF-1.6` (screenshots were
  not listed there); decide whether to re-host the images (e.g. in the GitHub repo) and update or drop the block.
- [ ] **Low**: `README.md` claims `allegro-config` is absent on the documenting machine, but Allegro 4.4.3 is installed in the
  current environment. Reword to drop the machine-specific absence claim when that section is next touched.
- [ ] **High**: game speed doubles when `FRAMES` is set above 60 (e.g. 120 Hz displays). The frame pacer (`check_fps()`,
  `src/gameloop.cpp:661`) keeps real frame time correct, but only some motion is scaled by `env.fps_mod`
  (`src/environment.cpp:1643`); fixed per-frame steps run twice as often per wall-clock second. Confirmed unscaled:
  explosion pacing via `weap->etime` (`src/explosion.cpp:62,604`), `CFloatText` rise/sway velocities
  (`src/floattext.cpp:121-122`), volley cadence (`weapon.delay * env.volley_delay`, `src/tank.cpp:151`,
  `src/gameloop.cpp:178`), UFO satellite acceleration/velocity (`src/satellite.cpp:26-32`). Still to audit: land-slide
  velocity (`CGlobalData::slide_land()`), tank aim/power adjust rates, menu-loop animation steps (`src/menu.cpp:801`). Fix
  direction: scale frame-count delays by `frames_per_second / 60` and per-frame velocities by `60 / frames_per_second`
  (new `env` factor next to `fps_mod`), then validate in-game at 60 vs 120 FPS. Found from user in-game testing during
  `WP PF-1.15` verification. Status 2026-09-21: implemented in the working tree (`env.frame_count_mod` in
  `src/environment.h:159`, scaled explosion/floattext/volley/satellite/landslide/aim-dials/menu sites); builds, unit
  tests, and `make doc` are green, in-game 60-vs-120 comparison still pending with the user.

## Planned Features

- [ ] **Upgrade**: develop a modern update checker to replace the disabled legacy SourceForge checker in `src/atanks.cpp`
  (masked with `#if 0`, with its farewell URL). The replacement needs a version endpoint on project infrastructure the fork
  controls (e.g. GitHub) and should reuse the `env.check_for_updates` option and the `update_data`/`update_string` plumbing
  where practical. Decided with the user during `WP PF-1.6`; no post-PF-1 item number assigned yet.
- [ ] **Upgrade**: transition the project to `atanks2` ("Atomic Tanks 2") as part of the big post-PF-1 updates, notably the move
  away from Allegro 4 (see the UI-framework modernization follow-up in `TODO.md`). Rationale: this fork coexists with the
  still-active upstream project, so a distinct project name resolves the remaining install collisions the AppStream rename
  (`WP PF-1.6.4`) could not — the `atanks` binary, `atanks.desktop`, icons, save/config paths, and user-visible titles. Scope
  includes the binary and desktop-entry names, the AppStream ID, build/install rules, docs and metadata, and user-facing
  strings; decide with the user whether `atanks2` also marks a compatibility break (savegames, network protocol).
