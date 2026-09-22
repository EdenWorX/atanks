# TODO Extra: Issues and Planned Features

This file tracks issues and planned features discovered while decomposing `TODO-PF-1` in `TODO.md` that fall outside any Work
Package scope there. `TODO.md` remains the canonical planning overview; the deferred post-PF-1 follow-ups (network
functionality, UI-framework modernization) live in its Planned follow-ups section and are not duplicated here. The legacy `TODO`
file was removed after the 6.7.1 Cleanup and Modernization (its ideas preserved in the `Planned Features` section below).

## Important Issues

- [ ] **High**: segfault when game-data loading fails: if arsenal loading returns false, `main` prints the error but then
  crashes with SIGSEGV in `CEnvironment::destroy()` → `destroy_bitmap()` during `exit()` cleanup of partially loaded
  bitmaps (backtrace verified 2026-09-21). Reproduces identically with the legacy positional loader on a missing
  `weapons.txt`, so pre-existing and unrelated to the TOML migration; needs a guard (skip destroy of unloaded assets
  or clean `EXIT_FAILURE` before teardown). Found while validating `PF-1.17.3.4` with a deliberately broken file.
- [x] **High**: `make test*` broken environment-wide since 2026-09-21: the system CppUTest 4.0 install (headers, libs,
  `.pc`) vanished, so tiers 1-2 miss, and the pinned `v4.0` FetchContent fallback fails because that release declares
  `cmake_minimum_required` below the installed CMake 4.3 floor. Fixed 2026-09-21 without touching the agreed pin: set
  `CMAKE_POLICY_VERSION_MINIMUM 3.5` narrowly around the FetchContent wiring (CMake's suggested escape) and cleared the
  stale probe cache in existing build dirs (`cmake -U'pkgcfg_lib_CPPUTEST*' -U'CPPUTEST_*'`); `make test-all` green in
  release and debug. Remaining noise: one `-Wnonnull` warning inside CppUTest's own `Utest.cpp` under GCC 16.
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
- [x] **High**: game speed doubles when `FRAMES` is set above 60 (e.g. 120 Hz displays). The frame pacer (`check_fps()`,
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
- [x] **High**: FPS-independence follow-ups found during validation of the above: (1) flying debris moved at twice the
  wall-clock speed above 60 FPS because `CExplosion` debris velocities (`src/explosion.cpp:893-894`) were fixed
  pixels-per-frame; (2) shot range shrank markedly at 120 FPS (user: "twice the power for the same distance"). Root
  cause of (2): gravity as a per-frame velocity increment must scale quadratically (`1/FPS^2`), but `fall_vector`
  used the linear `fps_mod` (`1/FPS`) — at 120 FPS gravity pulled twice as hard in wall time (proven by replicating the
  integrator: linear gives ranges 273/154/82 px at 30/60/120 FPS, quadratic gives 150/154/156). Fixed via
  `fall_vector = gravity * fps_mod / frame_count_mod` (`CEnvironment::set_fps()` plus the gravity-option path), with
  the same second-order treatment for land-slide gravity (already quadratic), satellite accel/velocity (exact),
  parachute relaxation/cap/deploy, rocket launch velocity, cartoon hover delay, smoke relaxation, and meteor tumble;
  the drag relaxation fix stands as correct first-order. AI aiming needed no change (its power formula even becomes
  FPS-invariant with correct gravity). Residual rate-class items, deferred as rare/cosmetic: repulsor-shield impulses,
  SDI check rate, naturals spawn rate, satellite shoot chance, wind random-walk rate, damage-flash threshold,
  velocity-magnitude stop thresholds, menu millisecond truncation. Validated 2026-09-21: user confirmed identical
  shot distance (human and AI) and identical debris behavior at 60 vs 120 FPS.

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
- [ ] **Upgrade**: replace the current slim networking capabilities with a full networking system that allow true mult-player
  games over local network and the internet. _Moved from `TODO`_: Make sure network client doesn't get unlimited shots.
- [ ] **Feature**: _Moved from `TODO`_: Add scroll bar to buying screen.
- [ ] **Feature**: _Moved from `TODO`_: Add randomize button to buying screen to have items automatically purchased.
- [ ] **Feature**: _Moved from `TODO`_: Add a "field repair kit" item: Spend a turn to repair your tank rather than fire.
  Limited uses, heals more than Auto Repair Kit.
- [ ] **Feature**: _Moved from `TODO`_: Add a radar resistant missile: A missile designed to avoid the missile defence system
  by masking its heat signature. Yield: Large missile
- [ ] **Feature**: _Moved from `TODO`_: Add rocks as semi-destructable items. Explosions look normal, but the carving radius
  is halved, so that the amount of debris and the size of the chunk taken out are quartered.
- [ ] **Feature**: _Moved from `TODO`_: Add underground mines
  - This feature needs a discussion about what it actually means and what the effect on the game will be
- [ ] **Feature**: _Moved from `TODO`_: rocket-like fireworks
  - This feature needs a discussion about what it actually means and what the effect on the game will be
- [ ] **Feature**: _Moved from `TODO`_: Make it possible to shoot down the UFO
- [ ] **Feature**: _Moved from `TODO`_: Make main window scalable
  - This feature is blocked until the transition away from Allegro 4 is done
- [ ] **Feature**: _Moved from `TODO`_: Add an option to make the ground harder
  - Possible options, like "hard", "rock" and "steel" could cause the carving radius be multiplied by 0.75, 0.5 and 0.1
  - The carving radius is the radius of the ground being removed by an explosion and the base for the amount of debris sent flying
  - This feature needs a discussion whether and how this effects the riot weapons
  - This feature needs a discussion whether the dirt weapons generate a small amount of damage
- [ ] **Feature**: _Moved from `TODO`_: Add high voltage missiles (discharge on impact or when within range)
  - (Idea by Bharat Dhareshwar)
- [ ] **Feature**: _Moved from `TODO`_: Add tornadoes which pick up tanks and objects. Objects could be the new rocks
  - (Idea by Bharat Dhareshwar)
- [ ] **Feature**: _Moved from `TODO`_: Add Another level of armour improvement
  - This feature needs a discussion about what it actually means and what the effect on the game will be
