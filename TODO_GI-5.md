# TODO-GI-5: atanks.rc COPYING.txt reference

`src/atanks.rc` references `COPYING.txt`, but the file is named `COPYING` (no `.txt`). Fix the filename reference when
that resource is next edited (version-string work there belongs to its own task, so this fix rides along or lands on
its own).

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [x] GI-5.1: Analysis

Confirm the mismatch between the `LegalCopyright` string in `src/atanks.rc` and the tracked `COPYING` filename, and check
whether any other resource string carries the same stale suffix.

Findings (2026-09-22 session): mismatch confirmed — `COPYING` is tracked, `COPYING.txt` does not exist. Line 52 is the
only `.txt` reference in the file, and its sibling `credits.txt` exists, so exactly one string is stale. Landing
choice goes to WP GI-5.2.

## [x] GI-5.2: Discussion

Confirm with the user whether the fix lands on its own or rides along with the next version-string edit of the resource.

Decision (2026-09-22 session): **fix alone now** — single stale string, no version-string work pending.

## [x] GI-5.3: Implementation, fix the reference

Correct the filename reference. No further parts expected.

Implemented 2026-09-22: `COPYING.txt` → `COPYING` in the `LegalCopyright` string (`src/atanks.rc:52`). Verified:
`make test` green; string syntax intact by inspection.

## [x] GI-5.4: Tests

No behavior change; verify the resource still compiles in a Windows/CMake resource build if available.

No Windows resource compiler available in this session; change is a quoted-string literal only (syntax intact), and
`make test` is green. Windows compile check flagged for an environment with VS2026 if ever needed.

## [x] GI-5.5: Documentation

No doc changes.

## [x] GI-5.6: Final testing and finalization

Confirm the reference, then close the item per `docs/todo_planning.md`.

Reference confirmed (`COPYING` tracked, string fixed). Item finished; removal from `TODO.md` plus plan-file deletion
waits for the next release per the Removal Rule.
