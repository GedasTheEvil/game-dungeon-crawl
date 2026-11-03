# Sub-cells: monsters on steps and in crawl spaces

Status: draft 2026-11-02. Needs first: [subcell-physics](solved/subcell-physics.md). From the user.

Monsters move over steps and through crawl spaces by their size.

* Each `MonsterKind` (`src/world/monster_kinds.cpp`) gets a height in quarter tiles and a step rule. Small walkers
  (rats, scarabs) fit through crawl spaces; large ones (mummy, hippo, crocodile, Anubis) do not, so a crawl space is
  an escape route.
* Walk-jumpers and leapers jump onto a quarter step; plain walkers stop at it like at a wall (open point: let walkers
  climb a quarter step slowly, if the levels feel empty of chasers).
* Falls land on the floor height under the monster, as for the player.
* Flyers: a low ceiling lowers their flight band; a flyer taller than an opening does not pass.
* Swimmers, burrowers (the sand worm), figurines: no change unless the shape is in their cell (worm: does not come up
  under a solid sub-cell).
* Bosses (AGENTS.md, Boss abilities): a boss takes steps if its kin does, higher where it makes sense (a jump onto
  half a tile). A boss is usually larger than its kin; say in its kind whether it fits through crawl spaces.
* Monster paths (`Dungeon::followPath`, `ClimbMap`) treat a step and a crawl space by the kind's rule.

## Tests

* Unit: a rat passes a crawl space, a mummy stops; a leaper jumps a step, a walker stops; a boss jumps higher than
  its kin where set.
* Scenario on `tests/levels/subcells`: a large chaser stopped at a crawl space.
* Play script `tests/play/subcell-monsters.txt`.
