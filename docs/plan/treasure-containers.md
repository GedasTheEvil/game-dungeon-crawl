# Treasure: chest orientations and containers

Status: implemented 2026-10-17, waiting for the user's play-test: `bin/game --play tests/play/treasure-containers.txt`
(every container and stance in a row, a mimic among the chests), or `bin/game --level 9` (worked-tunnel and tomb
mixes in the campaign). Draft 2026-10-10, from the user. Includes canopic jars from the lore review.

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
4. **Closed until opened** (the user, 2026-10-12): the item no longer spins on top of a closed container; it shows
   (spinning, as today) only once the container is open, then the player takes it. The player can no longer see the
   loot from afar: a trade for mystery, wanted for [trapped-chests](trapped-chests.draft.opus.md), whose trap fires
   in this clip.

## Notes

* The mimic (Chest of Set) idles as a chest and must keep looking like one: it uses the chest kinds only, in any
  orientation the real chests use. So every depth's mix keeps chests (wooden chest or coffer, at least about a third
  of the treasures): on a level of jars a lone chest would give the mimic away. Optional: a mimic jar.
* Level format unchanged if the look is derived (depth + effects roll). A `Treasure` tile has no spare field (its
  attribute and value are the item type and id): forcing a look per chest needs new data, see the level format note
  in [trapped-chests](trapped-chests.draft.opus.md).
* Models built procedurally (`tools/blender/models/items.py` / `props.py`, [docs/modeling.md](../modeling.md)).
* A trapped container's trap fires in its opening clip ([trapped-chests](trapped-chests.draft.opus.md)); a trapped
  canopic jar curses by default. Its rule covers [tomb-curse-triggers](tomb-curse-triggers.draft.opus.md) too.

## Tests

* Unit: the container and yaw per cell are stable across a save and load; the gameplay stream does not shift.
* Scenario with screenshots: each container kind, a few orientations, a mimic beside real chests.

## Done (2026-10-17)

* **Rules** in one table, `src/world/containers.h`: `Container` (chest, coffer, basket, canopic, jar, table),
  `CONTAINER_DEFS` (model, scale, clip length, item lift), `CONTAINER_PERCENT` (the shares per decoration tier: cave
  chests, baskets, jars; coffers and canopic jars from the worked tunnels, offering tables from the tombs; the wooden
  chest 40 % or more everywhere, the mimic's cover), `STANCES` (front, angled, side-on, at the back wall, against a
  side wall where there is rock).
* **The look per cell** (`scatterTreasures`, `src/world/decor_scatter.cpp`): rolled from the level's file name and
  the cell like the decorations, by depth (`decorTier`), for every open cell, so an emptied container keeps its look
  after a load. Not the effects stream as the draft said: a hash is stable over loads by itself, and the gameplay
  stream is untouched either way. Level format unchanged. Scenario `container COL KIND [STANCE]` forces a look.
* **Opening** (`Dungeon::openContainer`, `dungeon_interact.cpp`): the first press opens (Open), the clip plays
  (`CONTAINER_DEFS::openMs`, the prompt shows nothing), then the item shows, turning, and the next press takes it
  (Take). The emptied container stays, open. A kill's chest and a dead mimic's are plain wooden chests facing the
  camera, already open (`DROPPED_LOOK`): the mimic's die clip ends with the lid falling open, the player sees what a
  kill left. Saved per cell (`containers:` after the draft map in the save). Sounds: `tools/audio/container_sounds.py`
  (`WorldSound::LidWood`, `LidReed`, `LidStone`, `JarSmash`); the jar's dust is `Grit::burst`
  (`Dungeon::drawContainerEffects`).
* **Models**: the chest got its opening clip (`items.py`, `CHEST_FRAMES`, frame 0 closed, the last frame the old open
  chest the mimic copies); the coffer, basket, canopic jars, sealed jar and offering table in the new
  `tools/blender/models/containers.py`. The mimic idles closed now, like the closed chests (`mimic.py`, idle clip).
* **Canopic jars**: the first ones opened write a field note (`FieldNote::CanopicJars`), the joke about their
  contents.
* Tests: `tests/unit/containers_test.cpp` (tables, looks stable over loads and from the tier, side-wall stance only by
  rock, the press-clip-press order, the gameplay stream untouched, save and load, the canopic note);
  `tests/scenarios/containers.txt` (screenshots: each kind closed, opening and open, stances, the mimic among chests),
  level `tests/levels/containers`.

Left out (open for later): a mimic jar; turning the mimic to the chests' other stances (it faces the camera, as the
most common stance); the trap in the clip ([trapped-chests](trapped-chests.draft.opus.md)).
