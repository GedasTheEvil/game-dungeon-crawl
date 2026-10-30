# Checker: threat of reckless monsters

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: checker modelling. Left from [trap-walking-monsters](solved/trap-walking-monsters.md).

`monsterThreat` in `src/world/level_check.cpp` does not know that mummies, Anubis guards and most bosses walk
through traps. A trap between them and the player protects the player in the checker's model, not in the game.

* Raise the threat (or drop the trap's protection) for reckless monsters near traps, using the same
  courage / reckless data the game uses.
* Fix the levels it then flags (by hand, no regeneration).
