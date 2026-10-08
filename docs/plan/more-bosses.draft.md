# More bosses

Status: draft 2026-10-01, cleaned up 2026-10-07. Split off [solved/boss-rooms.md](solved/boss-rooms.md).

Six bosses are in, one every 5 levels of the [30-level campaign](solved/longer-campaign.md): the boss scarab (lvl5), the
vampire bat (lvl10), the [scorpion queen](solved/scorpion-queen-boss.md) (lvl15), [Apep](apep-serpent-boss.md) (lvl20),
[Sobek](sobek-boss.md) (lvl25) and the Anubis boss (lvl30, the last boss). The boss framework (`MonsterKind::boss`, `Summon`
kinds, boss gate, HUD bar, `levelcheck` rules) takes a new boss as a table row plus a model or texture.

Every campaign boss slot is taken, so a new boss needs a new place.

## Open

* Where: a mid-level boss between the current ones, or bosses in generated levels (`levelgen` places none so far).
* Each should summon its own way, like the sand, ceiling, coffin and egg summons (`Summon`, `src/entities/monster.h`).

## Candidates

* Rat king: rats fused at the tails, reuses the rat model; rats tear loose at HP thresholds (a `Split` summon). Cheap
  boss for generated levels or a mid boss.
* Sphinx: tied to the riddles, a right answer weakens it. The Anubis boss stays the finale, so the sphinx would be a
  mid boss or a guardian of a riddle room.
