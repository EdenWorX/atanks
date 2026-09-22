# TODO-GI-8: README allegro-config claim

`README.md` claims `allegro-config` is absent on the documenting machine, but Allegro 4.4.3 is installed in the current
environment. Reword to drop the machine-specific absence claim when that section is next touched.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [ ] GI-8.1: Analysis

Read the affected `README.md` passage and confirm the current environment state (Allegro present, `allegro-config`
working) so the rewording states verified facts only.

## [ ] GI-8.2: Discussion

Confirm the replacement wording with the user (drop the claim entirely versus restating it as a prerequisite note).

## [ ] GI-8.3: Implementation, reword the passage

Apply the agreed rewording. No further parts expected.

## [ ] GI-8.4: Tests

No behavior change; prose stays within the 128-column documentation limit.

## [ ] GI-8.5: Documentation

The `README.md` change itself is the documentation.

## [ ] GI-8.6: Final testing and finalization

Confirm the wording, then close the item per `docs/todo_planning.md`.
