# TODO

Canonical planning rules: `docs/todo_planning.md`. Release rules: `docs/release_process.md`.

Numbering: `TODO-GI-*` (defects), `TODO-II-*` (design constraints), `TODO-PF-*` (planned features). Detailed plans live in
per-item files (`TODO_PF-<nr>.md`, `TODO_GI-<nr>.md`, `TODO_II-<nr>.md`); this file holds only the overview table plus one
row per item.

## Status

| Item  | Description                      | File   | Status        |
|-------|----------------------------------|--------|---------------|
| PF-1  | Cleanup and Modernization        | _none_ | **completed** |
| II-1  | Load-failure teardown segfault   | _none_ | not planned   |
| II-2  | Missile null-pointer exposure    | _none_ | not planned   |
| GI-1  | tank.cpp cur_x/cur_y cleanup     | _none_ | not planned   |
| GI-2  | teleport.cpp condition review    | _none_ | not planned   |
| GI-3  | Redundant conditional assignment | _none_ | not planned   |
| GI-4  | shop.cpp finding review          | _none_ | not planned   |
| GI-5  | atanks.rc COPYING.txt reference  | _none_ | not planned   |
| GI-6  | optiontypes.h license typo       | _none_ | not planned   |
| GI-7  | Metainfo screenshot URLs         | _none_ | not planned   |
| GI-8  | README allegro-config claim      | _none_ | not planned   |
| PF-2  | Modern update checker            | _none_ | not planned   |
| PF-3  | atanks2 rename and Allegro move  | _none_ | not planned   |
| PF-4  | Full networking system           | _none_ | not planned   |
| PF-5  | Buy-screen scrollbar             | _none_ | not planned   |
| PF-6  | Buy-screen randomize button      | _none_ | not planned   |
| PF-7  | Field repair kit item            | _none_ | not planned   |
| PF-8  | Radar-resistant missile          | _none_ | not planned   |
| PF-9  | Semi-destructible rocks          | _none_ | not planned   |
| PF-10 | Underground mines                | _none_ | not planned   |
| PF-11 | Firework rockets                 | _none_ | not planned   |
| PF-12 | Shootable UFO                    | _none_ | not planned   |
| PF-13 | Scalable main window             | _none_ | not planned   |
| PF-14 | Harder ground option             | _none_ | not planned   |
| PF-15 | High-voltage missiles            | _none_ | not planned   |
| PF-16 | Tornadoes                        | _none_ | not planned   |
| PF-17 | Another armor level              | _none_ | not planned   |

PF-1 done 2026-09-21, released as 6.7.1: CMake build with wrapper `Makefile`, consolidated license/version truth, SourceForge to
GitHub move, normalized naming and guards, modernized config parsing, TOML arsenal data, frame-rate-independent physics.
Details in `CHANGELOG.md` (`## 6.7.1`); the git log records the individual steps.
