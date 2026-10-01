# Boss coffins after a load

Status: solved 2026-10-01 (first option, verified by `tests/scenarios/anubis_coffins_load.txt`). Split off
[boss-rooms.md](boss-rooms.md).

## Problem

The Anubis boss's coffins are not in the level file. `Dungeon::bossCoffin` places one on every empty floor cell
within `MINION_SUMMON_REACH` (3) of the boss's tile on its row when the level loads (`scatterDecorations`). When the
boss dies, `Dungeon::updateBoss` rewrites his tile to `Empty`, so he does not come back. After loading a save made
after his death, the boss tile is gone and so are the coffins: the chamber is suddenly bare.

## Options

* Keep a marker on the dead boss's tile, for example `Tile{Empty, 0, value}` with a "boss was here" value, and let
  `bossCoffin` look for that too. Check that nothing else reads the `value` of an `Empty` tile.
* Put the coffins in the level: a decor tile or a cell flag the ASCII sources and the editor can set. More work, but
  the level author decides where they stand.

## Done

* The dead boss's spawn tile becomes `slainBossTile(type)` (`level.h`): `Tile{Empty, bossType, 0}`. Only save games
  have it. `bossCoffin` counts it like a live boss's spawn tile, and the tile itself gets no coffin or decoration,
  as in the boss's life.
* Nothing else read an `Empty` tile's `attr`. `PickUp` used to leave a chest's attr/value on its emptied cell; it now
  writes a clean `Tile{Empty, 0, 0}`, so no picked-up chest can pass for a slain boss.
* Scenario field `coffins` (coffin decorations on the level). The scenario kills the boss, saves, loads and expects
  6 coffins; with the old `Tile{Empty, 0, 0}` it gets 0.
* Saves made before this fix still load a bare chamber.
