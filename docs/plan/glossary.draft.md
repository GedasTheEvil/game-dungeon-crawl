# Glossary

Status: draft 2026-10-09 (idea, not decided). From the user: a `docs/glossary.md` with the common terms and their
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

## Open

* Scope: all of the above, or only terms that were used two ways (level, range, gap, threat)?
* Order: alphabetical, or grouped as above?
* Keep it current: a doc test (`tests/unit/docs_test.cpp`) that every code name in the glossary still exists?
* UK or US spelling (armour vs `Armor` in the code)?
