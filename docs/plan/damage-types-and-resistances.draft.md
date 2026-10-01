# Damage types and monster resistances

Status: draft 2026-10-01.

## Idea

No single weapon should be the best against every monster. Each weapon deals a mix of damage types, each monster
type resists some types and is weak to others, so each monster has its own preferred weapon. The club stops being a
throwaway weapon you use on level 1 and forget.

## Damage types

Three types: **blunt**, **slash**, **pierce**. Each weapon deals a mix (shares of its damage):

| Weapon | Blunt | Slash | Pierce |
|---|---:|---:|---:|
| Club | most | a little | |
| Sword | | most | a little? |
| Spear | | a little | most |
| Bow (arrow) | | | all |

The sword was not in the original idea: slash as its main type is the obvious fit.

## Monster resistances

Per monster type, one multiplier per damage type (for example 0.5 resists, 1.0 normal, 1.5 weak). Example from the
idea:

* **Bats** (bat, giant bat, vampire bat): weak to blunt, resist slash and pierce. The club is the bat weapon.

Other monsters: open. Ideas to check when the work starts:

* Scarabs (giant, boss): hard shell, resist slash and pierce? Weak to blunt too would make the club too strong;
  maybe weak to pierce (the spear between the plates) instead.
* Mummy: wrappings, weak to slash?
* Plant: weak to slash, resists pierce and blunt.
* Mimic: wooden chest, resists pierce?
* Rats: normal to all.

Spread the weaknesses so each weapon (club, sword, spear, bow) is the best choice against at least one monster type.

## What

* Weapon damage mix in `ITEM_DEFS`, resistances in `MONSTER_DEFS` / `BOSS_DEFS` (`src/state/assets.cpp`).
* The hit applies `sum(damage * share * monster multiplier)`.
* Feedback in play: the player has to learn which weapon works. Maybe a different hit sound or number colour for
  "weak" and "resisted" hits, or a hint in the item / monster description.
* Weapon switching must be quick enough that changing weapons for a monster is worth it.
* Levelcheck threat (`monster_kinds.cpp`) may need the resistances if a level gives no weapon that works well.

## Related

* [weapon-ranges-and-balance.draft.md](weapon-ranges-and-balance.draft.md): weapon damage per second retune; do the
  resistances together with it (the club at 10 dps is too weak for any multiplier to save it).
* [monster-strength.draft.md](monster-strength.draft.md): monster HP/stat readjustment.
