# TODO Extra: Issues and Planned Features

This file tracks issues and planned features discovered while decomposing `TODO-PF-1` in `TODO.md` that fall outside any Work
Package scope there. `TODO.md` remains the canonical source for the planned work; the deferred post-PF-1 follow-ups (network
functionality, UI-framework modernization) live in its Planned follow-ups section and are not duplicated here. The legacy `TODO`
file is frozen per `WP PF-1.8` and is not triaged here either.

## Important Issues

No open items — decomposition of `WP PF-1.1`–`WP PF-1.17` surfaced no blocking or high-risk issue outside Work Package scope.
(This is a planning-only pass; no source code was changed.)

## General Issues

- [ ] **Low**: `src/atanks.rc:52` references `COPYING.txt`, but the file is named `COPYING` (no `.txt`). Left untouched because
  version-string work on that file belongs to `WP PF-1.2`; fix the filename reference when that package edits the resource.
- [ ] **Low**: the 12 screenshot URLs in `io.github.EdenWorX.atanks.metainfo.xml` (`<image>https://atanks.sourceforge.io/...`)
  still point at the old SourceForge site, which this fork no longer controls. Out of scope for `WP PF-1.6` (screenshots were
  not listed there); decide whether to re-host the images (e.g. in the GitHub repo) and update or drop the block.

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
