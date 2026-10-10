# Stronger poisons

Status: draft 2026-10-10, refined 2026-10-10 (decided, not implemented). From the user.

## Idea

Poison hardly matters today: the player rarely needs to cure it. Make the medium and strong tiers scale with the
player's max HP, so poison is dangerous and the antidote is worth carrying.

| Tier | Today (`POISON_TIERS`, `src/world/poison.h`) | Planned |
|---|---|---|
| Weak (scorpion) | 1 HP/s, 20 s: 20 HP | no change (1 HP/s) |
| Medium (cobra) | 3 HP/s, 25 s: 75 HP | 1% of max HP per second, at least 2 HP/s, 60 s |
| Strong (scorpion queen) | 5 HP/s, 30 s: 150 HP | 5% of max HP per second, at least 3 HP/s, 19 s |

The floors (weak 1, medium 2, strong 3 HP/s) keep poison biting at a low max HP: at the base 50 HP medium does
2 HP/s (120 HP in all, more than the max), strong 3 HP/s (57 HP). Above 200 max HP medium runs on its percent,
above 60 strong does.

Left alone, a strong poison takes the player to 5% HP (below 60 max HP the floor kills); any other damage on top kills.

## Sketch

* `PoisonTierDef` gets a percent-of-max-HP rate beside (or instead of) the flat `hpPerSecond`. `Poison::Advance`
  needs the max HP (`PlayerStats::CurrentMaxHP()`, amulet bonus included). A second's damage is
  `max(floor, maxHp * percent / 100)`, with a hundredths carry (as `Monster::TrapHit`) for the fraction above the
  floor.
* Max HP is read at each tick, so a max HP change mid-poison changes the rate.

## Decisions

* Monsters scale the same way (by their own max HP, same floors): `Monster::TakePoison` / `UpdatePoison` share the
  `Poison` class (venom amulet, poison dart trap). Bosses are no worry: most will get a high poison resistance
  (`poisonResistPercent`).
* Resistance potion / amulet: still a roll to resist the whole hit (`Player::Poison`), no change.

## Related

* [poison-and-antidote](solved/poison-and-antidote.md), [monster-poison](solved/monster-poison.md),
  [monster-balance](monster-balance.draft.md)
