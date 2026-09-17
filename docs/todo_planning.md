# EdenWorX To-Do Planning Rules

This document defines how to-do items are written, classified, numbered, and tracked in `TODO.md`. It is the canonical
reference for the planning rules;
`TODO.md` itself holds only the open to-do items and their detailed plans, or a list of `TODO_<feature>.md` files if the plan
list gets too extensive.

In the latter case these rules also apply to every `TODO_<feature>.md` file!

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
number (e.g. `WP PF-1.1`, task `PF-1.1.1`). Never write a bare relative number such as `WP 1.1`, which is ambiguous across
items of different categories.

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
  - Existing behavior is preserved.
  - The project builds successfully.
  - Relevant tests are added or updated where practical.
- If an agent discovers that an item is too large, it should split it into smaller child items before implementing it.

## Removal Rule

- Once a to-do item (`TODO-*-*`) is finished and its essence has been   documented in `CHANGELOG.md`, it is removed from
  `TODO.md`.

## Versioning Rules

The projects version follows semantic versioning (`MAJOR.MINOR.PATCH`; see `docs/release_process.md`). 

The current version is maintained in `CMakeLists.txt` (`project(VERSION ...)`) only.

All changes are documented in `CHANGELOG.md` on **MINOR** and **MAJOR** version changes. Changes that only raise the PATCH
version do not get their own entry, only if they trigger a new release.

## Cross-Referencing

- `TODO.md` is the canonical source for the open to-do items and their Work Packages and Tasks.
- Work Package, Implementation Task, and Action Item references always use the full category-prefixed number (e.g. `WP PF-1.1`,
  task `PF-1.1.1`), never a bare relative number.
