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

## Planned Features

No open items — no enhancement was found that belongs outside the current `TODO-PF-1` scope. Post-PF-1 work is listed under
Planned follow-ups in `TODO.md`; user-owned `TODO-PF-*` entries for the legacy `TODO` contents will be created after `TODO-PF-1`
per `WP PF-1.8`.
