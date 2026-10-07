# Level editor

`make editor`, then `make run-editor`. The editor runs from the repo root.

## Screen layout

* Left: the map grid, 40 columns x 47 rows. The bottom row of the grid is the bottom of the level (row 0).
  Above the grid: the column, row and contents of the cell under the mouse.
* Right, top: the `Paint` / `Check` mode buttons and the `Structure` / `Objects` layer buttons.
* Right, middle: the palette of the layer (structures, or the object tiles in two rows), then the `Attribute` and
  `Value` fields (objects only).
* Right, papyrus: what the tile, attribute and value mean (the [Tile reference](#tile-reference) as text).
  Numbers that mean nothing to the game show in red.
* Right, bottom: the level name field and the `Save` and `Load` buttons.

## Modes

* `Paint` (key `P`): click or drag on the grid to write the brush (selected tile, attribute and value) into cells.
  The cell under the mouse previews the brush.
* `Check` (key `C`): clicking a cell does not change it. The papyrus explains the clicked cell.
  The level check (see [docs/levels.md](../docs/levels.md)) runs on the map: the path from the entrance to the
  goal is drawn with gold dots, the result shows under the grid.

In both modes, a right click on a cell picks it into the brush: its structure, or its tile, attribute and value.

## Layers

A cell has two layers (see [File format](#file-format)): its structure (wall, empty, half water, deep water) and at
most one object in it (a ladder, a monster, a key, ...). Key `L` or the `Structure` / `Objects` buttons switch the
layer the brush paints and the palette shows. The map shows both; the other layer stays visible, dimmed.

* Painting an object into a wall or deep water carves the cell open (`Empty`).
* Painting a wall or deep water over an object removes the object.
* `None` in the objects palette removes the object and keeps the structure.

## Workflow

1. Pick the layer, then click a structure or a tile in the palette.
2. If the tile needs one, set the attribute and value (see [Tile reference](#tile-reference)):
   click `Attribute` (or press `Tab`), type digits. `Tab` goes to the next field, `Enter` or `Esc` leaves it.
   The numbers apply as you type. Reset both fields to empty (0) before you paint plain tiles.
3. Paint on the grid.
4. Click the name field, type a name (max. 32 characters: letters, digits, `_`, `-`, `.`).
5. Click `Save` (`Ctrl+S`). The file goes to `tools/editor/saved/<name>`. `Load` (`Ctrl+O`) reads the same path.
6. Copy the file to `levels/lvlN`. The game starts on `levels/lvl1` and each exit loads `lvl<N+1>`.

Limits: attribute max. 3 digits, value max. 4 digits, digits only.
A new map is all `Wall`. You carve the playable space out of it: paint `Empty` structure, or paint objects (they
carve their cell).

Tile icons are PNG files in `tools/editor/icons/`, drawn by `tools/editor/icons/make_icons.py` (Pillow). Fonts and backgrounds come from the game's `fonts/` and
`textures/ui/`.

## Structure reference

| Glyph | Structure | Number | In game |
|---|---|---|---|
| `#` | Wall | 0 | Solid rock. The player stands on it and cannot walk through it. Holds no object. |
| `.` | Empty | 1 | Open space. With no wall below, the player falls. |
| `~` | Half water | 2 | Open space, half filled with water. Any object may stand in it. The player wades at half speed and cannot sprint or jump in it; put deep water or a wall under it ([docs/levels.md](../../docs/levels.md#level-format)). |
| `=` | Deep water | 3 | Full of water, solid like a wall. Holds no object. |

The glyph is the one in the level file's structure drawing and in `levelcheck --map`.

## Tile reference

The object layer. Palette order: `None`, `Door`, `Death`, `Monster`, `Spike`, `Ladder`, `Treasure`; then `Ankh`,
`Key`, `Gate`, `Lever`, `RockFall`. Types 0 (the wall, now a structure) and 7 (`Area3D`, unused) are gone.

| Type | Tile | Attribute | Value | In game |
|---|---|---|---|---|
| 1 | None | - | - | No object: the cell is just its structure. |
| 2 | Door (sphinx gate) | Gate type, see below | - | Sphinx statue. Behaviour depends on the gate type. |
| 3 | Death | - | - | Large spike trap (scale 40). Damages the player on contact. |
| 4 | Monster | Monster type, see below | - | Monster spawns on this cell when the cell is in view. |
| 5 | Spike | - | - | Small spike trap (scale 16). Damages the player on contact. |
| 6 | Ladder | - | - | Column. The player climbs up and down only between vertically adjacent ladder cells, and does not fall on it. |
| 8 | Treasure | Item type, see below | Item id, see below | Chest with the item on top. Interact to pick it up. The cell is then left without an object. |
| 9 | Ankh | - | - | Level goal. Interact with it to win the game. |
| 10 | Key | Lock colour, see below | - | Key on the floor. The player picks it up on touch. The cell is then left without an object. |
| 11 | Gate (lock gate) | Lock colour, see below | 0 closed, 1 open | Portcullis. A closed gate blocks the corridor. It opens when the player comes up to it with the key of its colour, or when a lever of its colour is pulled. It slides up in 1.2 s. |
| 12 | Lever | Lock colour, see below | 0 | Interact to pull it. Opens every gate of the same colour. |
| 13 | RockFall | - | 0 | Loose ceiling, walkable. When the player steps into the cell, grit trickles down and a rock falls after approx. 0.65 s + 0.3 s. Under its centre (0.3 cells) it crushes (1000 damage), nearer its edge (0.6 cells) it grazes (50 damage); armor does not help. Walk on without stopping, sprint on or step back to get clear; a jump in place does not dodge it. The rock stays on the floor (walkable). Put a wall above it. |

Trap damage starts at 1 and rises while the player stays in the trap. A short gap resets it.

### Door: gate type (attribute)

| Attribute | Gate | Behaviour |
|---|---|---|
| 1 | Entrance | Player start position. Shows a plasma portal. Use exactly one per level; if there are more, the last one in the file wins. |
| 2 | Exit | Plasma portal. Interact to load the next level (`levels/lvl<N+1>`). |
| 3 | Riddle | Question mark above the sphinx. Interact to get a riddle; the gate then becomes type 4. |
| 4 | Empty gate | Decoration only (also the state of a used riddle gate). |
| 5 | Teleporter | Two columns facing the camera, plasma between them (no sphinx). Interact to jump to the other teleporter with the same value (pair id); the player can go back the same way. Use exactly two per pair id. |
| 0 | - | Decoration only. |

### Monster: monster type (attribute)

| Attribute | Monster |
|---|---|
| 1 | Scarab |
| 2 | Worm |
| 3 | Plant |
| 4 | Anubis |
| 5 | Rat |
| 6 | Giant rat |
| 7 | Bat |
| 8 | Giant bat |
| 9 | Mimic |
| 10 | Giant scarab |
| 11 | Boss scarab (boss) |
| 12 | Vampire bat (boss) |
| 13 | Mummy |
| 14 | Anubis boss (boss) |
| 15 | Crocodile |
| 16 | Scorpion |
| 17 | Cobra |
| 18 | Giant cobra |
| 19 | Giant scorpion |
| 20 | Egg cluster |
| 21 | Scorpion queen (boss) |
| 22 | Apep (boss) |
| 23 | Sobek (boss) |

Bats hang on the ceiling of their cell until the player comes within 1.75 cells in the same row, then fly through the player (a bite on the way), 1.5 cells on, turn and come back. They fly over traps and turn at walls. A mimic looks like a treasure chest until the player comes within 1.5 cells, then bites; killed, it leaves a real chest with a random weapon or potion. A mummy lies in an open coffin on its cell until the player comes within 1.6 cells (or hits it), climbs out, then walks like the others. The giant rat and giant scarab leap over pits and traps up to 2 cells wide. A crocodile lies in the water (only its eyes and back show) until the player comes within 1.4 cells of its centre in the same row, then walks like the others; slow on land, in half water 2.5 times as fast (faster than the player walks). Put it in or next to water. A scorpion's sting poisons the player (weak poison: 1 HP a second for 20 s). A cobra lies coiled until the player comes within 1.8 cells in the same row, rears up, then walks like the others; from up to 2.5 cells it stops and spits venom at the player's chest (a jump dodges it). Its bite and its venom poison (medium: 3 HP a second for 25 s). The giant cobra does the same, bigger, from up to 3 cells. The giant scorpion's sting poisons (medium). Any other value spawns a copy of the player model. Max. 32 monsters are live at one time.

A boss (boss scarab, vampire bat, scorpion queen, Apep, Sobek, Anubis boss) summons minions next to itself, on the side away from the player: some when it
appears, then one every few seconds up to a limit. The boss scarab's scarabs dig out of the floor, the vampire bat's
bats drop from the ceiling. The vampire bat flies like a bat and heals by 30% of the HP its bites take. The Anubis
boss's mummies climb out of coffins: one stands on every empty floor cell within 3 cells of the boss's tile on its
row; a mummy rises from the free coffin nearest the boss, not beyond the player or next to them. The scorpion queen's giant scorpions hatch from the egg clusters (monster 20: rooted, harmless, killable) placed in her room; with none left she summons no more. Apep dives into the floor and comes up 1.6 cells behind the player; his cobras dig out. Sobek lies in the water, wakes from 4 cells, charges along the row (a jump clears it) and is stunned by a wall; his crocodiles come out beside him. Its HP shows in a bar at the bottom of the screen. Its death opens every boss
gate (Gate with lock colour 5); the boss does not come back, also not after loading a save. Minions give 1 XP while
the boss lives, half the normal XP after its death. Put at most one boss on a level, in a room only a teleporter
leads to (`levelcheck` warns otherwise).

### Key, Gate, Lever: lock colour (attribute)

| Attribute | Colour | Gem |
|---|---|---|
| 1 | Red | Carnelian |
| 2 | Blue | Lapis |
| 3 | Green | Turquoise |
| 4 | Gold | Amber |
| 5 | Boss (gates only) | Obsidian |

Keys, gates and levers with the same colour belong together. A boss gate (colour 5) has no key or lever: it opens
when the level's boss dies.

### Treasure: item type (attribute) and item id (value)

| Attribute | Item type | Value |
|---|---|---|
| 1 | Melee weapon | 0 club, 1 short sword, 2 spear |
| 2 | Ranged weapon | 0 self-bow, 1 composite bow, 2 sling, 3 throwing stick, 4 javelin |
| 3 | Potion | 0 small health (25% of max. HP), 1 large health (50% of max. HP), 2 might (+2 might), 3 armor (+2 armor), 4 life (+5% max. HP, full heal), 5 small stamina (50% stamina), 6 large stamina (full stamina), 7 antidote (cures all poison) |
| 0 | Empty chest | - |

Invalid ids write an error to the log and give nothing.

## File format

Level format v2, plain text (`src/world/level.h`). A header, the structure layer as a drawing, then the object layer
as a list:

```
DCLEVEL 2 40 47
structure
########################################
#......................................#
...                                       47 rows of 40 glyphs, the top row (46) first
objects 3
1 1 2 1 0                                 column row type attribute value
4 1 6 0 0
...
```

The structure glyphs are in the [Structure reference](#structure-reference). The object list holds only the cells
with an object, row 0 (the bottom) first, left to right. Rows count from the bottom, columns from the left.

The old format (v1: the cell count `1881`, then one `type attribute value` line per cell, type 0 a wall) still loads:
the game, the editor and `levelcheck` convert it as they read it (`levelcheck` warns). `levelconvert FILE...`
rewrites files in v2 for good.
