# Level editor

`make editor`, then `make run-editor`. The editor runs from `DungeonEditor/`.

## Screen layout

* Left: the map grid, 40 columns x 47 rows. The bottom row of the grid is the bottom of the level (row 0).
  Above the grid: the column, row and contents of the cell under the mouse.
* Right, top: the `Paint` / `Check` mode buttons.
* Right, middle: the tile palette (two rows of 7 tiles), then the `Attribute` and `Value` fields.
* Right, papyrus: what the tile, attribute and value mean (the [Tile reference](#tile-reference) as text).
  Numbers that mean nothing to the game show in red.
* Right, bottom: the level name field and the `Save` and `Load` buttons.

## Modes

* `Paint` (key `P`): click or drag on the grid to write the brush (selected tile, attribute and value) into cells.
  The cell under the mouse previews the brush.
* `Check` (key `C`): clicking a cell does not change it. The papyrus explains the clicked cell.
  The level check (see [docs/levels.md](../docs/levels.md)) runs on the map: the path from the entrance to the
  goal is drawn with gold dots, the result shows under the grid.

In both modes, a right click on a cell picks its tile, attribute and value into the brush.

## Workflow

1. Click a tile in the palette.
2. If the tile needs one, set the attribute and value (see [Tile reference](#tile-reference)):
   click `Attribute` (or press `Tab`), type digits. `Tab` goes to the next field, `Enter` or `Esc` leaves it.
   The numbers apply as you type. Reset both fields to empty (0) before you paint plain tiles.
3. Paint on the grid.
4. Click the name field, type a name (max. 32 characters: letters, digits, `_`, `-`, `.`).
5. Click `Save` (`Ctrl+S`). The file goes to `DungeonEditor/Saved/<name>`. `Load` (`Ctrl+O`) reads the same path.
6. Copy the file to `Levels/lvlN`. The game starts on `Levels/lvl1` and each exit loads `lvl<N+1>`.

Limits: attribute max. 3 digits, value max. 4 digits, digits only.
A new map is all `Wall`. You carve the playable space out of it.

Tile icons are PNG files in `DungeonEditor/Textures/`, drawn by `DungeonEditor/Textures/make_icons.py` (Pillow). Fonts and backgrounds come from the game's `Fonts/` and
`Textures/ui/`.

## Tile reference

Palette order: top row `Wall`, `Empty`, `Door`, `Death`, `Monster`, `Spike`, `Ladder`; bottom row `3D`, `Treasure`, `Ankh`, `Key`, `Gate`, `Lever`, `RockFall` (type 0 to 13).

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
| 11 | Gate (lock gate) | Lock colour, see below | 0 closed, 1 open | Portcullis. A closed gate blocks the corridor. It opens when the player comes up to it with the key of its colour, or when a lever of its colour is pulled. It slides up in 1.2 s. |
| 12 | Lever | Lock colour, see below | 0 | Interact to pull it. Opens every gate of the same colour. |
| 13 | RockFall | - | 0 | Loose ceiling, walkable. When the player steps into the cell, grit trickles down and a rock falls after approx. 0.65 s + 0.3 s: 20 damage if the player is still under it. Sprint on, jump on or step back to get clear. The rock stays on the floor (walkable). Put a `Wall` above it. |

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
| 7 | Bat |
| 8 | Giant bat |

Bats hang on the ceiling of their cell until the player comes within 1.75 cells in the same row, then fly through him (a bite on the way), 1.5 cells on, turn and come back. They fly over traps and turn at walls. Any other value spawns a copy of the player model. Max. 9 monsters are active at one time.

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
| 3 | Potion | 0 small health (+25 HP), 1 large health (+50 HP), 2 strength (+2 might), 3 armor (+2 armor), 4 life (+5% max. HP, full heal), 5 small stamina (50% stamina), 6 large stamina (full stamina) |
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
