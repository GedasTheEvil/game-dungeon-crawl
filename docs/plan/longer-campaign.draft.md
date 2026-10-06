# Longer campaign

Status: draft 2026-10-05.

New bosses ([scorpion-queen-boss.draft.md](scorpion-queen-boss.draft.md),
[apep-serpent-boss.draft.md](apep-serpent-boss.draft.md)) need more boss slots than lvl5 / 10 / 15.

* More campaign levels after lvl15 (`CAMPAIGN_LEVELS`, `src/world/campaign.h`), for example with `levelgen` as a start
  and hand-finished. Each must pass `./levelcheck` with no warnings.
* Keep one boss every 5 levels (proposal).
* The ankh moves to the last level, and the Anubis boss with it as the last boss. lvl15 becomes a normal boss level
  with a key behind its boss gate.
* New monsters (scorpion, cobra) get their place in the level table in [../levels.md](../levels.md).
* Then rebalance all monsters and bosses across the longer curve: player level and gear when reaching each boss, XP per
  level. See [monster-balance.draft.md](monster-balance.draft.md).

## Decided (2026-10-06)

* **30 levels**, a boss every 5. **The Anubis boss is the last boss, for good**: every new boss comes before him.
* Bosses: 5 boss scarab, 10 vampire bat, 15 [scorpion queen](scorpion-queen-boss.draft.md), 20
  [Apep](apep-serpent-boss.draft.md), 25 **Sobek** (picked by the agent while the user was away; the giant crocodile,
  from [more-bosses.draft.md](more-bosses.draft.md); easy to swap), 30 the Anubis boss and the ankh.
* The Anubis guards (`MonsterAnubis`) get stronger for the late levels: retuned with the curve below.
* New monsters: [cobra](cobra.md) (16-19 and Apep's minion), [giant cobra](giant-cobra.draft.md) (21-29), giant
  scorpion (the queen's minion).
* New levels use the whole grid: [denser-levels.draft.md](denser-levels.draft.md).
* The user is away; the agent works through it on its own, committing each step.

## Order of work

1. New levels 16-30, lvl15 reworked as a boss level, the finale moved to lvl30. Until a new boss exists, its level
   holds a stand-in boss from the old ones (not the Anubis boss).
2. Giant cobra.
3. Scorpion queen (and the giant scorpion), swapped into lvl15.
4. Apep, swapped into lvl20.
5. Sobek, swapped into lvl25.
6. Balance over the 30 levels: XP curve, the Anubis guards and boss, [monster-balance.draft.md](monster-balance.draft.md).

## Level plan

| Levels | Theme |
|---|---|
| 11-14 | as now: mummies, the first Anubis guards |
| 15 | The scorpion queen's nest: giant scorpions, scorpions, antidotes |
| 16-19 | The serpent temple: cobras, crocodile water, mummies, Anubis guards |
| 20 | Apep's pit |
| 21-24 | The flooded halls: giant cobras, crocodiles, giant bats, mummies |
| 25 | Sobek's lake |
| 26-29 | The necropolis: Anubis guards, mummies, giant cobras, giant scarabs, every lock |
| 30 | The ankh chamber: the Anubis boss |

Open:

* Does the player's level curve need a cap or a slower XP rate for the longer run?
