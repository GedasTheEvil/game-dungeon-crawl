# Level editor

`make editor`, then `make run-editor`. The editor runs from `DungeonEditor/`.

## Screen layout

* Left: the map grid, 40 columns x 47 rows. The bottom row of the grid is the bottom of the level (row 0).
* Right, top: the tile palette (two rows of 5 tiles, one row of 4). The tile above the palette shows the selected tile.
* Right, middle: the `Atribute`, `Value` and `DungeonName` text fields.
* Right, bottom: the `Save` and `Load` buttons.

## Workflow

1. Click a tile in the palette.
2. If the tile needs one, set the attribute and value (see [Tile reference](#tile-reference)):
   click `Atribute`, type digits, press Enter. Do the same for `Value`.
   The numbers apply only after you press Enter. The active field is green.
3. Click or drag on the grid to paint. Each painted cell stores the selected tile and the current attribute and value.
   Reset both fields to empty (0) before you paint plain tiles.
4. Click `DungeonName`, type a name (max. 5 characters), press Enter.
5. Click `Save`. The file goes to `DungeonEditor/Saved/<name>`. `Load` reads the same path.
6. Copy the file to `Levels/lvlN`. The game starts on `Levels/lvl1` and each exit loads `lvl<N+1>`.

Limits: attribute max. 3 digits, value max. 4 digits, digits only.
A new map is all `Wall`. You carve the playable space out of it.

## Tile reference

Palette order: top row `Wall`, `Empty`, `Door`, `Death`, `Ankh`; middle row `Monster`, `Spike`, `Ladder`, `3D`, `Treasure`; bottom row `Key`, `Gate`, `Lever`, `RockFall`.

| Type | Tile | Attribute | Value | In game |
|---|---|---|---|---|
| 0 | Wall | - | - | Solid block. The player stands on it and cannot walk through it. |
| 1 | Empty | - | - | Open space. With no `Wall` below, the player falls. |
| 2 | Door (sphinx gate) | Gate type, see below | - | Sphinx statue. Behaviour depends on the gate type. |
| 3 | Death | - | - | Large spike trap (scale 40). Damages the player on contact. |
| 4 | Monster | Monster type, see below | - | Monster spawns on this cell when the cell is in view. |
| 5 | Spike | - | - | Small spike trap (scale 16). Damages the player on contact. |
| 6 | Ladder | - | - | Column. The player climbs up and down only between vertically adjacent ladder cells, and does not fall on it. |
| 7 | 3D (Area3D) | - | - | Not used by the game. Renders as open space. |
| 8 | Treasure | Item type, see below | Item id, see below | Chest with the item on top. Interact to pick it up. The cell then becomes `Empty`. |
| 9 | Ankh | - | - | Level goal. Interact with it to win the game. |
| 10 | Key | Lock colour, see below | - | Key on the floor. The player picks it up on touch. The cell then becomes `Empty`. |
| 11 | Gate (lock gate) | Lock colour, see below | 0 closed, 1 open | Portcullis. A closed gate blocks the corridor. The key or a lever of the same colour opens it. |
| 12 | Lever | Lock colour, see below | 0 | Interact to pull it. Opens every gate of the same colour. |
| 13 | RockFall | - | 0 | Loose ceiling, walkable. When the player steps into the cell, a rock falls after approx. 0.65 s + 0.3 s. Put a `Wall` above it. |

Trap damage starts at 1 and rises while the player stays in the trap. A short gap resets it.

### Door: gate type (attribute)

| Attribute | Gate | Behaviour |
|---|---|---|
| 1 | Entrance | Player start position. Shows a plasma portal. Use exactly one per level; if there are more, the last one in the file wins. |
| 2 | Exit | Plasma portal. Interact to load the next level (`Levels/lvl<N+1>`). |
| 3 | Riddle | Question mark above the sphinx. Interact to get a riddle; the gate then becomes type 4. |
| 4 | Empty gate | Decoration only (also the state of a used riddle gate). |
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

Any other value spawns a copy of the player model. Max. 9 monsters are active at one time.

### Key, Gate, Lever: lock colour (attribute)

| Attribute | Colour | Gem |
|---|---|---|
| 1 | Red | Carnelian |
| 2 | Blue | Lapis |
| 3 | Green | Turquoise |
| 4 | Gold | Amber |

Keys, gates and levers with the same colour belong together.

### Treasure: item type (attribute) and item id (value)

| Attribute | Item type | Value |
|---|---|---|
| 1 | Melee weapon | 0 club, 1 sword, 2 spear |
| 2 | Ranged weapon | 0 bow |
| 3 | Potion | 0 small health (+25 HP), 1 large health (+50 HP), 2 strength (+2 might), 3 armor (+2 armor), 4 life (+5% max. HP, full heal) |
| 0 | Empty chest | - |

Invalid ids write an error to the log and give nothing.

## File format

Plain text. The first line is the cell count, `1881` (40 x 47 + 1 spare cell).
Then one line per cell: `type attribute value`. Cells go row by row, from the bottom row up, left to right in each row (index = `row * 40 + column`).

```
1881
0 0 0
1 0 0
2 1 0
...
```
