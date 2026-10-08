# Poisoned monsters

Status: done 2026-10-07, to be play-tested with the [venom amulet](venom-amulet.md). Drafted 2026-10-07. Split off [venom-amulet](venom-amulet.md) and
[poison-dart-trap](solved/poison-dart-trap.md): both need it, so it is built once, first.

## Idea

Monsters can be poisoned, as the player can today ([poison-and-antidote](solved/poison-and-antidote.md)). Poison only
runs on the player so far (`PoisonTier`, `src/world/poison.h`).

Used by:

* [venom-amulet](venom-amulet.md): the player's hits have a chance to poison.
* [poison-dart-trap](solved/poison-dart-trap.md): the plate's arrows poison a heavy monster that sets it off (medium).

## Rules

* Poison works on a monster as on the player: the same three tiers (`PoisonTier`: weak, medium, strong), the same
  damage over time and length (`POISON_TIERS`, `src/world/poison.h`). The dart trap's arrows are medium.
* The one difference: monsters have a built-in poison resistance, a chance to shrug a poisoning off, like the player's
  poison warding amulet (`poisonResistPercent`). The player without an amulet has 0%: every poisoned hit poisons.
* A field in the monster row: `MonsterKind::poisonResistPercent` (`src/world/monster_kinds.cpp`), default 0.
* 100% (immune): the mummy, the Anubis guard, the Anubis boss, the plant, the mimic.
* A boss resists at least as well as its common kin (the Anubis boss as the guard, the scorpion queen as the
  scorpions, ...): never less. See [boss resistances](solved/boss-resistances.md).

## Done

* `MonsterKind::poisonResistPercent` (`src/world/monster_kinds.cpp`). A `static_assert` fails the build for a value
  outside 0-100 or a boss below its kin.

  | Resistance | Monsters |
  |---|---|
  | 100 (immune) | mummy, Anubis guard, Anubis boss, plant, mimic, egg cluster (a nest: nothing to carry it) |
  | 75 | scorpion queen, Apep (above their kin) |
  | 50 | scorpion, giant scorpion, cobra, giant cobra (used to venom) |
  | 0 | the rest, the other bosses as their kin |

* `Monster::TakePoison(tier, byPlayer, rng)`: rolls the resistance (no roll at 0, as the player's amulet), then
  `Poison::Apply`, the player's own timers. `Monster::UpdatePoison`, once a tick (`Dungeon::updateMonsterPoison`): the
  damage, armour or not, as `takeHit` (it wakes a lurker, splashes blood).
* Poison can kill, as on the player. The kill is the player's (XP, journal, kill drop) when any running tier came from
  them (`byPlayer`); a trap's poison (the dart trap) gives nothing, as a trap's kill.
* The tier numbers stay the player's: small next to a boss's HP (strong: 150 HP of Sobek's 1600). Retune with
  [monster-balance](monster-balance.draft.md) if needed.
* Shown: the monster's health bar has a lime outline and its fill pulses venom green to lime; the boss bar is green,
  like the player's (`PlayerHud::POISON_TOP`, shared).
* A respawn or a death clears the poison. Monsters are not saved, so neither is their poison.
* Scenario commands `poisonmonster`, `poisonboss`, field `nearest_poison` ([testing](../testing.md));
  `tests/scenarios/monster_poison.txt`.

## Left for later

* Journal: the poison resistance on the creature page: done with the [venom amulet](venom-amulet.md).
* Unit tests: `Poison` is tested already; the monster side waits for
  [sim-unit-tests-monster-rules](sim-unit-tests-monster-rules.draft.md).

## To test

* Once a source exists: a poisoned monster's bar is easy to tell from a healthy one; the pulse not too busy.
