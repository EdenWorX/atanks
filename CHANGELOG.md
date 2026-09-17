# Changelog

Releases use semantic versioning (`MAJOR.MINOR.PATCH`); all changes are documented here on `MINOR` and `MAJOR` version
changes. Finished to-do items from `TODO.md` are removed once their essence is documented here (see `docs/todo_planning.md`
for the planning rules and `docs/release_process.md` for the release process). Older history is preserved exactly as it was in
`docs/Changelog.history`.

## 6.7 (2023-08-09)

### Added

- The `Makefile` got some debugging targets for easier debug mode building.
- Random number generation has been modernized and was made thread local.
- Beautified land and moon creation.

### Changed

- Source code is now formatted uniformly using clang-format.
- Got rid of C90 style pointer juggling where possible (aka where performance is non-critical).
- Bot values have been lowered and the spread between worst and best AI player has been increased. The DEADLY player is also
  no longer immune to making errors.
- SDI will no longer shoot a missile down that is going to be repulsed.
- Some recursive functions have been rewritten to be non.recursive.
- Some "spaghetti-code" style functions have been split up.
- Many C++17 perks have been made use of where they make sense.
- Got rid of sscanf(). Everywhere.
- Decoupled many objects that deserved their own compilation unit.
- SDI repulsor calculation is now independent from AI level of its owner.
- Enhance armour/amp/weapon boosting, so that AI players no longer forget what they wanted to buy (but couldn't) between
  rounds.
- The maximum SDI range calculation has been rewritten, and the repulsor shields have been strengthened a bit.
- AI takes target level and already dealt damage ratios more into account. This stops the bots to get "stuck" on a (mainly
  human) opponent.
- AI does not longer know the exact coordinates of their opponent, but "drift" a bit according to their level.

### Fixed

- Fixed a bug that caused background updates to glitch.
- Added missing Theft Bomb missile image, that caused an assertion failure whenever someone tried to shoot one.
- Fixed all g++ and clang-tidy warnings.
- Fixed a bug where global.surface was being used as height instead of the surface y-coordinate.
- Fixed a bug in land creation that caused possible crashes.
- Fixed a bug that allowed AI players to have a ton of amps but no armour and vice versa.
- Remove C89 style TRUE/FALSE integers where possible.
- Fixed typo: Swapper costs 4 grand, not 40k.
- Fixed a bug that caused AI to ignore 90% of the items they might have wanted to save their money on.
- Fixed a bug that caused the AI to break off their attack planning early just to shoot a Riot Bomb anywhere.
- Fixed a bug that caused the AI to use the first guess on their last attack planning attempt without actually planning it
  through.
- Fixed a bug that could cause a segfault when fast-forward-play ends.
