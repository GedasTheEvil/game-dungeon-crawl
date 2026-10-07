# Poisoned monsters

Status: draft 2026-10-07. Split off [venom-amulet](venom-amulet.draft.md) and
[poison-dart-trap](poison-dart-trap.draft.md): both need it, so it is built once, first.

## Idea

Monsters can be poisoned, as the player can today ([poison-and-antidote](solved/poison-and-antidote.md)). Poison only
runs on the player so far (`PoisonTier`, `src/world/poison.h`).

Used by:

* [venom-amulet](venom-amulet.draft.md): the player's hits have a chance to poison.
* [poison-dart-trap](poison-dart-trap.draft.md): the plate's arrows poison a heavy monster that sets it off (medium).

## Rules

* Poison works on a monster as on the player: the same three tiers (`PoisonTier`: weak, medium, strong), the same
  damage over time and length (`POISON_TIERS`, `src/world/poison.h`). The dart trap's arrows are medium.
* The one difference: monsters have a built-in poison resistance, a chance to shrug a poisoning off, like the player's
  poison warding amulet (`poisonResistPercent`). The player without an amulet has 0%: every poisoned hit poisons.
* A field in the monster row: `MonsterKind::poisonResistPercent` (`src/world/monster_kinds.cpp`), default 0.
* 100% (immune): the mummy, the Anubis guard, the Anubis boss, the plant, the mimic.
* A boss resists at least as well as its common kin (the Anubis boss as the guard, the scorpion queen as the
  scorpions, ...): never less. See [boss resistances](monster-balance.draft.md#boss-resistances).

## Open

* The other types' resistance: the poisoners themselves (scorpions, cobras, the queen, Apep), the egg cluster.
* Can poison kill, or does it stop at 1 HP? XP and the kill drop when it kills.
* With the player's tier numbers a monster's poison is small next to its HP (a boss with 1000+ HP): fine as it is,
  or scaled later with [monster-balance](monster-balance.draft.md)?
* Shown how: a green tint or bubbles on the monster, its health bar, the boss bar.
* Journal: the poison resistance in the creature page, next to the damage resistances.
* Unit-testable in the level library, apart from the model ([sim-unit-tests-monster-rules](sim-unit-tests-monster-rules.draft.md)).
