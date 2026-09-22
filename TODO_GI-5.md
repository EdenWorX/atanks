# TODO-GI-5: atanks.rc COPYING.txt reference

`src/atanks.rc` references `COPYING.txt`, but the file is named `COPYING` (no `.txt`). Fix the filename reference when
that resource is next edited (version-string work there belongs to its own task, so this fix rides along or lands on
its own).

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [ ] GI-5.1: Analysis

Confirm the mismatch between the `LegalCopyright` string in `src/atanks.rc` and the tracked `COPYING` filename, and check
whether any other resource string carries the same stale suffix.

## [ ] GI-5.2: Discussion

Confirm with the user whether the fix lands on its own or rides along with the next version-string edit of the resource.

## [ ] GI-5.3: Implementation, fix the reference

Correct the filename reference. No further parts expected.

## [ ] GI-5.4: Tests

No behavior change; verify the resource still compiles in a Windows/CMake resource build if available.

## [ ] GI-5.5: Documentation

No doc changes.

## [ ] GI-5.6: Final testing and finalization

Confirm the reference, then close the item per `docs/todo_planning.md`.
