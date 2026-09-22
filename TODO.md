# TODO

Canonical planning rules: `docs/todo_planning.md`. Release rules: `docs/release_process.md`.

Numbering: `TODO-GI-*` (defects), `TODO-II-*` (design constraints), `TODO-PF-*` (planned features). Detailed plans live in
per-item files (`TODO_PF-<nr>.md`, `TODO_GI-<nr>.md`, `TODO_II-<nr>.md`); this file holds only the overview table plus one
row per item.

## Status

| Item  | Description                      | File                           | Status        |
|-------|----------------------------------|--------------------------------|---------------|
| PF-1  | Cleanup and Modernization        | _none_                         | **completed** |
| II-1  | Load-failure teardown segfault   | [TODO-II-1](./TODO_II-1.md)    | **completed** |
| II-2  | Missile null-pointer exposure    | [TODO-II-2](./TODO_II-2.md)    | not started   |
| GI-1  | tank.cpp cur_x/cur_y cleanup     | [TODO-GI-1](./TODO_GI-1.md)    | not started   |
| GI-2  | teleport.cpp condition review    | [TODO-GI-2](./TODO_GI-2.md)    | not started   |
| GI-3  | Redundant conditional assignment | [TODO-GI-3](./TODO_GI-3.md)    | not started   |
| GI-4  | shop.cpp finding review          | [TODO-GI-4](./TODO_GI-4.md)    | not started   |
| GI-5  | atanks.rc COPYING.txt reference  | [TODO-GI-5](./TODO_GI-5.md)    | not started   |
| GI-6  | optiontypes.h license typo       | [TODO-GI-6](./TODO_GI-6.md)    | not started   |
| GI-7  | Metainfo screenshot URLs         | [TODO-GI-7](./TODO_GI-7.md)    | not started   |
| GI-8  | README allegro-config claim      | [TODO-GI-8](./TODO_GI-8.md)    | not started   |
| PF-2  | Modern update checker            | [TODO-PF-2](./TODO_PF-2.md)    | not started   |
| PF-3  | atanks2 rename and Allegro move  | [TODO-PF-3](./TODO_PF-3.md)    | not started   |
| PF-4  | Full networking system           | [TODO-PF-4](./TODO_PF-4.md)    | not started   |
| PF-5  | Buy-screen scrollbar             | [TODO-PF-5](./TODO_PF-5.md)    | not started   |
| PF-6  | Buy-screen randomize button      | [TODO-PF-6](./TODO_PF-6.md)    | not started   |
| PF-7  | Field repair kit item            | [TODO-PF-7](./TODO_PF-7.md)    | not started   |
| PF-8  | Radar-resistant missile          | [TODO-PF-8](./TODO_PF-8.md)    | not started   |
| PF-9  | Semi-destructible rocks          | [TODO-PF-9](./TODO_PF-9.md)    | not started   |
| PF-10 | Underground mines                | [TODO-PF-10](./TODO_PF-10.md)  | not started   |
| PF-11 | Firework rockets                 | [TODO-PF-11](./TODO_PF-11.md)  | not started   |
| PF-12 | Shootable UFO                    | [TODO-PF-12](./TODO_PF-12.md)  | not started   |
| PF-13 | Scalable main window             | [TODO-PF-13](./TODO_PF-13.md)  | not started   |
| PF-14 | Harder ground option             | [TODO-PF-14](./TODO_PF-14.md)  | not started   |
| PF-15 | High-voltage missiles            | [TODO-PF-15](./TODO_PF-15.md)  | not started   |
| PF-16 | Tornadoes                        | [TODO-PF-16](./TODO_PF-16.md)  | not started   |
| PF-17 | Another armor level              | [TODO-PF-17](./TODO_PF-17.md)  | not started   |

PF-1 done 2026-09-21, released as 6.7.1: CMake build with wrapper `Makefile`, consolidated license/version truth, SourceForge to
GitHub move, normalized naming and guards, modernized config parsing, TOML arsenal data, frame-rate-independent physics.
Details in `CHANGELOG.md` (`## 6.7.1`); the git log records the individual steps.
