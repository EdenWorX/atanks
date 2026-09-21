# Weapons and Items Data Format (TOML)

This document specifies the TOML format.

## Files and Localization

Numeric stats live in three base files (one per record kind, mirroring the legacy sections); display strings are
translated per language. This split is kept from the legacy format, where English is always loaded first and a second
pass overwrites only names and descriptions.

| File | Language | Contents |
|---|---|---|
| `text/weapons.toml` | English (base) | Weapon stats plus `name`/`desc` for every weapon |
| `text/naturals.toml` | English (base) | Natural stats plus `name`/`desc` for every natural |
| `text/items.toml` | English (base) | Item stats plus `name`/`desc` for every item |
| `text/weapons_fr.toml` | French | `name`/`desc` only |
| `text/weapons_de.toml` | German | `name`/`desc` only |
| `text/weapons_it.toml` | Italian | `name`/`desc` only |
| `text/weapons.pt_BR.toml` | Portuguese (Brazil) | `name`/`desc` only (note the dot, kept from the legacy name) |
| `text/weapons_ru.toml` | Russian | `name`/`desc` only |
| `text/weapons_sk.toml` | Slovak | `name`/`desc` only |
| `text/weapons_ES.toml` | Spanish | `name`/`desc` only |

All files must be UTF-8 (BOM tolerated). The legacy `weapons_ES.txt` and `weapons_it.txt` are ISO-8859 encoded and
must be transcoded to UTF-8 during migration. Numbers are locale-independent by definition in TOML, so the
`setlocale(LC_NUMERIC, "C")` workaround in the loader goes away. Lines starting with `#` are comments; blank lines
are free.

## Top-Level Structure

Each file holds three arrays of tables. Order is significant: array index maps to the in-code enum, exactly like
positional order does today.

| Array | File | Entries | Index maps to |
|---|---|---|---|
| `[[weapon]]` | `weapons.toml` | Exactly 56 | `EWeaponType` 0 (`SML_MIS`) to 55 (`LRG_LAZER`), see `src/weapon.h` |
| `[[natural]]` | `naturals.toml` | Exactly 6 | `EWeaponType` 56 (`SML_METEOR`) to 61 (`LRG_LIGHTNING`) |
| `[[item]]` | `items.toml` | Exactly 24 | `EItemType` 0 (`ITEM_TELEPORT`) to 23 (`ITEM_SDI`), see `src/item.h` |

Each base file must hold exactly its count, else loading fails with an error. Translation files hold the same
arrays in the same order but with only `name`/`desc` keys; they are applied by index. A short translation file
leaves the remaining entries in English (several legacy translations are incomplete, e.g. Spanish); entries past
the counts are ignored. Unknown keys and missing keys in the base file are load errors, so typos fail loudly
instead of silently corrupting data like a shifted positional column does today.

## Weapon and Natural Records

Weapons and naturals share one record layout (`CWeapon`, see `src/weapon.h`). Keys are snake_case, matching the
project naming convention. Integer fields must hold TOML integers; float fields accept `10` or `10.0`.

| Key | Type | Meaning |
|---|---|---|
| `name` | string, max 127 chars | Display name (translated per language) |
| `desc` | string, max 511 chars | Display description (translated per language) |
| `cost` | integer | Shop price in $ |
| `amt` | integer | Package amount when bought |
| `mass` | float | Projectile mass for physics |
| `drag` | float | Air drag for physics |
| `radius` | integer | Explosion radius |
| `sound` | integer | Sound index |
| `etime` | integer | Explosion frame time |
| `damage` | integer | Damage power |
| `picpoint` | integer | Flight bitmap selector |
| `spread` | integer | Weapons per shot |
| `delay` | integer | Volley delay (`0` disables volleys) |
| `noimpact` | integer | No impact detonation when nonzero |
| `tech_level` | integer | Shop tech level |
| `warhead` | integer | Warhead flag |
| `num_submunitions` | integer | Submunition count |
| `submunition` | integer | Next-stage weapon index |
| `impart_velocity` | float | Velocity imparted to submunitions, 0.0-1.0 |
| `divergence` | integer | Total submunition spread angle |
| `spread_variation` | float | 0 uniform, 1.0 random divergence around the centre |
| `launch_speed` | float | Speed given to submunitions |
| `speed_variation` | float | 0 uniform, 1.0 random speed around the centre |
| `countdown` | integer | Submunition countdown in frames |
| `count_variation` | float | 0 uniform, 1.0 random countdown around the centre |

Example:

```toml
[[weapon]]
name   = "Small Missile"
desc   = "Produces a low impact explosion"
cost   = 1500
amt    = 10
mass   = 10.0
drag   = 0.2
radius = 25
sound  = 0
etime  = 2
damage = 30
# ... remaining keys ...
```

## Item Records

Items fill `CItem` (see `src/item.h`): five mandatory scalar keys plus an effect-values array. The legacy format
grows the value list by total field count (5 mandatory, then extras); the TOML format states it directly.

| Key | Type | Meaning |
|---|---|---|
| `name` | string, max 127 chars | Display name (translated per language) |
| `desc` | string, max 511 chars | Display description (translated per language) |
| `cost` | integer | Shop price in $ |
| `amt` | integer | Package amount when bought |
| `selectable` | integer | Selectable in battle when nonzero |
| `tech_level` | integer | Shop tech level |
| `sound` | integer | Sound index |
| `vals` | float array, 0-6 entries | Effect values into `CItem::vals` (missing entries read as `0.0`; longer arrays are an error) |

Observed `vals` lengths in the shipped data, for orientation: 0 for the four teleporter/fan entries, 1 for amps,
armour and flight items, 2 for vengeance-type and utility entries, 6 for shields and repulsors. What each entry
means depends on the item type and lives with the consuming code (`shop.cpp`, `player.cpp`, `tank.cpp`); this spec
covers the mechanism, not per-item semantics.

Example:

```toml
[[item]]
name       = "Teleport"
desc       = "Teleports the tank to a random location"
cost       = 2000
amt        = 2
selectable = 1
tech_level = 3
sound      = 6
vals       = []
```

## Appendix: Legacy Positional Layout (Reference Only)

One record is three consecutive non-comment lines: name, description, whitespace-separated numbers. Sections are
`*WEAPONS*`, `*NATURALS*`, `*ITEMS*`; `#` lines and lines of 2 or fewer characters are skipped. Weapon/natural data
lines hold 23 numbers in the table order above (the weapons loader warns below 22 yet reads 23 values, the naturals
loader warns below 23). Item data lines hold
5 numbers, 6 (adds `vals[0]`), 7 (adds `vals[1]`), or 11 (adds `vals[2..5]`); 8-10 is an error. Non-English files
hold only name/description line pairs, applied by index. Counts are capped silently (`WEAPONS`/`NATURALS`/`ITEMS`).
