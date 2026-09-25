# TODO-PF-18: Bastion/Rogue team rename and recolor

Replace the team names "Jedi" and "Sith" (Star Wars bound) with "Bastion" (solid teamwork block) and "Rogue"
(individualistic, self-serving). Recolor the network client coding in the same pass: Bastion blue, Rogue red,
Neutral green, own client tank purple (see `src/client.cpp`). Enumerator *values* stay untouched so savegames keep
working; only labels and display strings change. Team names are proper nouns and stay identical in all translations.

Source: user discussion (no `TODO_Xtra.md` entry — decided directly). Status: see the overview table in `TODO.md`.

## [x] PF-18.1: Analysis, inventory every Jedi/Sith/color reference

Map all `Jedi`/`Sith` hits (code labels, UI strings, `text/*.txt` taunts across translations, docs) plus the four
`makecol` lines in `src/client.cpp` and the `README.md` network section.

Findings (2026-09-22 session, full `git grep` inventory):

- Enum labels (values frozen for savegames): `ETeamTypes` (`player_types.h:97`, SITH=0/NEUTRAL/JEDI) and `EWinner`
  (`globaltypes.h:258`, JEDI=104/SITH=105).
- Logic labels: `TEAM_*` across `aicore`/`atanks`/`client`/`environment`/`gameloop`/`optionitemplayer`/`player`/`shop`;
  `WINNER_*` in `environment`/`gameloop`; range checks (`player.cpp:2177`, `TEAM_JEDI` as max) stay valid.
- Locals: `jcnt`/`scnt`, `idx_jedi`/`idx_sith`, `all_jedi`/`all_sith`(+`_alive`), `jediMoney`/`jediCount`/`sithMoney`/
  `sithCount` — rename to bastion/rogue forms for consistency.
- Display strings: `get_team_name` (`player.cpp:1880/1886`, 8-byte buffer fits "Bastion" exactly), `"Jedi Win!"`
  /`"Sith Win!"` (`gameloop.cpp:1711/1713`), option lists (`optioncontent.h:1468-1485`, order is index-mapped to the
  enum — must stay positional), `text/ingame*.txt` taunts (7 files, proper nouns only), debug-log texts.
- Colors (all in `client.cpp:257-268` network join; local tanks use player-chosen colors): Bastion blue, Rogue red,
  Neutral green, own tank purple. Docs: `README.md:6,59`, `docs/README.md:9,146,343`.
- Russian option row uses transliterations (`Ситх`/`Джедай`) → `Бастион`/`Роуг`, needs a native-speaker review later.
- Scope decision goes to WP PF-18.2.

## [x] PF-18.2: Discussion

Names (Bastion/Rogue) and the color rotation are decided. Confirm scope: enum labels, all display strings including
translations (proper nouns, no translation needed), client recolor, docs.

Decision (2026-09-22 session): **full scope** — labels, strings, translations (RU transliterated `Бастион`/`Роуг`,
needs native-speaker review later), locals, recolor, docs. Enum values frozen (`ROGUE=0`, `BASTION=2`,
`WINNER_BASTION=104`, `WINNER_ROGUE=105`) so savegames keep working.

## [x] PF-18.3: Implementation, rename labels and strings

Rename `TEAM_JEDI`/`TEAM_SITH` labels (values unchanged), all display strings, and the `text/*.txt` taunts.

Implemented 2026-09-22: both enums relabeled, all logic labels, locals (`bcnt`/`rcnt`, `idx_*`, `all_*`,
`*Money`/`*Count`), display strings (`get_team_name`, win banners, option lists positional, 7 taunt files), debug
texts, comments; declaration groups realigned to the original grid (v23-only drift rejected).

## [x] PF-18.4: Implementation, recolor network client coding

Bastion blue, Rogue red, Neutral green, own tank purple in `src/client.cpp`; update the `README.md` network section.

Implemented 2026-09-22: four `makecol` lines rotated, both network color-coding paragraphs (`README.md`,
`docs/README.md`) updated.

## [x] PF-18.5: Tests

`make test` stays green; full-text search proves no `Jedi`/`Sith` remains outside history (`CHANGELOG.md`,
`docs/Changelog.history`).

Verified 2026-09-22: `make test` green; `git grep -i` clean across `src/`, `text/`, docs (history files excluded by
intent).

## [x] PF-18.6: Documentation

User docs (`README.md`) and technical docs (`docs/README.md`) updated for the new names and colors.

## [x] PF-18.7: Final testing and finalization

In-game validation (team selection, network colors), then close the item per `docs/todo_planning.md`.

Validated 2026-09-22: user tested in-game, everything solid. Item finished; removal from `docs/TODO.md` plus plan-file
deletion waits for the next release per the Removal Rule.
