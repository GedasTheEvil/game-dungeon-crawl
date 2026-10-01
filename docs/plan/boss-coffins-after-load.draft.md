# Boss coffins after a load

Status: draft 2026-10-01. Split off [solved/boss-rooms.md](solved/boss-rooms.md).

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
