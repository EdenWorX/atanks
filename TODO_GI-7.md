# TODO-GI-7: Metainfo screenshot URLs

The 12 screenshot `<image>` URLs in `io.github.EdenWorX.atanks.metainfo.xml` still point at the old SourceForge site,
which this fork no longer controls. Decide whether to re-host the images (e.g. in the GitHub repo) and update the block,
or drop the block.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [x] GI-7.1: Analysis

Check whether the SourceForge URLs still resolve, inventory which screenshots exist locally (if any), and find out where
GitHub-hosted images for the metainfo file could live (repo assets, release artifacts, or project site).

Findings (2026-09-22 session): all 12 SourceForge screenshot URLs still resolve (HTTP 200, checked individually).
No screenshots exist locally (gitignored `screenshot_*` absent, none tracked). No GitHub-hosted replacements exist —
re-hosting needs 12 screenshots captured, uploaded (repo assets or release artifacts), and re-linked, which exceeds this
session. `appstreamcli`/`appstream-util` are available for validation. Decision goes to WP GI-7.2.

## [x] GI-7.2: Discussion

Decide with the user: re-host and update the URLs, or drop the screenshot block from the metainfo file.

Decision (2026-09-22 session): **drop the block** — links are live today but uncontrolled; re-hosting needs 12 new
captures and stays a later task if artwork is wanted back.

## [x] GI-7.3: Implementation, update or drop the block

Apply the agreed outcome. No further parts expected.

Implemented 2026-09-22: `screenshots` block (12 uncontrolled URLs) removed from the metainfo file.

## [x] GI-7.4: Tests

Validate the metainfo file (e.g. `appstreamcli validate` if available); no behavior change.

Validated 2026-09-22 with `appstreamcli validate`: no new findings (12 pedantic screenshot findings gone with the
block); remaining 1 error (`tag-duplicated update_contact`) plus 1 info are pre-existing on `HEAD` — reported as a
follow-up candidate, out of scope here.

## [x] GI-7.5: Documentation

The metainfo file itself is the documentation; no further doc changes.

## [x] GI-7.6: Final testing and finalization

Confirm the validation, then close the item per `docs/todo_planning.md`.

Validation confirmed (see GI-7.4). Item finished; removal from `TODO.md` plus plan-file deletion waits for the next
release per the Removal Rule.
