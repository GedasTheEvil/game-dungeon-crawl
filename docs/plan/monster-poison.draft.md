# Poisoned monsters

Status: draft 2026-10-07. Split off [venom-amulet](venom-amulet.draft.md) and
[poison-dart-trap](poison-dart-trap.draft.md): both need it, so it is built once, first.

## Idea

Monsters can be poisoned, as the player can today ([poison-and-antidote](solved/poison-and-antidote.md)). Poison only
runs on the player so far (`PoisonTier`, `src/world/poison.h`).

Used by:

* [venom-amulet](venom-amulet.draft.md): the player's hits have a chance to poison.
* [poison-dart-trap](poison-dart-trap.draft.md): the plate's arrows poison a heavy monster that sets it off (medium).

## Open

* Damage over time per tier, how long, does it stack. The player's tiers (`POISON_TIERS`) as they are, or the
  monster's own (monsters have far more HP: a share of max HP per tick?).
* Immune monsters: the poisoners themselves, mummies, the egg cluster? A field in the monster row (`MonsterKind`,
  `src/world/monster_kinds.cpp`), e.g. a poison resistance like the damage resistances.
* Can poison kill, or does it stop at 1 HP? XP and the kill drop when it kills.
* Shown how: a green tint or bubbles on the monster, its health bar, the boss bar.
* Journal: a note on a creature's poison resistance in its creature page.
* Unit-testable in the level library, apart from the model ([sim-unit-tests-monster-rules](sim-unit-tests-monster-rules.draft.md)).
