# Checker: threat of reckless monsters

Status: solved 2026-11-08. Was: draft 2026-10-30, size M, needs nothing first. From the project audit of 2026-10-30.

Opus: checker modelling. Left from [trap-walking-monsters](trap-walking-monsters.md).

`monsterThreat` in `src/level/level_check.cpp` does not know that mummies, Anubis guards and most bosses walk
through traps. A trap between them and the player protects the player in the checker's model, not in the game.

* Raise the threat (or drop the trap's protection) for reckless monsters near traps, using the same
  courage / reckless data the game uses.
* Fix the levels it then flags (by hand, no regeneration).

## Done

* Found: `monsterReaches` ignored traps for every monster, so the reckless ones already counted fully; the cowards
  were counted through traps they never cross. Now a trap between the spawn and the player (`walkerTrapAt`, the
  game's own trap set from `walkerBlockedAt`) stops a coward walker only; reckless kinds, flyers and leapers reach
  through it. Test: `subcell_checker_test.cpp` ("a trap between shields the path ...").
* No campaign level gained a warning (`bin/levelcheck content/levels/lvl*`); only difficulty scores moved.
* Kind changed to tooling: the checker only, no gameplay.
