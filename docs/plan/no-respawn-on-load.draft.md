# Dead monsters stay dead after a load

Status: draft 2026-10-09, refined 2026-10-09 (decided, not implemented). From the user: dead monsters don't respawn on level load.

## Today

* Monsters are not saved. A monster is a `MonsterSpawn` tile in the map; `Dungeon::spawnInView`
  (`src/world/dungeon_base.cpp`) spawns it when the tile comes into view, unless a monster from that tile is still
  active (`SpawnMonster`, `src/world/dungeon_monsters.cpp`).
* A kill leaves the tile in the map. Only the dead monster in its slot keeps the tile from spawning again.
* `Dungeon::LoadDump` (loading a save) calls `clearMonsters()`, and the saved map still has every spawn tile, so every
  monster killed on the level comes back. That is an XP farm (save, kill, load) and wrong.
* Exceptions already in place: a boss's tile turns into `slainBossObject` (`src/world/dungeon_boss.cpp`), so the boss
  stays dead. A killed mimic's tile turns into a treasure tile. The minions have no tile.
* Related, without a load: `freeMonsterSlot` reuses a dead monster's slot when no slot is free (`MAX_MONSTERS` 32, the
  busiest level has 15). The freed tile can then spawn its monster again when it comes back into view. That is rare
  today; [denser levels](denser-levels.draft.md) make it likelier.
* `MAX_MONSTERS` 32 has no design meaning (user, 2026-10-09). It grew from the original code's limit and can go up
  when needed, e.g. for denser levels.

## Idea

* On a kill (`rewardKill`), turn the monster's spawn tile into a "slain" marker, like the slain boss. The map is saved, so the kill is saved with it, and the slot
  reuse case is fixed too.
* A monster walks away from its tile, so the monster needs to remember its spawn cell (it has `Col()` / `Row()` of
  now; check whether the spawn cell is kept).
* No save format change: old saves keep their monsters (no record of past kills).

## Decided (2026-10-09)

* **When:** at the kill, not at the end of the death clip. A save during the clip already counts the kill.
* **Marker:** the spawn tile turns into a "slain" marker, like `slainBossObject`, not just cleared. The decor stays
  the same after a reload (the coffins of killed mummies stay), and the map and `levelcheck` can tell a kill from an
  empty cell. The marker is a map tile, so the save format stays the same.
* **Draft map:** killed monsters drop off the map, as the slain boss does.
* The slot reuse case (`freeMonsterSlot`) is fixed by the same marker.

## Open (for the implementer)

* The monster needs its spawn cell (it walks away from it): check whether it is kept.
* Anything that reads spawn tiles after the kill: `decor_scatter.cpp` (entombed monsters' coffins, the boss coffins),
  `map_view.cpp` (the mimic's symbol), `levelcheck`. Each must treat the marker right.
* XP: the curve ([monster-balance](monster-balance.draft.md#the-30-level-curve-2026-10-06)) already assumes no
  respawns, so nothing to retune. Players who farmed with loads will level more slowly.
* Tests: a scenario in `tests/scenarios/` (kill a monster, `savegame`, `loadgame`, the monster is not back; the
  same for walking away and back).
