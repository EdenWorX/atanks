# EdenWorX To-Do Planning Rules

This document defines how to-do items are written, classified, numbered, and tracked in the planning files referenced in
`TODO.md`. It is the canonical reference for the planning rules; `TODO.md` itself holds only the open to-do items as an
overview table and references their detailed plans in corresponding per-item files (see `TODO.md Overview Table` below).

These rules also apply to every per-item file!

## Plan Files

Detailed plans live in one file per to-do item: even a single item plan (TODO-PF-1, Cleanup and Modernization) grew beyond
any acceptable single-file size, so a second item would have pushed `TODO.md` over every limit. One file per item is the
only rational layout.

Per-item plan files are named `TODO_<CAT>-<nr>.md`: the category prefix plus the item number, e.g. `TODO_PF-1.md`,
`TODO_GI-1.md`, `TODO_II-2.md` (underscore after `TODO`, hyphen between category and number).

## Hierarchy

To-do items use a four-level hierarchy so coding agents and humans can refer to work items precisely:

- **To-Do Item**: A classified open item (`TODO-GI-*`, `TODO-II-*`, `TODO-PF-*`).
- **Work Package**: A coherent group of work inside a to-do item.
  - Number format: `<CAT>-<item>.<wp>`, e.g. `GI-1.1`, `PF-1.1`. The category prefix (`GI`/`II`/`PF`) makes every Work Package
    number globally unambiguous, even when two items share the same numeric suffix.
  - Usually corresponds to one source file, class, or closely related feature area.
- **Implementation Task**: A concrete coding task.
  - Number format: `<CAT>-<item>.<wp>.<task>`, e.g. `GI-1.1.1`.
  - Should normally fit into one focused coding session.
- **Action Item**: A fine-grained checklist item inside an Implementation Task.
  - Number format: `<CAT>-<item>.<wp>.<task>.<action>`, e.g. `GI-1.1.1.1`.

**Mandatory rule**: Work Package, Implementation Task, and Action Item references must always use the full category-prefixed
number (e.g. `PF-1.1`, task `PF-1.1.1`). Never write a bare relative number such as `1.1`, which is ambiguous across
items of different categories.

## Document Structure

Per-item files (`TODO_PF-<nr>.md`, `TODO_GI-<nr>.md`, `TODO_II-<nr>.md`) use this fixed heading hierarchy so the CLion
Structure Viewer (`#` title, `##` chapters, `###` sections) and human readers get a stable overview:

- Phase (to-do item) → `#` title of the file: `# TODO-<type>-<nr>: <title>`
- Work Package → `##` chapter: `## [ ] <type>-<nr>.<wp>: <title>`
- Implementation Task → `###` section: `### [ ] <type>-<nr>.<wp>.<task>: <title>`
- Action Item (optional) → unordered list item: `- [ ] **<type>-<nr>.<wp>.<task>.<action>**: <short description>`

Structural rules:

- One `#` title per to-do item file; never nest items. The only other `##` chapters are Work Packages plus the
  `Planned follow-ups` and `Feature-Complete Checklist` sections.
- The `WP ` prefix is used in prose references (e.g. `WP PF-1.1`), not in the Work Package heading itself, which carries the
  bare category-prefixed number (e.g. `## [ ] PF-1.1: <title>`).
- Action Item numbers always use dots (`PF-1.1.1.1`), never dashes.
- Do not group Work Packages under theme subheadings; record ordering constraints as `Dependencies:` notes in the Work Package
  description instead.
- Checkbox state follows the Completion Rules (`[ ]` → `[x]`, parents only when children complete). Note that Markdown
  renderers display `[ ]` inside headings as literal text, not as clickable boxes; only `- [ ]` list items (Action Items,
  checklist entries) stay interactive.
- Keep prose (descriptions, task text) wrapped at 128 columns per the repository documentation guidelines; headings that cannot
  be split are exempt like table rows.

## TODO.md Overview Table

`TODO.md` keeps the general explanation plus a reference to these rules, and one status chapter: a human-readable overview
table with one row per to-do item. Columns, in order: `Item`, `Description`, `File`, `Status`.

- `Item`: the category-prefixed item number (`PF-1`, `GI-1`, `II-2`).
- `Description`: a very short (few-word) summary of the item.
- `File`: a link to the written plan file (`[TODO-PF-2](./TODO_PF-2.md)`), or `_none_` while no plan file exists yet.
- `Status`: one of `not planned`, `in planning`, `not started`, `planned`, `_in progress_`, `**completed**`. `in planning` means
  the plan file exists and is being drafted; `not started` means planning is finished (or was deemed sufficient) and
  implementation has not begun; `planned` means the plan is complete and awaits implementation.

"Human-readable" means the columns are aligned with padding spaces. Markdown rendering ignores the padding, but aligned
columns are far easier to read for humans. Table rows are exempt from the 128-column prose limit. Example:

| Item | Description                          | File                        | Status        |
|------|--------------------------------------|-----------------------------|---------------|
| PF-1 | Cleanup and Modernization            | _none_                      | **completed** |
| PF-2 | Add modern update check              | [TODO-PF-2](./TODO_PF-2.md) | in planning   |
| GI-1 | Verify `cur_x`/`cur_y` in `tank.cpp` | _none_                      | not planned   |

## Classification and Numbering

To-do items are classified into three kinds, each with its own numbering that restarts at 1:

- `TODO-GI-*` — **General Issues** (defects). e.g. `TODO-GI-1`.
- `TODO-II-*` — **Important Issues** (design constraints). e.g. `TODO-II-2`.
- `TODO-PF-*` — **Planned Features** (enhancements). e.g. `TODO-PF-3`.

## Completion Rules

- Mark an item complete by changing `[ ]` to `[x]`.
- A parent item should only be marked complete when all child items below it are complete.
- An Implementation Task should only be marked complete after:
  - The relevant code is implemented.
  - The project builds successfully.
  - Relevant tests are added or updated where practical.
  - All tests finish successfully
- If an agent discovers that an item is too large, it should split it into smaller child items before implementing it.

## Removal Rule

- Once a to-do item (`TODO-*-*`) is finished and its essence has been documented in `CHANGELOG.md`, its overview-table row
  is removed from `TODO.md`, and its detailed plan file `TODO_<item>.md` is deleted.

## Versioning Rules

The projects version follows semantic versioning (`MAJOR.MINOR.PATCH`; see `docs/release_process.md`). 

The current version is maintained in `CMakeLists.txt` (`project(VERSION ...)`) only.

All changes are documented in `CHANGELOG.md` on **MINOR** and **MAJOR** version changes. Changes that only raise the PATCH
version do not get their own entry, only if they trigger a new release.

## Cross-Referencing

- `TODO.md` is the canonical source for the open to-do items and their detailed plan references.
- Work Package, Implementation Task, and Action Item references always use the full category-prefixed number (e.g. `PF-1.1`,
  task `PF-1.1.1`), never a bare relative number.
- The `File` column of the overview table links each item to its plan file; while no plan file exists yet the cell holds
  `_none_`.
- When a to-do item is removed after its completion, all cross-references must be resolved and removed (e.g. replaced with a
  changelog reference or the now existing status or state).
