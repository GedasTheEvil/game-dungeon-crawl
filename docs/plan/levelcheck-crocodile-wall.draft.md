# levelcheck: a crocodile is a wall in the water

Status: draft 2026-10-05. Split off [crocodiles-and-flooded-cells.md](solved/crocodiles-and-flooded-cells.md).

## Idea

In water the player is at half speed and the crocodile 25% faster, so the player cannot swim past one. The checker
says so: its walker treats a crocodile's spawn cell in half water as a wall. A path that needs to get past a crocodile
in the water is not a path; the level needs another way (dry ground, a ladder, a bridge above) or the crocodile goes.

## Open

* Only the spawn cell, or the cells around it too (it moves from there)? How far does its wall reach?
* On dry ground the crocodile is a normal monster (cost on the path, as today), not a wall.
* Killing it from dry ground with the bow opens the way in the game, but the checker cannot fight. Should a
  crocodile within bow range of dry ground count as passable?
* Error or warning: a level whose only way runs past a crocodile in water.
