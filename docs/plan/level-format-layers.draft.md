# Level format v2: layers

Status: draft 2026-10-01, rewritten 2026-10-05 (was the binary level format). Needed by
[crocodiles-and-flooded-cells.draft.md](crocodiles-and-flooded-cells.draft.md).

## Why

A cell holds one thing (`Tile` in `src/world/level.h`: type, attr, value). Water must sit under other things: a
crocodile's spawn, the foot of a ladder, a key, a trap. So a cell gets two layers.

## Layers

* **Structure**: what the cell is made of. `Wall`, `Empty`, `HalfWater`, `DeepWater`. A type only.
* **Object**: at most one thing in the cell, with attr and value as today. Everything else goes here: ladder, door
  (entrance, exit, riddle, teleporter), gate, lever, key, treasure, ankh, monster spawn, spike, death trap, rock fall.

Rules:

* A cell blocks when its structure is `Wall` or `DeepWater`, or its object is a closed gate.
* An object needs a structure that is `Empty` or `HalfWater`.
* Which objects may stand in half water is up to `levelcheck` (ladder yes; spikes, death traps: open).
* Two objects in one cell (a monster on a treasure) stay impossible, as today.
* `Area3D` (type 7) is used by no level: drop it.

Later, if needed: hand-placed decoration (today only scattered at load) or a wall look per cell. The version number
in the header leaves room for a third layer or more fields.

## File

* A header with a magic and a version, then the structure layer, then the object layer.
* Text, not binary: it diffs well in git and can be fixed by hand. A binary format is an optional later step, only if
  `Dungeon::LoadCampaignLevel` turns out slow (measure first; the files are about 13 KB).
* The loader reads v2. Old files (v1: a cell count, then one `type attr value` line per cell) are read through the
  converter below, so old saves keep working.

## Converter

One mapping from v1 to v2, written once in `src/world/level.cpp` and used in two places:

* **A tool** (for example `tools/level/levelconvert`, built by `make` like `levelcheck`): converts `levels/lvl*` and
  the editor's `tools/editor/saved` files in place, once. Then every level must pass `./levelcheck levels/lvl*` with
  no warnings, as before.
* **The game's loader**: save games hold the whole cell map (`readLevelCells` in `src/world/dungeon_io.cpp`), so a v1
  save is converted when it loads. New saves are written as v2.

Mapping: `Wall` -> structure `Wall`, no object. `Empty` -> `Empty`, no object. Every other type -> structure `Empty`
plus the same type, attr and value as the object.

Check: convert, load both versions, compare cell by cell (a unit test), and play a level from an old save.

## Everything that reads or writes cells

* The game: `loadLevelFile` / `saveLevelFile`, `Dungeon` (map access, collision, drawing), save games.
* `levelcheck` (rules and `--map`), `levelgen`, `ascii2level.py` and the ASCII sources in `tools/level/campaign/`
  (glyphs for water, and a way to draw an object over water), the editor (`tools/editor`: pick the layer, show both),
  `tile_defs` / `tile_info`, the scenario tests.
* Docs: [../levels.md](../levels.md), [tools/editor/readme.md](../../tools/editor/readme.md).

## Open

* `ascii2level.py`: two drawings per level, or one drawing with water glyphs and a separate list for objects in water?
* The editor: a layer switch, or place objects and the structure under them in one click?
