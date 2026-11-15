# Level checker: sunbeams and light shafts are not light terrain

Status: draft 2026-11-15, size M, needs nothing first.

## Repro

1. A level with a light shaft whose beam (at level start, mirrors as placed) runs through a dark stretch, and a light
   shunner (bat, spider: `hostile` has `Terrain::Light`) spawned in the beam or within `LIGHT_SHUN_CELLS` of it.
2. `bin/levelcheck` on it.

## Expected

The checker and the generator see the same light as the game. In game, `Dungeon::TerrainsAt`
(`src/sim/dungeon_walkers.cpp`) uses `Dungeon::Lit`, which counts a cell as light when it or a cell within
`LIGHT_SHUN_CELLS` on its row is `Sunlit` (the shaft's own cell and every cell its beam runs through, `traceBeams`).
So the spawn above is hostile: the checker warns ("weak where it stands", `checkSpawnTerrains`), the threat score
uses the hostile factors, and `LevelBuilder::hostileCell` (`src/level/level_gen_content.cpp`) keeps generated
spawns out of it.

## Actual

`spawnTerrains` (`src/level/level_check.cpp`) counts only braziers, lamps and wall torches as light. A shaft or beam
cell with none of them is darkness: the shunner above is scored native (higher threat, `terrainThreatFactor`), with no
warning, and the generator may place one there.

## Fix notes

* In `spawnTerrains`, also count a cell as light when a cell within `LIGHT_SHUN_CELLS` on its row is sunlit at level
  start: `sunlitCells(grid.cells, blocksBeam..., [](int) { return false; })`, as `atStart` in
  `src/level/level_check_mechanisms.cpp`. Compute it once per level, not per call: pass it in (or a small struct
  with it) instead of tracing per spawn; `spawnTerrains` has three callers (two in `level_check.cpp`, one in
  `level_gen_content.cpp`).
* Only the start state of the mirrors: the player turning them later is play, not spawn placement. Say so in a
  comment.
* Gates in the beam: `blocksBeam` already stops it at a closed gate, same as the game at start.
* Then run `bin/levelcheck content/levels/lvl*`: a spawn the fix now flags is a level bug; fix the level (move the
  spawn or swap the kind), per AGENTS.md. List the touched levels in the commit.
* Glossary row "native terrain" or a "light" terrain row: note that light is lamps, braziers, torches and sunbeams.

## Fixed when

* A test that names the plan fails before the fix and passes after it: a unit test with a small grid, a light shaft
  and a bat spawn in its beam: `spawnTerrains` has `Terrain::Light`, `checkLevel` warns. And one beside the beam
  (`LIGHT_SHUN_CELLS` away) also lit; one past it dark.
* `bin/levelcheck content/levels/lvl*` passes with no warnings.

## Play-test

* None in game (checker only), unless a level was changed: walk the changed spawns, the monsters stand where the
  checker says and fight as expected (a shunner not in a beam).
