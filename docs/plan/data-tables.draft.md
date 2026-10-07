# Data tables: one record per kind

Status: draft 2026-10-07. Split off [architecture-review.md](solved/architecture-review.md).

## Decision

* **Agreed:** one record per kind (e.g. `ItemDef`: name, short name, texts, file id, grade, colour, stats) in one
  table, instead of parallel arrays across files. Adding a kind then means one row. Same for monsters and the other
  per-kind tables.
* **Format: open.** JSON, C++ tables, or anything else; the implementer picks the better fit when doing it. Weigh
  the comparison below.
* **Timing:** after the queued content plans land (more bosses, Egyptian weapons, amulets, ...). Before starting,
  re-check the scatter list below against the code, then confirm with the user.

## Progress

* **Format: C++ tables** (2026-10-07). No concrete need for data files yet (no translations, modding or tuning
  without rebuilds). Built with C++20 for designated initializers: a row names only the fields that differ from the
  record's defaults.
* **Monsters: done.** One `MonsterKind` row per type in `KINDS` (`world/monster_kinds.cpp`, level library): names,
  glyph, threat, stats, model, attack mix, resistances, wading, poison, spit, charge, boss summons (with the hatch
  nest), levelgen weights. `MonsterType` (the game's) is the row plus the loaded model. The mimic, mummy and crocodile
  checks read the locomotion; the decor scatter reads the boss table itself (no `CoffinBoss` callback).
* Items and tiles: open.

## Today (before the monsters were done)

Re-checked 2026-10-07 in the [architecture review](solved/architecture-review.md). One kind's data is spread over
several tables in several files:

* Monsters: `MONSTER_DEFS` (balance stats next to model paths), `BOSS_DEFS`, `RESISTANCE_DEFS`, `WADING_DEFS`,
  `POISON_DEFS`, `SPIT_DEFS`, `ATTACK_MIX_DEFS` in `state/assets.cpp` (lists of rows keyed by id, not arrays by
  kind); `KINDS` in `world/monster_kinds.cpp`; `PICKS` in `world/level_gen.cpp`. In code: `isPoisoner()` repeats
  `POISON_DEFS`, Sobek's `charges = true` (`assets.cpp:479`), the egg cluster check (`dungeon_boss.cpp:180`), other
  `== MonsterX` checks (mimic, crocodile, mummy).
* Items: `FILE_IDS`, `TEXTS`, `WEAPON_MIXES` in `world/items.cpp`; `WEAPON_DEFS` and `MISSILES` in `state/assets.cpp`;
  `WEAPON_GRADES` in `world/loot.cpp`; `POTION_COLORS` in `ui/inventory.cpp`; `MELEE` / `RANGED` / `WEIGHTS` in
  `world/level_gen.cpp`. Per-kind switches: `potionGain`, `weaponGrowthPercent` (`item_bag.cpp`), `potionNote`
  (`journal.cpp`), `missileOf` (`items.h`). Implicit orders: save slots (`OLD_SLOTS`), the HUD icon index, the mimic
  loot range ending at `Antidote`, hand-typed `ITEM_KIND_COUNT` / `WEAPON_KIND_COUNT`.
* Tiles: `TILES` (`world/tile_defs.cpp`) plus `tileGlyph`, the editor's `ICONS` and `describeCell`; the drawing
  switches (`dungeon_render.cpp`, `map_view.cpp`) are real behaviour and stay.
* `damage.h` and `poison.h` hold per-damage-type and per-tier constants, not per-kind tables.

Cost today: a monster variant 4-6 places in 3 files, a weapon 9-11, a potion ~11, a tile ~10.

So the problem is mostly the **scatter**, not the format. Moving the monster balance stats out of `assets.cpp` into the
level library also makes them unit-testable and helps [monster balance](monster-balance.draft.md).

## Format comparison

| | Tables in code | JSON / data files |
|---|---|---|
| Checks | compiler: table size must match kind count | own validation at load: missing fields, typos, bad numbers |
| Deps | none | parser lib (or hand-written; there is an INI parser for settings already) |
| Edit loop | rebuild | edit and restart, no rebuild |
| Behaviour | data and code side by side | data only; new behaviour still needs code, so a new kind touches both |
| Translations | hard | easy (one file per language) |
| Modding | no | yes |

* Data files pay off with a concrete need: translations, modding, or tuning balance without rebuilds
  ([monster-balance](monster-balance.draft.md)).
* If data files: texts first (pure data, no behaviour). Keep enums for kinds in code, load by string id. A check
  (level checker or new one) validates the files.
