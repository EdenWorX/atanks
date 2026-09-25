# Atomic Tanks

A turn-based artillery duel in the Scorched Earth / Worms tradition: drive your tank, buy weapons and defensive items
between rounds, and blow up the opposition. The last tank standing wins the round.

- Human players and AI bots (several difficulty levels), teams (Bastion / Rogue / Neutral)
- Destructible terrain, wind and weather
- Local play plus rough network play
- Localized in-game text

Current release: v6.7.1 — see [CHANGELOG.md](./CHANGELOG.md). Bug reports go to the
[issue board](https://github.com/EdenWorX/atanks/issues).

## Quick start

You need CMake 3.25+, ninja, a C++17 compiler, and the Allegro 4 development files (`allegro-config`; see
[liballeg.org](https://liballeg.org/)). Then:

```bash
git clone https://github.com/EdenWorX/atanks.git
cd atanks
make user
./cmake-build-release/atanks --windowed
```

Other build paths: `make` then `make install` for a system install (`PREFIX=` and `DESTDIR=` supported),
`make osxuser` on macOS, `make bsduser` on BSD, and Visual Studio 2026 with CMake support on Windows (see
`vs12`/`vs14/README_allegro.txt` for the Allegro paths). Full details live in the
[technical documentation](docs/README.md).

## How to play

When you first run the game with no human player, the player creation screen opens automatically: enter a name and
pick a tank colour. Afterwards choose **Play**, select 2–10 tanks (don't forget yourself!), and press **Okay**.

The buy screen lets you spend money on weapons and defensive items: left-click buys, right-click sells, **Done**
confirms. There is no perfect combination, so experiment a little.

In battle, aim with Left/Right, set power with Up/Down, switch weapons with TAB, and fire with Space. The round ends
when one tank is left standing.

Useful keys:

- `Space` — fire, confirm menu items
- `Enter` — confirm, like the OK button
- `Up`/`Down` — tank power, menu cycling, buy-screen scrolling
- `Left`/`Right` — aim, buy/sell, adjust values
- `Esc` — cancel out of a menu
- `F1` — screenshot
- `F10` — let the computer take over your tank (saves the game on the buy screen)
- `V` / `v` — volume up / down
- `~` — scoreboard (`#` on German keyboards)

Run `./cmake-build-release/atanks -h` for all command-line options.

## Network play

Network play works but is still rough. The host enables Networking under Options → Network and restarts; clients set
Server Address to the host IP and choose Network Game. Client tanks are color-coded: Bastion blue, Rogue red, Neutral
green, your own tank purple. The client side is buggy — see Known issues below.

## Known issues

- The network client side is buggy (unlimited shots and other client quirks); fixes are planned.

## Get involved

- Report bugs and ideas on the [issue board](https://github.com/EdenWorX/atanks/issues) with as many details as you can.
- Developers start with the [technical documentation](docs/README.md); the project history lives in
  [CHANGELOG.md](./CHANGELOG.md).
- License: GPL-3.0-or-later, see [LICENSE](./LICENSE). Authors and attributions are in `credits.txt`.
