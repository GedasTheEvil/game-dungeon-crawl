# Glossary

Status: draft 2026-10-09, refined 2026-10-09 (decided, not implemented). From the user: a `docs/glossary.md` with the common terms and their
meanings, used in development and in the game, so everyone uses the same language when working.

## Idea

* One page, `docs/glossary.md`, linked from `AGENTS.md` and the docs index.
* A term, its meaning in one or two lines, and where it lives in the code (type, constant, file) if it has a home.
* Both sides: the in-game word the player sees and the code name, where they differ (e.g. the journal "note" and
  `Journal`, a "level" of the map and the player's level).

## Candidate terms (from the plans and the code)

* Map and levels: level / lvl (map level) vs player level, cell, tile, row, floor, gap, flooded cell, half water,
  deep water, ladder top, foot of a ladder, boss level, boss arena, slain marker.
* Monsters: kind (`MonsterKind`), kin, boss, minion, coward / reckless (`Courage`), climber, walker, walk-jumper,
  leaper, flyer, swimmer, burrower, entombed, coiled, lurking, alerted, threat (checker score), spit, hold.
* Combat: reach, range (weapons in tenths of a tile, spit in tiles), gap (between hitboxes), hitbox, bite reach,
  damage mix (blunt, slash, pierce), resistance, weakness, might, armour, poison tier.
* Items: tier names (lesser, minor, normal, grand), amulet, potion, quick-drink, bag.
* Tooling: scenario, unit test, sim (the sim library), checker (`levelcheck`), threat, gameplay stream vs effects
  stream, tick, draft / implemented / solved plan.

## Decided (2026-10-09)

* **Readers:** people reading the docs (the user, devs, testers) and agents. Not in the game (no journal page).
* **Scope:** the full list above. A term used two ways (level, range, gap, threat) says so in its row ("not the
  player's level").
* **Layout:** one file, grouped sections (map, monsters, combat, items, tooling) with an index at the top, A-Z within
  a section. One table row per term, so a grep returns the whole entry:

  | Term | In game | Meaning | Code |
  |---|---|---|---|
  | range | | how far a weapon or spit reaches, between the hitboxes; weapons in tenths of a tile, spit in tiles | `WeaponDef::range`, `SpitRules::range` |

  "In game" holds the word the player sees, empty for dev-only terms.
* **Agents:** one line in `AGENTS.md`: unsure of a term, or naming a new thing, grep `docs/glossary.md`; add new
  terms there. No need to read the whole file.
* **Spelling:** as the player sees it in the game; code names as they are in the code.
* **Kept current:** a check in `tests/unit/docs_test.cpp`: every code name in the Code column still exists in `src/`.
* Docs only (no change to the game): once done and checked, straight to `solved/`.
