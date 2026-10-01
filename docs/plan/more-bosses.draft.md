# More bosses

Status: draft 2026-10-01. Split off [solved/boss-rooms.md](solved/boss-rooms.md).

Three bosses are in, one every 5 levels: the boss scarab (lvl5), the vampire bat (lvl10) and the Anubis boss (lvl15).
The boss framework (`BOSS_DEFS`, `Summon` kinds, boss gate, HUD bar, `levelcheck` rules) takes a new boss as a table
row plus a model or texture.

Open:

* Which other bosses, and where: more campaign levels, a mid-level boss between the current ones, or bosses in
  generated levels (`levelgen` places none so far).
* Candidates are open. Each should summon its own way, like the sand, ceiling and coffin summons.
