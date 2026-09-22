# TODO-GI-7: Metainfo screenshot URLs

The 12 screenshot `<image>` URLs in `io.github.EdenWorX.atanks.metainfo.xml` still point at the old SourceForge site,
which this fork no longer controls. Decide whether to re-host the images (e.g. in the GitHub repo) and update the block,
or drop the block.

Source: `TODO_Xtra.md` (`General Issues`). Status: see the overview table in `TODO.md`.

## [ ] GI-7.1: Analysis

Check whether the SourceForge URLs still resolve, inventory which screenshots exist locally (if any), and find out where
GitHub-hosted images for the metainfo file could live (repo assets, release artifacts, or project site).

## [ ] GI-7.2: Discussion

Decide with the user: re-host and update the URLs, or drop the screenshot block from the metainfo file.

## [ ] GI-7.3: Implementation, update or drop the block

Apply the agreed outcome. No further parts expected.

## [ ] GI-7.4: Tests

Validate the metainfo file (e.g. `appstreamcli validate` if available); no behavior change.

## [ ] GI-7.5: Documentation

The metainfo file itself is the documentation; no further doc changes.

## [ ] GI-7.6: Final testing and finalization

Confirm the validation, then close the item per `docs/todo_planning.md`.
