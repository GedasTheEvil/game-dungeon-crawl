# Level editor

`make editor`, then `make run-editor`. The editor runs from the repo root.

## Screen layout

* Left: the map grid, 40 columns x 47 rows. The bottom row of the grid is the bottom of the level (row 0).
  Above the grid: the column, row and contents of the cell under the mouse.
  Zoomed, the grid shows a window of the map and a minimap in its top right corner (see [Zoom](#zoom)).
* Right, top: the `Paint` / `Check` mode buttons and the `Structure` / `Wall` / `Objects` layer buttons.
* Right, middle: the palette of the layer (structures, the wall types, or the object tiles in two rows), then the
  `Attribute` and `Value` fields (fixtures and objects only).
* Right, papyrus: what the tile, attribute and value mean (the [Tile reference](#tile-reference) as text).
  Numbers that mean nothing to the game show in red.
* Right, bottom: the level name field and the `Save` and `Load` buttons.
* Bottom: a line with the mouse buttons and keys.

## Modes

* `Paint` (key `P`): click or drag on the grid to write the brush (selected tile, attribute and value) into cells.
  The cell under the mouse previews the brush.
* `Check` (key `C`): clicking a cell does not change it. The papyrus explains the clicked cell.
  The level check (see [docs/levels.md](../../docs/levels.md)) runs on the map: the path from the entrance to the
  goal is drawn with gold dots, the result shows under the grid.

In both modes, a right click on a cell picks it into the brush, in the layer being painted: its structure, or its
fixture's or object's tile, attribute and value. In `Check` mode with the `Wall` layer, the papyrus explains the
clicked cell's fixture.

## Zoom

The grid has three zoom levels, 1x (the whole map), 2x and 3x. The level stays 40 x 47: only the view changes, and it
is not saved.

* Zoom: `+` / `-`, or `1` / `2` / `3` (when no text field has focus); `Ctrl` + mouse wheel. It zooms around the cell
  under the mouse.
* Pan: mouse wheel (up / down), `Shift` + wheel (left / right), the arrow keys, or a middle button drag. The window
  stays inside the map.
* Minimap (zoomed only): the structure of the whole map in the top right corner of the grid, with a frame for the
  window. Click or drag in it to move the window. Cells under it cannot be painted.

Painting, picking, the check mode and the hover line work at every zoom.

## Layers

A cell has three layers (see [File format](#file-format)): its structure (wall, empty, half water, deep water), at
most one fixture on its back wall (a lever, an inscription, wall spikes: the wall layer) and at most one object in it
(a ladder, a monster, a key, ...). Key `L` (Structure, Wall, Objects, round) or the `Structure` / `Wall` / `Objects`
buttons switch the layer the brush paints and the palette shows. The map shows all three; the other layers stay
visible, dimmed, and the layer being painted is drawn on top.

* Painting an object or a fixture into a wall or deep water carves the cell open (`Empty`).
* Painting a wall or deep water over a cell removes its object and fixture.
* `None` in the wall or objects palette removes the fixture or the object and keeps the rest.

## Workflow

1. Pick the layer, then click a structure or a tile in the palette.
2. If the tile needs one, set the attribute and value (see [Tile reference](#tile-reference)):
   click `Attribute` (or press `Tab`), type digits. `Tab` goes to the next field, `Enter` or `Esc` leaves it.
   The numbers apply as you type. Reset both fields to empty (0) before you paint plain tiles.
3. Paint on the grid.
4. Click the name field, type a name (max. 32 characters: letters, digits, `_`, `-`, `.`).
5. Click `Save` (`Ctrl+S`). The file goes to `tools/editor/saved/<name>`. `Load` (`Ctrl+O`) reads the same path.
6. Copy the file to `content/levels/lvlN`. The game starts on `content/levels/lvl1` and each exit loads `lvl<N+1>`.

Limits: attribute max. 3 digits, value max. 4 digits, digits only.
A new map is all `Wall`. You carve the playable space out of it: paint `Empty` structure, or paint objects (they
carve their cell).

Tile icons are PNG files in `tools/editor/icons/`, drawn by `tools/editor/icons/make_icons.py` (Pillow). Fonts and backgrounds come from the game's `media/textures/fonts/` and
`media/textures/ui/`.

## Structure reference

| Glyph | Structure | Number | In game |
|---|---|---|---|
| `#` | Wall | 0 | Solid rock. The player stands on it and cannot walk through it. Holds no object. |
| `.` | Empty | 1 | Open space. With no wall below, the player falls. |
| `~` | Half water | 2 | Open space, half filled with water. Any object may stand in it. The player wades at half speed and cannot sprint or jump in it (the Favour of Hapi skill and the Blessing of Hapi potion allow both); put deep water or a wall under it ([docs/levels.md](../../docs/levels.md#level-format)). |
| `=` | Deep water | 3 | Full of water, solid like a wall. Holds no object. |
| `,` | Sand | 4 | Open space, a drift of sand on the floor (plan desert-chapter). Any object may stand in it. The player walks at 3/4 speed and cannot sprint in it; jumping is fine. A sand worm placed in it lies under the surface (a ripple) and comes up under the player within 2.5 cells on its row (at the sand's edge for a player beside it), later dives to come up under them again. Put a wall under it. |
| `%` | Quicksand | 5 | Looks like sand, with a slow swirl up close. Standing in it sinks the player: slower each second, stuck after 3 s, 5% of the max HP a second from 5 s (no armour); a jump frees them, for 40 stamina (twice a jump's). `levelcheck` treats it as a passable trap (a jump crosses one cell of it) and warns about runs of more than 2 cells. Put a wall under it. |

The glyph is the one in the level file's structure drawing and in `levelcheck --map`.

## Tile reference

The object and wall layers share the type numbers. The wall types (`Lever`, `Inscription`, `WallSpikes`) go in the
wall layer, the others in the object layer. Objects palette: `None`, `Door`, `Death`, `Monster`, `Spike`, `Ladder`,
`Treasure`; then `Ankh`, `Key`, `Gate`, `RockFall`, `DartPlate`. Wall palette: `None`, `Lever`, `Inscription`,
`WallSpikes`. Types 0 (the wall, now a structure) and 7 (`Area3D`, unused) are gone.

| Type | Tile | Attribute | Value | In game |
|---|---|---|---|---|
| 1 | None | - | - | No object: the cell is just its structure. |
| 2 | Door (sphinx gate) | Gate type, see below | - | Sphinx statue. Behaviour depends on the gate type. |
| 3 | Death | - | - | Large spike trap (scale 40). Damages the player on contact. |
| 4 | Monster | Monster type, see below | - | Monster spawns on this cell when the cell is in view. |
| 5 | Spike | - | - | Small spike trap (scale 16). Damages the player on contact. |
| 6 | Ladder | - | - | Column. The player climbs up and down only between vertically adjacent ladder cells, and does not fall on it. |
| 8 | Treasure | Item type, see below | Item id, see below | A treasure container (chest, coffer, basket, canopic jars, jar, offering table: rolled by depth) holding the item. Interact to open it, again to take the item. The cell is then left without an object. |
| 9 | Ankh | - | - | Level goal. Interact with it to win the game. |
| 10 | Key | Lock colour, see below | - | Key on the floor. The player picks it up on touch. The cell is then left without an object. |
| 11 | Gate (lock gate) | Lock colour, see below | 0 closed, 1 open | Portcullis. A closed gate blocks the corridor. It opens when the player comes up to it with the key of its colour, or when a lever of its colour is pulled. It slides up in 1.2 s. |
| 12 | Lever | Lock colour, see below | 0 | Interact to pull it. Opens every gate of the same colour. |
| 13 | RockFall | - | 0 | Loose ceiling, walkable. When the player steps into the cell, grit trickles down and a rock falls after approx. 0.65 s + 0.3 s. Under its centre (0.3 cells) it crushes (1000 damage), nearer its edge (0.6 cells) it grazes (50 damage); armor does not help. Walk on without stopping, sprint on or step back to get clear; a jump in place does not dodge it. The rock stays on the floor (walkable). Attribute 1 (`&` in the ASCII sources): a sand fall, sand pours from a crack instead (40 damage, no armour, and a 1.5 s slow to 40% for the player under it); the cell turns to sand. Put a wall above it. |
| 14 | DartPlate | - | 0 armed, 1 pressed | Pressure plate in the floor ([dart trap](../../docs/plan/solved/poison-dart-trap.md)). When the player or a monster of weight 2 and up (not rats, scarabs, scorpions, cobras or flyers) stands on the slab, it clicks and the 3 dart holes in the back wall above it shoot, 0.2 s later and 0.16 s apart, out across the corridor. At 0.3 cells over the floor (a jump at its top lets them pass, small monsters too) each takes the body in front of its hole: 2 pierce damage and medium poison. Jumping over the plate does not press it. It re-arms 3 s after the click, once nothing heavy stands on it. Cowards stop at its edge like at spikes. |
| 15 | Inscription | Text id, 1 to 64; 100 a name cartouche ([names](../../docs/names.md)) | - | Hieroglyphs on the back wall ([inscriptions](../../docs/inscriptions.md)). Interact to read them: the status line shows the title and the archaeologist's comment, the translation goes into the journal. The text id is an `id:` in `content/inscriptions/*.txt`. In the campaign from level 6 on. Put a floor under it. |
| 16 | WallSpikes | Speed: 0 normal, 1 fast, 2 slow | Phase, 0 to 7 (eighths of the cycle) | Rhythm trap ([plan](../../docs/plan/solved/rhythm-traps.md)): poison spikes shoot out of the back wall and go back in on a fixed cycle: out 1.2 s of every 4 s (fast: 0.8 of 3.2 s, slow: 1.6 of 4.8 s), a scrape and a twitch 0.6 s before they come out. Out, they block like a closed gate (no jump over them). Whoever stands in the cell as they come out takes one stab (10 damage, weak poison; the trap ward and the Dodge skill help) and is shoved out. The phase shifts a trap against the others: a rhythm. The checker warns when a run of them in a row cannot be walked through both ways; at normal speed a run of three never can. Put a floor under it and a wall above it. |
| 17 | LightShaft | Way: 0 down, 1 left, 2 up, 3 right | - | Sunlight through a crack in the rock ([sunlight mirrors](../../docs/plan/solved/sunlight-mirrors.md)): a beam runs from this cell the way it shines, through open cells, until rock, deep water or a gate not fully open stops it. Mirrors turn it; it burns mummies standing in it and keeps camel spiders out like a torch. Put rock behind it (above it for a beam down; the checker warns). |
| 18 | Mirror | - | Way it points: 0 down, 1 left, 2 up, 3 right | A bronze mirror on a stand: a beam that falls on it goes on the way it points (the gold pointer on its hub); pointing the way the beam goes it stands edge-on and lets it by, pointing back at it, it catches it. Interact turns it a quarter turn clockwise. Not in front of an inscription (reading it takes the key; the checker warns). |
| 19 | SunDisc | Lock colour, see below | 0 | A sun disc (the Aten) on the back wall. A beam on it lights it for good and opens every gate of its colour, as a lever. The checker warns when no beam can ever reach it, and when one lights it as the level starts. |
| 20 | Page | Page number, 1 to 18, in order of depth over the campaign | 0 (1 once taken) | A torn page of the earlier expedition's notebook ([expedition notes](../../docs/plan/expedition-notes.md)), floating over a skeleton (the scatter puts one there). Picked up on touch, it goes into the journal's Expedition section; the skeleton stays. Optional: keep it off the exit path, but for the last page (18), which lies on it. The checker warns about one out of reach or without a number. |

Trap damage starts at 1 and rises while the player stays in the trap. A short gap resets it.

### Door: gate type (attribute)

| Attribute | Gate | Behaviour |
|---|---|---|
| 1 | Entrance | Player start position. Shows a plasma portal. Use exactly one per level; if there are more, the last one in the file wins. |
| 2 | Exit | Plasma portal. Interact to load the next level (`content/levels/lvl<N+1>`). |
| 3 | Riddle | Question mark above the sphinx. Interact to get a riddle; the gate then becomes type 4. |
| 4 | Empty gate | Decoration only (also the state of a used riddle gate). `levelcheck` warns about one in a level file. |
| 5 | Teleporter | Two columns facing the camera, plasma between them (no sphinx). Interact to jump to the other teleporter with the same value (pair id); the player can go back the same way. Use exactly two per pair id. A value of pair id + 256 makes a name gate: sealed by the level's royal name ([names](../../docs/names.md)). |
| 0 | - | Decoration only. `levelcheck` warns. |

### Monster: monster type (attribute)

| Attribute | Monster |
|---|---|
| 1 | Scarab |
| 2 | Worm (game name: Sand worm) |
| 3 | Plant (game name: Thorn acacia) |
| 4 | Anubis (game name: Anubis living statue) |
| 5 | Rat |
| 6 | Giant rat |
| 7 | Bat |
| 8 | Giant bat |
| 9 | Mimic (game name: Chest of Set) |
| 10 | Giant scarab |
| 11 | Boss scarab (boss; game name: Khepri) |
| 12 | Vampire bat (boss; game name: Bat of Shezmu) |
| 13 | Mummy |
| 14 | Anubis boss (boss; game name: Anubis) |
| 15 | Crocodile |
| 16 | Scorpion |
| 17 | Cobra |
| 18 | Giant cobra |
| 19 | Giant scorpion |
| 20 | Egg cluster |
| 21 | Scorpion queen (boss) |
| 22 | Apep (boss) |
| 23 | Sobek (boss) |
| 24 | Lotus (game name: Blue lotus) |
| 25 | Mummy priest (boss; game name: High Priest of Anubis) |
| 26 | Camel spider |
| 27 | Hippo (game name: Hippopotamus) |
| 28 | Ushabti (game name: Living ushabti) |
| 29 | Cursed mimic (game name: Cursed chest of Set) |
| 30 | Seth (boss) |

Bats hang on the ceiling of their cell until the player comes within 1.75 cells in the same row, then fly through the player (a bite on the way), 1.5 cells on, turn and come back. They fly over traps and turn at walls. A mimic looks like a treasure chest until the player comes within 1.5 cells, then bites; killed, it leaves a real chest with a random weapon or potion. A mummy lies in an open coffin on its cell until the player comes within 1.6 cells (or hits it), climbs out, then walks like the others. The giant rat and giant scarab leap over pits and traps up to 2 cells wide. A crocodile lies in the water (only its eyes and back show) until the player comes within 1.4 cells of its centre in the same row, then walks like the others; slow on land, in half water 2.5 times as fast (faster than the player walks). Put it in or next to water. A scorpion's sting poisons the player (weak poison: 1 HP a second for 20 s). A cobra lies coiled until the player comes within 1.8 cells in the same row, rears up, then walks like the others; from up to 2.5 cells it stops and spits venom at the player's chest (a jump dodges it). Its bite and its venom poison (medium: 3 HP a second for 25 s). The giant cobra does the same, bigger, from up to 3 cells. The giant scorpion's sting poisons (medium). A blue lotus floats on the water like a flower (only a twitch now and then gives it away) until the player comes within 1.2 cells of its centre in the same row, rears up, then bites where it stands; each bite drains a quarter of the player's stamina. Put it in half water. A camel spider runs faster than the player but does not step into the light: a cell within 1 cell along the row of a wall torch, a brazier or an oil lamp. Light between it and the player keeps it off (it runs out of the bow's range, like a coward that cannot reach the player); placed in the light, it runs for the nearest dark. A hippo lies in the water like the crocodile (only its eyes, ears and nostrils show) until the player comes within 4 cells of its centre in the same row, then gapes and charges along the row from 2 to 4 cells away (3.2 cells a second, through the player for double damage, on into a wall that stuns it for 2.5 s); between charges it walks, slowly on land, three times as fast in half water. Put it in or next to water, best in a long pool. A living ushabti stands on its cell among the decor ushabti (the game puts them there) as one of them, its arms crossed (a turn of its head now and then gives it away), until the player comes within 1.3 cells of its centre in the same row (or hits it), steps off, then walks slowly like the others. Glazed faience: a club breaks it at double damage, an arrow skids off. A cursed mimic is a darker chest (purple where the treasure chest is blue) that lies in wait like the mimic, then hops after the player (a bit slower than they walk) and leaps pits and traps like the giant rat; a quarter of its bites lay insanity. Killed, it leaves its chest where it died: half the time a lesser or minor amulet, else the mimic's loot. A monster bites a moment after it opens its jaws; a quick jump over a low one dodges the bite. The Anubis shoots a bolt of fire from its sceptre from up to 1.4 cells (the Anubis boss from up to 2); it sets the player burning. In half water a crocodile's bite may hold the player for a death roll; the giant scorpion's pincers may hold the player for the sting (a hit on it ends the hold sooner). Climbers follow the player up and down ladders: rats, giant rats, scorpions, giant scorpions, mummies, the Anubis and their bosses. Any other value spawns a copy of the player model. Max. 32 monsters are live at one time.

A boss (boss scarab, vampire bat, scorpion queen, Apep, Sobek, mummy priest, Anubis boss, Seth) summons minions next to itself, on the side away from the player: some when it
appears, then one every few seconds up to a limit. The boss scarab's scarabs dig out of the floor, the vampire bat's
bats drop from the ceiling. The vampire bat flies like a bat and heals by 30% of the HP its bites take. The mummy
priest lies in a large coffin like a mummy, swings a staff, shoots a purple bolt from up to 1.8 cells and lays many
curses; his mummies climb out of coffins: one stands on every empty floor cell within 3 cells of the boss's tile on its
row; a mummy rises from the free coffin nearest the boss, not beyond the player or next to them. The Anubis boss's
guards step out of the wall the player looks at (the end of their row), onto the free cell in front of it; every other one is a living ushabti. The scorpion queen's giant scorpions hatch from the egg clusters (monster 20: rooted, harmless, killable) placed in her room; with none left she summons no more. Apep dives into the floor and comes up 1.6 cells behind the player; his cobras dig out. Sobek lies in the water, wakes from 4 cells, charges along the row (a jump clears it) and is stunned by a wall; his crocodiles come out beside him. Seth, the red hippo, does the same, charging from 2.5 to 7 cells at 4.5 cells a second; as a stun ends he roars a sandstorm: within 2 cells of him on his row the player is hit, shoved 1.5 cells away and blinded by sand for a second. His hippos come out beside him. Put him in or next to water, in a long pool. Its HP shows in a bar at the bottom of the screen. Its death opens every boss
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
| 1 | Melee weapon | 0 club, 1 short sword, 2 spear, 3 dagger, 4 khopesh, 5 epsilon axe, 6 duckbill axe, 7 mace |
| 2 | Ranged weapon | 0 self-bow, 1 composite bow, 2 sling, 3 throwing stick, 4 javelin |
| 3 | Potion | 0 small health (25% of max. HP), 1 large health (50% of max. HP), 2 might (+2 might), 3 armor (+2 armor), 4 life (+5% max. HP, full heal), 5 small stamina (50% stamina), 6 large stamina (full stamina), 7 antidote (cures all poison), 8 lesser resistance (resists poison 50% for 2 min, adds to the amulet), 9 greater resistance (cures, resists poison 95% for 2 min), 10 water blessing (Blessing of Hapi: wade, sprint and jump in half water for 2 min, 4 with the Favour of Hapi skill) |
| 4 | Amulet | 0 lesser amulet of strength, 1 minor amulet of strength, 2 amulet of strength, 3 grand amulet of strength (+1, +2, +4, +6 might); 4 lesser amulet of armor, 5 minor amulet of armor, 6 amulet of armor, 7 grand amulet of armor (+1, +2, +4, +6 armor); 8 lesser amulet of health, 9 minor amulet of health, 10 amulet of health, 11 grand amulet of health (+5, +10, +20, +30% max. HP); 12 lesser amulet of poison guard, 13 minor amulet of poison guard, 14 amulet of poison guard, 15 grand amulet of poison guard (10, 25, 50, 80% chance a poisoned hit does not poison); 16 lesser amulet of trap guard, 17 minor amulet of trap guard, 18 amulet of trap guard, 19 grand amulet of trap guard (25, 50, 75% less trap damage, grand: immune); 20 lesser amulet of smash guard, 21 minor amulet of smash guard, 22 amulet of smash guard, 23 grand amulet of smash guard (8, 16, 28, 40% less smash damage); 24 lesser amulet of cut guard, 25 minor amulet of cut guard, 26 amulet of cut guard, 27 grand amulet of cut guard (8, 16, 28, 40% less cut damage); 28 lesser amulet of stab guard, 29 minor amulet of stab guard, 30 amulet of stab guard, 31 grand amulet of stab guard (8, 16, 28, 40% less stab damage); 32 amulet of regeneration, 33 grand amulet of regeneration (1, 2 HP a second while no monster chases the player); 34 lesser amulet of venom, 35 minor amulet of venom, 36 amulet of venom, 37 grand amulet of venom (10, 15, 20, 30% of weapon hits poison the monster: weak, weak, medium, strong); 38 lesser amulet of fire guard, 39 minor amulet of fire guard, 40 amulet of fire guard, 41 grand amulet of fire guard (20, 40, 60, 80% less fire damage) |
| 0 | Empty chest | - |

Invalid ids write an error to the log and give nothing.

## File format

Level format v3, plain text (`src/world/level.h`). A header, the structure layer as a drawing, then the wall layer
and the object layer as lists:

```
DCLEVEL 3 40 47
structure
########################################
#......................................#
...                                       47 rows of 40 glyphs, the top row (46) first
wall 1
6 1 15 3 0                                column row type attribute value
objects 3
1 1 2 1 0                                 column row type attribute value
4 1 6 0 0
...
```

The structure glyphs are in the [Structure reference](#structure-reference). The wall list holds only the cells with
a fixture (wall types only), the object list only the cells with an object, row 0 (the bottom) first, left to right.
Rows count from the bottom, columns from the left.

The old formats still load: the game, the editor and `levelcheck` convert them as they read them (`levelcheck`
warns). v2 has no wall list: its wall types stand in the object list and move to the wall layer. v1 is the cell count
`1881`, then one `type attribute value` line per cell, type 0 a wall. `levelconvert FILE...` rewrites files in v3 for
good.
