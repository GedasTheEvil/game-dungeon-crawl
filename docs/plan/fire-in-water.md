# Fire in water

Status: draft 2026-10-19, from the user. Ready: nothing needed first.

## Idea

Who stands in water takes a tenth of a hit's fire damage. Player and monsters alike. Other damage types are
unchanged.

Today water only puts burning out (`Player::UpdateBurning`, `Monster` burn tick, plan fire-damage) and a burning
hit does not ignite a monster in water (`Monster::TakeWeaponHit`). The hit itself still deals its full fire share:
an Anubis fire bolt or a flaming arrow (plan skill-flaming-arrows) hurts a wading target as much as a dry one.

* Rule: in water, the fire share of a hit is multiplied by `WATER_FIRE_DAMAGE_PCT` = 10 (`gameplay_config.h`).
  Blunt, slash, pierce shares untouched. Applied after the resistances, before the at-least-1 rounding.
* "In water": the same test that douses burning (the monster's `InWater`, the player's `inWater` of
  `UpdateBurning`). The player's hit path (`playerHitDamage` via `player_stats.cpp`) does not know it yet: pass it
  in (or store it on the player each tick), whichever is smaller.
* Monsters: `Monster::TakeWeaponHit` (the player's weapons, arrows, and anything else with a `DamageMix`).
* Stacks with `ARROW_WATER_DAMAGE_PCT` (an arrow into a monster in water already loses half): intended, the water
  slows the arrow and drowns its flame.
* Burning is unchanged (water already puts it out, nothing ignites in water).
* Journal: the fire text in `src/ui/journal_view.cpp` ("water puts it out") gets a short addition, e.g. "and in the
  water a flame barely scorches".

## Tests

* Unit: `playerHitDamage` / `resistedDamage` with fire in water is a tenth of the fire share, the physical share
  unchanged (a mixed hit like the Anubis bolt: fire 100 : blunt 40 : pierce 20); a monster in water takes a tenth
  of a flaming arrow's fire share; out of water nothing changes.
