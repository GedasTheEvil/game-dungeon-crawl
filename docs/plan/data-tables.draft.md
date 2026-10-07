# Data tables: one record per kind

Status: draft 2026-10-07. Split off [architecture-review.draft.md](architecture-review.draft.md).

## Decision

* **Agreed:** one record per kind (e.g. `ItemDef`: name, short name, texts, file id, grade, colour, stats) in one
  table, instead of parallel arrays across files. Adding a kind then means one row. Same for monsters and the other
  per-kind tables.
* **Format: open.** JSON, C++ tables, or anything else; the implementer picks the better fit when doing it. Weigh
  the comparison below.
* **Timing:** after the queued content plans land (more bosses, Egyptian weapons, amulets, ...). Before starting,
  re-check the scatter list below against the code, then confirm with the user.

## Today

Already table-driven, as `constexpr std::array`s indexed by kind. One item's data is spread over several tables in
several files, though:

* `world/items.cpp`: `FILE_IDS` and `TEXTS`
* `world/loot.cpp`: `WEAPON_GRADES`
* `ui/inventory.cpp`: `POTION_COLORS`
* `world/monster_kinds.cpp`, `world/tile_defs.cpp`, `world/damage.h`, `world/poison.h`: the same pattern
* weapon stats and timers: somewhere else again

So the problem is mostly the **scatter**, not the format.

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
