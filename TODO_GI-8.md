# TODO-GI-8: README allegro-config claim

`README.md` claims `allegro-config` is absent on the documenting machine, but Allegro 4.4.3 is installed in the current
environment. Reword to drop the machine-specific absence claim when that section is next touched.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [x] GI-8.1: Analysis

Read the affected `README.md` passage and confirm the current environment state (Allegro present, `allegro-config`
working) so the rewording states verified facts only.

Findings (2026-09-22 session): `allegro-config --version` reports 4.4.3 — present and working, so the "absent on this
machine" parenthetical (`README.md:189-190`) is false. The host claim ("The build fails without it") stays true
(`CMakeLists.txt` fails fast without `allegro-config`). Wording choice goes to WP GI-8.2.

## [x] GI-8.2: Discussion

Confirm the replacement wording with the user (drop the claim entirely versus restating it as a prerequisite note).

Decision (2026-09-22 session): **drop the parenthetical** — the prerequisite is already covered in Development
Workflow.

## [x] GI-8.3: Implementation, reword the passage

Apply the agreed rewording. No further parts expected.

Implemented 2026-09-22: false parenthetical dropped from `README.md`.

## [x] GI-8.4: Tests

No behavior change; prose stays within the 128-column documentation limit.

Prose re-wrapped within limits (session-verified).

## [x] GI-8.5: Documentation

The `README.md` change itself is the documentation.

## [x] GI-8.6: Final testing and finalization

Confirm the wording, then close the item per `docs/todo_planning.md`.

Wording confirmed. Item finished; removal from `TODO.md` plus plan-file deletion waits for the next release per the
Removal Rule.
