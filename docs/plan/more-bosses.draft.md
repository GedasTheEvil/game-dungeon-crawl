# More bosses

Status: draft 2026-10-01. Split off [solved/boss-rooms.md](solved/boss-rooms.md).

Three bosses are in, one every 5 levels: the boss scarab (lvl5), the vampire bat (lvl10) and the Anubis boss (lvl15).
The boss framework (`BOSS_DEFS`, `Summon` kinds, boss gate, HUD bar, `levelcheck` rules) takes a new boss as a table
row plus a model or texture.

Open:

* Which other bosses, and where: more campaign levels, a mid-level boss between the current ones, or bosses in
  generated levels (`levelgen` places none so far).
* Each should summon its own way, like the sand, ceiling and coffin summons.

Picked (2026-10-05):

* [Scorpion queen](scorpion-queen-boss.draft.md): before the Anubis boss, scorpion minions from egg clusters, poison
  ([poison-and-antidote.md](solved/poison-and-antidote.md)).
* [Apep serpent](apep-serpent-boss.draft.md): dives between floor holes, cobra minions from baskets.
* Placement: [longer-campaign.draft.md](longer-campaign.draft.md).

Other candidates, not picked:

* Rat king: rats fused at the tails, reuses the rat model; rats tear loose at HP thresholds (a `Split` summon). Cheap
  boss for generated levels or a mid boss.
* Sobek (crocodile): charges in a line, stunned when it hits a wall.
* Sphinx: tied to the riddles, a right answer weakens it. A finale candidate.
