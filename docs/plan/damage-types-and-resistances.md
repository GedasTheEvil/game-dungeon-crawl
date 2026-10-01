# Damage types and monster resistances

Status: draft 2026-10-01. Done 2026-10-01, to verify in play (see [Implemented](#implemented)).

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
* [monster-journal.md](monster-journal.md): where the player learns the resistances.

## Implemented

Decided 2026-10-01 while implementing:

* Types: `src/world/damage.h`. Rates: weak 200%, normal 100%, resists 50%, tough 25%. A hit deals
  `sum(damage * share * rate)`, rounded, at least 1 (`resistedDamage`); traps stay untyped.
* Weapon mix (`ITEM_DEFS`, blunt / slash / pierce %): club 85 / 15 / 0, sword 0 / 85 / 15, spear 0 / 15 / 85,
  bow 0 / 0 / 100.
* Weapon damage and upgrades (changed 2026-10-01 after review: the sword has the most damage, its cost is the short
  reach; the club starts weak and grows the most):
  * Base damage: club 9 -> 10, spear 15 -> 20; sword 35 and bow 12 stay.
  * Growth per weapon level, of the base damage: club +40%, spear and bow +20%, sword +10% (`weaponGrowthPercent`,
    `src/world/item_bag.cpp`). Club level 5: 26 damage, x 2 against blunt-weak monsters.
  * Max weapon level 10 -> 5; copies needed 2, 4, 7, 11 (`upgradeCost`, was 2, 4, 8, ... 512).
  * Copies: before, the campaign had 3 weapon chests (spear lvl8, bow lvl9, sword lvl14) and no club copy, so no
    weapon was ever upgraded. Now: club chests in levels 2, 3, 4, 6, 7, 11; spear 10, 12; bow 11, 13; sword 15. And
    kills drop a copy of a weapon the player already has: 5% of the kills (not minions, not mimics), every boss
    (`RollKillDrop`, `src/world/loot.cpp`; the chest appears when the body has died away).
* Resistances (`RESISTANCE_DEFS` in `src/state/assets.cpp`), blunt / slash / pierce:

| Monster | Blunt | Slash | Pierce | Best |
|---|---|---|---|---|
| Worm | resists | weak | normal | sword |
| Scarab, giant, boss | normal | resists | weak | spear, bow |
| Plant | tough | weak | tough | sword only |
| Bat, giant, vampire | weak | resists | tough | club |
| Mimic | weak | resists | tough | club |
| Anubis | weak | resists | normal | club |
| Anubis boss | normal | resists | weak | spear, bow (from afar) |
| Mummy | resists | weak | tough | sword |
| Rat, giant rat | normal | normal | normal | sword |

  The finale switches weapons: the sword for the mummies, the spear or bow for the boss.
* Feedback: the first hit of a weapon's main type on a monster type writes it down in the journal (creature page:
  "blunt normal  slash resists  pierce ?") with a status line ("Journal: Giant scarab, weak to pierce"). The
  inventory names the weapon's main type ("Close combat, slash, level 1"); the Weapons field note explains it.
* Saved in the journal (version 4). Scenarios: `tests/scenarios/damage_types.txt`, `boss.txt` (the boss's chest,
  scenario field `chests`); `bow.txt` needs ten arrows for the plant now.
* Not done: a hit sound or number colour per rate; levelcheck threat does not look at resistances (every level
  keeps the club, so a weak spot is always at hand for the bats and mimics).
