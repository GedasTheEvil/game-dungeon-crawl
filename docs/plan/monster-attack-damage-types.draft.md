# Monster attack damage types

Status: draft 2026-10-06. Needed by the typed damage amulets ([amulets.draft.md](amulets.draft.md)).

## Problem

Since [solved/damage-types-and-resistances.md](solved/damage-types-and-resistances.md) monsters resist or are weak to
blunt / slash / pierce, but their own hits on the player are untyped: `Player::TakeHit(int dmg, ...)`, armour takes a
flat amount (`PlayerStats::HitDamage`). Nothing for a typed resistance to act on.

## Idea

* Each monster (and boss) gets a damage mix for its attack, like the weapons' mix in `ITEM_DEFS`: for example a bite
  pierce, a claw slash, a scarab ram blunt. The mix data next to `RESISTANCE_DEFS` / `MONSTER_DEFS`
  (`src/state/assets.cpp`).
* `TakeHit` takes the typed damage; the player's resistances (from the amulet) apply per type, then the armour.
* Traps stay untyped (or get a type of their own for the trap amulet).
* The journal shows what a creature deals, written down when it first hits the player.

## Open

* The mix per monster type and boss.
* Order: type resistance before or after the armour.
* Poison stays apart from the type (it ignores armour today).
