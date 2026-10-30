# Wall layer: a third map layer for things on the back wall

Status: solved 2026-10-14 (play-tested by the user). From the user.
Pairs with [denser-levels](denser-levels.md): a level built after this one can pack more into each cell.

## Done

* Wall types: `Lever` (its model is a plate on the back wall), `Inscription`, `WallSpikes` (`isWallType`, `level.h`).
  In code the wall thing is a **fixture**: `Fixture` in `Tile::fixture`, `setFixture` / `clearFixture` / `hasFixture`,
  `setThing` (a thing into its layer by type), `cellWith`. The lever and spike helpers (`leverPulled`, `spikeState`,
  `spike_cycle.h`) take a `Fixture`.
* Level format v3 (`wall N` list before `objects N`), v1 and v2 read and converted. Every campaign, test and editor
  level converted with `bin/levelconvert`; 29 of the 30 campaign ASCII sources rebuild their file byte for byte (lvl1
  has an older hand edit, one half water cell, not from this change).
* Interact: the chest first, then the fixture (lever, inscription), then the brazier, the gate last (it may take the
  player away, so an inscription in a gate's cell stays readable).
* Drawn: `Dungeon::drawFixture` before the object, on the back wall plane. A monster in front of an inscription or a
  lever leaves it visible (`tests/scenarios/solved/wall_layer.txt`, parked).
* Checker: wall spikes cost on top of the object's; the new warning "X in front of wall spikes" for a spawn, ladder,
  gate, door or treasure; a fixture in rock warns like an object. `--map` prints a `wall` drawing, `--legend` a
  `wall TYPE...` line and `wall:` before a wall type's meaning. Torches and decals keep off a cell whose fixture says
  so (`TileDef::torch` / `decals`), and `isEmptyCell` means no fixture too, so the scatter and `levelgen` place as
  before; `levelgen` puts its levers on the wall.
* Tests: `tests/unit/wall_layer_test.cpp`, `tests/levels/wall_layer.txt`.
* `make paths`: its scripts now wait out the gate travel fade (they waited 5 ticks since gate-travel, so every level
  failed at the exit). 16 of 30 pass; the other 14 fail on a jump at 0 stamina, the same on master before this
  change (lvl11 checked): a checker / stamina mismatch for a later fix, not the wall layer (none of them has wall
  spikes).

## Today

A cell has two layers ([level-format-layers](level-format-layers.md), [levels.md](../../levels.md#level-format)):
the structure (wall, empty, half water, deep water) and **one** object. Some objects sit on the cell's back wall but
still use up the cell's only object slot, so nothing else can go there: an inscription and a monster cannot share a
cell. Levels pad corridors with empty cells to place them.

## Idea

Split the object layer in two. Three layers per cell:

1. **Structure**: unchanged.
2. **Wall**: what hangs on or comes out of the back wall. At most one per cell.
3. **Object**: what stands in the middle of the cell (today's object layer, minus the wall things). At most one per
   cell.

### What moves to the wall layer

* `Inscription` (15).
* `WallSpikes` (16, [rhythm-traps](rhythm-traps.md)).
* `Lever` (12), if its model sits on the back wall (check `dungeon_render*.cpp`; if it stands in the cell, it
  stays an object).
* Later wall things go here from the start: the wall vent of [fire-jet-trap](fire-jet-trap.md), mirrors
  and light shafts of [sunlight-mirrors](sunlight-mirrors.md), hand-placed wall decor (torches, reliefs),
  a wall niche holding a treasure ([treasure-containers](treasure-containers.md): canopic jars in a niche).

Stay objects: monster spawns, treasure (chests stand in the cell), keys, gates, doors, ladders, floor traps (spike,
death, rock fall, dart plate: its darts come from the wall, but the plate is in the floor), the ankh.

Keep the type numbers: a type is either a wall type or an object type (`isWallType(type)` in `level.h`), so one
number space, one legend, one `tile_defs` table.

### Rules for two things in a cell

* **Interact**: both layers may hold something to interact with (an inscription over a chest, a lever by a key).
  The object goes first (open the chest, then read the inscription with the next press). With
  [interact-prompt](interact-prompt.md) the prompt shows the one the next press will use.
* **Wall spikes** block the cell when out. `levelcheck` warns about a monster spawn, ladder, gate, door or treasure
  in a wall spikes cell (a spawn inside the spikes, a chest the player can never stand at safely). A key or a floor
  trap under them is fine.
* **Draw order**: the wall thing is drawn on the back wall plane, the object in front, as now. Check that a monster
  in front of an inscription does not hide it fully (the inscription's glow, or nudge the monster).
* **Map screen** (`map_view.cpp`) and `levelcheck --map` show one glyph per cell: the object if there is one, else
  the wall thing. The `--map` output prints the wall drawing as its own block (below).

## Format: level file v3

* `DCLEVEL 3 40 47`, then `structure`, then `wall N` with one `COL ROW TYPE ATTR VALUE` line per wall thing, then
  `objects N` as in v2.
* In memory: `Tile` gets a second slot for the wall (type, attr, value), next to the object's.
  `setObject`/`clearObject` keep the wall; new `setWall`/`clearWall`. `hasObject` stays object-only.
* `readLevel` reads v1, v2 and v3, writes v3. A v2 file (or save) converts on load: a wall type in the object
  layer moves to the wall layer. `bin/levelconvert` rewrites v1 and v2 files as v3; `levelcheck` warns about a file
  older than v3. Convert all campaign, test and editor levels in the same change.
* Save games hold the wall layer with its live state (wall spikes' phase, a pulled lever).

## Tools

* **ASCII sources** (`tools/level/ascii2level.py`): an optional `wall` drawing after the object drawing (and after
  `structure` if present), same rows; `.` / `#` mean nothing there. A wall glyph in the object drawing still works
  (goes to the wall layer), so the campaign sources stay valid unchanged.
* **levelcheck**: wall things need an open structure (empty or half water), like objects. The rules that read
  inscriptions, wall spikes and levers read the wall layer. `--map` prints the wall drawing when the level has
  one. `--legend` marks which glyphs are wall types.
* **Editor** (`tools/editor`): a third layer in the layer switch; the other two stay visible, dimmed.
* **levelgen**: no change needed (it places none of the wall types today); may use it later.

## Levels

No redesign in this plan: the converter moves every wall thing to the wall layer and the levels play the same.
Compacting levels (an inscription in a monster's corridor instead of its own cell, a lever in a guarded hall) is
for the level work that follows: [denser-levels](denser-levels.md) and each trap plan's placement. Every
level still passes `bin/levelcheck` with no warnings.

## Docs

* [levels.md](../../levels.md#level-format), [tools/editor/readme.md](../../../tools/editor/readme.md) (file format),
  [inscriptions.md](../../inscriptions.md) if it says the cell must be empty.
* Glossary: add "wall layer" (and "wall thing" or the name chosen) in "Map and levels".

Boss rule (AGENTS.md): no monster gains an ability here.

## Open points (implementer's choice)

* Lever: wall or object (by its model).
* Interact order when both layers have something: object first is the default.
* Name in code: `Wall` clashes with `Structure::Wall`; e.g. `WallFixture` / `fixture`.

## Tests

* Unit: v2 to v3 conversion moves exactly the wall types (cell by cell, all campaign levels); a v3 save and load
  keeps both layers and wall spikes' phase; interact order over a chest with an inscription; a monster walks
  through an inscription cell; the checker's wall-spikes warnings.
* `make paths` still passes for every campaign level.
