# Stronger poisons

Status: draft 2026-10-10 (not implemented). From the user.

## Idea

Poison hardly matters today: the player rarely needs to cure it. Make the medium and strong tiers scale with the
player's max HP, so poison is dangerous and the antidote is worth carrying.

| Tier | Today (`POISON_TIERS`, `src/world/poison.h`) | Planned |
|---|---|---|
| Weak (scorpion) | 1 HP/s, 20 s: 20 HP | no change |
| Medium (cobra) | 3 HP/s, 25 s: 75 HP | 1% of max HP per second, 60 s: 60% of max HP |
| Strong (scorpion queen) | 5 HP/s, 30 s: 150 HP | 5% of max HP per second, 19 s: 95% of max HP |

A strong poison left alone takes the player to 5% HP; with any other damage on top it kills.

## Sketch

* `PoisonTierDef` gets a percent-of-max-HP rate beside (or instead of) the flat `hpPerSecond`. `Poison::Advance`
  needs the max HP (`PlayerStats::CurrentMaxHP()`, amulet bonus included) and a hundredths carry, as
  `Monster::TrapHit` does, since 1% of a small max HP is under 1 HP a second.
* Max HP is read at each tick, so a max HP change mid-poison changes the rate.

## Open questions

* Monsters: `Monster::TakePoison` / `UpdatePoison` use the same `Poison` class (venom amulet, poison dart trap).
  Keep the flat HP for monsters, or scale them by their max HP too? 95% of a boss's HP from one strong poison is
  likely too much; flat for monsters is the safe default.
* Medium at low max HP: with the base 50 HP, 1%/s is 0.5 HP/s, 30 HP in all, weaker than today's 75. Fine (it grows
  with the player), or add a floor?
* Resistance potion / amulet: still a roll to resist the whole hit (`Player::Poison`), no change planned.

## Related

* [poison-and-antidote](solved/poison-and-antidote.md), [monster-poison](solved/monster-poison.md),
  [monster-balance](monster-balance.draft.md)
