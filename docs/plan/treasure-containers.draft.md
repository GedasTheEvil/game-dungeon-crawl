# Treasure: chest orientations and containers

Status: draft 2026-10-10, ready (needs nothing first). From the user. Includes canopic jars from the lore review.

## Today

A treasure cell (`$`, `Treasure`) draws one chest model at a fixed offset and yaw, the item spinning on top
(`Dungeon::DrawTreasureTile`, `src/world/dungeon_render.cpp`). Interact to pick the item up.

## Idea

1. **Orientations**: the chest turns to vary the look: facing the camera, side-on, angled, pushed against the back
   wall or the side wall. Chosen per cell from the effects stream (never the gameplay stream), stable across loads.
   Must stay readable as a treasure from the play view.
2. **Containers**, the same treasure under another look, picked by depth like the decor (`decor-by-depth`):
   * wooden chest (today), a painted coffer with a gabled lid, a reed basket;
   * a set of **canopic jars** (four, the sons of Horus heads); the journal jokes about what was in them;
   * a large sealed pottery jar that smashes when opened (shards, a puff of dust);
   * an offering table with the item on it.
3. Opening plays a short clip or effect per container (lid lifts, jar breaks) before the item can be taken.

## Notes

* The mimic (Chest of Set) idles as a chest and must keep looking like one: it uses the chest kinds only, in any
  orientation the real chests use. Optional: a mimic jar.
* Level format unchanged if the look is derived (depth + effects roll); a level may force one with the tile's spare
  value if needed (implementer's choice).
* Models built procedurally (`tools/blender/models/items.py` / `props.py`, [docs/remodeling.md](../remodeling.md)).
* [tomb-curse-triggers](tomb-curse-triggers.draft.md) may hook into opening canopic jars and coffers.

## Tests

* Unit: the container and yaw per cell are stable across a save and load; the gameplay stream does not shift.
* Scenario with screenshots: each container kind, a few orientations, a mimic beside real chests.
