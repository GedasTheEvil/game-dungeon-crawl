# Sub-cells: monsters on steps and in crawl spaces

Status: solved 2026-11-04 (play-tested by the user). Was: implemented 2026-11-03. Was: draft 2026-11-02, needed [subcell-physics](subcell-physics.md) first.
From the user.

Implementer's choices:

* `MonsterKind::heightQuarters` (default 2) and `stepQuarters` (default 0). A height is at least the model's (a unit
  test measures them), more for a bulky one: only a kind no wider than a quarter tile each side may be 1. The ones
  that fit a crawl space: rat, scarab, scorpion, bat, mimic, cursed mimic, mimic jar, worm, egg cluster, lotus. Not
  the giant scarab, giant bat, crocodile, camel spider (low but wide). 3: Anubis boss, mummy priest, hippo, scorpion
  queen, Mother of Webs, Babi; 4: Apep, Seth.
* Steps: the leapers (giant rat, giant scarab, cursed mimic) 1; the leaping bosses (boss scarab, Babi) 2, half a tile
  (their kin, scarab and baboon, are walkers: 0). The open point is left out: a walker does not climb a quarter step.
* One rule for the body against a sub-cell column (`columnFits`, `world/climb_path.*`): a rise over its step, or an
  opening under a ceiling lower than its height, stops it. Cell by cell for the plans ahead (`shapePasses`: the
  climb path, `walkerReaches`, a pounce's way, a leap's landing), column by column at its front as it moves
  (`Dungeon::stepAt`: the walk, a coward's flight, a kiter's back-off room, the bite's follow-up step, a charge, a
  gust's push).
* A leaper at a step within its rise hops onto it (`Dungeon::hopOnto`: a leap that puts its front in the middle of
  the step's column; the leap's cooldown holds, so a stair is taken a hop at a time). A leap over a gap never passes
  a shape it does not fit: no leap over a half step.
* Standing: the highest solid sub-cell top under any part of its body, a little in from its sides, as the player's
  foot (`Dungeon::monsterFloor`). Its feet ease there at `MONSTER_STEP_EASE` a tick, up in a hop, down off a step (the
  fall); drawn, its hitbox and its shots' height go with it. On a step in half water it is out of the water (no wading,
  no sink), as the player.
* Flyers (`Dungeon::flightBand`): over shaped cells under the body and ahead, the flight's target lift is held in the
  band from the highest floor to the lowest ceiling (less its height), and under its body the lift itself; an opening
  ahead lower than its height is a wall. A bat flies low through a crawl space, a giant bat turns there.
* Burrowers: no dive comes up into a solid sub-cell at the floor (`Dungeon::solidFloorAt`), in sand too. Swimmers,
  figurines, rooted ones: no change. Ladders in shaped cells and the level checker: no change
  ([subcell-checker](../subcell-checker.draft.opus.md)).
* Tests: `tests/unit/subcell_monsters_test.cpp` on a new level, `tests/levels/subcell_monsters` (seven halls). The
  scenario `tests/scenarios/solved/subcell_monsters.txt` uses it too, not `tests/levels/subcells`: an Anubis in its dead end
  is hidden behind the crawl space's rock from the only side the player can stand on.

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
* Play script `tests/play/subcell-monsters.txt` (removed once solved).
