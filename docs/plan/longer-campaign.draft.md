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

Open:

* How many levels, and which boss where.
* Does the player's level curve need a cap or a slower XP rate for the longer run?
