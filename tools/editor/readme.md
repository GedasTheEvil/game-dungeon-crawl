# Level editor

`make editor`, then `make run-editor` (or `make run-editor LEVEL=content/levels/lvl3`, or `bin/editor <file>`: opens any
level file, relative to where you run it, and `Save` writes back to it). The editor runs from the repo root.

## Screen layout

* Left: the map grid, 40 columns x 47 rows. The bottom row of the grid is the bottom of the level (row 0).
  Above the grid: the column, row and contents of the cell under the mouse.
  Zoomed, the grid shows a window of the map and a minimap in its top right corner (see [Zoom](#zoom)).
* Right, top: the `Paint` / `Check` mode buttons and the `Structure` / `Wall` / `Objects` / `Decor` / `Decals` /
  `Looks` layer buttons.
* Right, middle: the palette of the layer (structures, the wall types, the object tiles, the props, the decals or the
  container looks, rows of nine; the panel shows three rows: when a layer has more, the mouse wheel over the palette
  scrolls a row a notch and a small arrow at the panel's top or bottom edge shows the hidden rows), then the `Attribute` and `Value` fields (fixtures and objects), or the
  `Mirror` button (props and decals, key `M`).
* Right, papyrus: what the tile, attribute and value mean (the [Tile reference](#tile-reference) as text).
  A text longer than the papyrus scrolls: the mouse wheel over it; a red bar on its right shows where you are.
  Numbers that mean nothing to the game show in red.
* Right, bottom: the level name field, the level's zone (click it or press `Z` for the next one; saved in the file,
  it sets the level's stone, decor, light and sound in the game, [zone-looks](../../docs/plan/solved/zone-looks.md)) and the
  `Save` and `Load` buttons.
* Bottom: a line with the mouse buttons and keys.

## Modes

* `Paint` (key `P`): click or drag on the grid to write the brush (selected tile, attribute and value) into cells.
  The cell under the mouse previews the brush.
* `Check` (key `C`): clicking a cell does not change it. The papyrus explains the clicked cell.
  The level check (see [docs/levels.md](../../docs/levels.md)) runs on the map: the path from the entrance to the
  goal is drawn with gold dots, the result shows under the grid.

In both modes, a right click on a cell picks it into the brush, in the layer being painted: its structure, its
fixture's or object's tile, attribute and value, its prop or its decal (else its torch) with the mirror, its look. In `Check` mode with the `Wall` layer, the papyrus explains the
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

A cell has six layers (see [File format](#file-format)): its structure (wall, empty, half water, deep water), at
most one fixture on its back wall (a lever, an inscription, wall spikes: the wall layer), at most one object in it
(a ladder, a monster, a key, ...), how it is dressed ([placed-decor](../../docs/plan/solved/placed-decor.md)): its prop
(the decor layer) and its wall decal and torch (the decal layer), and its treasure's container (the looks layer,
[placed-container-looks](../../docs/plan/solved/placed-container-looks.feature.md)). Key `L` (Structure, Wall,
Objects, Decor, Decals, Looks, round) or the layer buttons switch the layer the brush paints and the palette shows.
The map shows the structure, fixtures and objects in every layer; the other layers stay visible, dimmed, and the
layer being painted is drawn on top. The props show in the decor layer only, the decals and torches in the decal layer
only, the looks in the looks layer only.

* Painting an object or a fixture into a wall or deep water carves the cell open (`Empty`).
* Painting a wall or deep water over a cell removes its object, fixture, prop, decal, torch and shape.
* `None` in the wall or objects palette removes the fixture or the object and keeps the rest.
* Props, decals and torches go only in open cells (painting them on rock does nothing).

### Shapes (sub-cells)

A cell is 4 x 4 sub-cells, a quarter tile each way ([level-subcells](../../docs/plan/solved/level-subcells.md)).
Its shape says which are solid: steps to jump onto, low ceilings, crawl spaces. Only an open structure (empty, half
water, sand, quicksand) takes one. The structure palette has the shape brushes after the six structures:

* `No shape`: takes the shape off; the cell is just its structure again.
* `+ Sub` / `- Sub`: make the sub-cell under the mouse solid or open (drag to paint more). The cell under the mouse
  shows the sub-cell it paints.
* The common shapes, painted on the whole cell: `Step` (a quarter high over the whole floor), `Step L`, `Step R` (on
  the left or right half), `Half`, `Half L`, `Half R` (half a cell high: too high for one jump), `Stair R`, `Stair L`
  (three quarter steps rising to the right or left), `Low 1`, `Low 2`, `Low 3` (a ceiling one to three quarters lower;
  two or three make a crawl space), `Lip L`, `Lip R` (a rim a quarter high and wide at the cell's edge, as at a
  ledge's edge).

A shape painted on rock or deep water carves the cell open (`Empty`), unless it leaves it all solid (`+ Sub`, `No
shape`). Painting a structure keeps the shape; rock or deep water removes it. A right click picks the structure, and
the shape's brush when it is one of the common shapes. The map draws the solid sub-cells in the rock's colour, the
hover line shows the shape's mask. The game draws the solid sub-cells as blocks and the player walks and jumps
against them ([subcell-physics](../../docs/plan/solved/subcell-physics.md)); monsters and the checker still treat the cell as
open.

### Decor and decal layers

A cell's prop and decal are each in one of three states. **Unset** (every cell of a new or older level): the game
scatters them as on any level (a prop on 30% of the free dry floor and half the half water, by depth; a decal on 35%
of the cells, a torch on 25%, 4 cells apart). **Bare**: none. **A prop or decal**: that one, with its mirror (`M`), at
any depth, whatever is in the cell. Its jitter (a prop's offset, a decal's place on the wall) still comes from the
level.

* Decor palette: `Unset`, `Bare`, then the props: the dry ones, the water ones (half water only), the coffins and
  `Sleeper`, the decoy: a coffin with a mummy asleep in it for good, drawn as the real one lies in its coffin. A row
  of coffins with one real mummy and a few decoys keeps the player guessing. A mummy's spawn always gets its own
  coffin, a page its skeleton, a living ushabti its decor ushabti: a placed prop there is not shown. Then `Stargate`
  and `Dial` (plan stargate-chamber): the great ring against the back wall (interact dials it; no torch over it) and
  its dial device beside it, for the one chamber of the campaign. Last `Shrine of Isis` (plan shrine-of-isis): praying
  there lifts every curse and wards them off for 2 minutes. Then `Shrine of Montu`, `Ptah`, `Amun`, `Serket` (plan
  shrines-of-the-gods): the god's blessing for 2 minutes; at most one shrine of any god a level.
* Decals palette: `Unset` and `Bare` (both the decal and the torch), `Torch` and `No torch` (the torch only), then the
  decals of the game's atlas (the decal only). A torch and a free decal (one at eye height) would overlap: painting
  one clears the other. Random torches keep 4 cells away from a placed one; placed torches may stand closer.
* A random prop never puts a brazier, a lamp or a Thoth statue under a placed torch, nor anything in front of a placed
  floor decal; a random torch never goes over a placed free decal.

### Looks layer

A treasure's container (chest, coffer, basket, canopic jars, pottery jar, offering table, coffin) rolls by the depth
and the cell. The looks layer picks it by hand: **Rolled** (every cell of a new or older level) leaves the roll, a
container forces that one at any depth. A coffin's lid rolls too (40% carry an execration text); `Plain coffin` and
`Cursed coffin` fix it: the cursed one curses on opening and holds one bonus item, as a rolled one with the text.
Put it next to a warning inscription, or among the coffins of a burial chamber. The stance (turn, place in the cell)
still rolls; a coffin only lies facing the camera or angled. Only a treasure cell (`Treasure`, `TrappedTreasure`)
takes a look: painting one elsewhere is refused. A trapped coffin always shows the text, whatever its lid.

`levelcheck` warns where a placed one does not fit: a prop in rock, a dry prop in water or with no floor under it, a
water prop out of half water, a web with no ceiling, a prop (not the web) in front of a chest, door, gate, ankh,
ladder, mirror, monster or a fixture, a prop the game does not show; a decal in rock or on a cell that takes no decals
(a door, a gate, ...), a ceiling decal with no rock above, a floor decal with no floor, in water or sand, or behind a
prop, a free decal behind a torch; a torch in rock, in deep water, on a cell that takes no torch, over a brazier, a
lamp or a Thoth statue; a look on a cell without a treasure, a plain lid on a trapped treasure, a look with no stance
that fits its cell.

## Workflow

1. Pick the layer, then click a structure or a tile in the palette.
2. If the tile needs one, set the attribute and value (see [Tile reference](#tile-reference)):
   click `Attribute` (or press `Tab`), type digits. `Tab` goes to the next field, `Enter` or `Esc` leaves it.
   The numbers apply as you type. Reset both fields to empty (0) before you paint plain tiles.
3. Paint on the grid.
4. Click the name field, type a name (max. 32 characters: letters, digits, `_`, `-`, `.`).
5. Click `Save` (`Ctrl+S`). The file goes to `tools/editor/saved/<name>`. `Load` (`Ctrl+O`) reads the same path.
6. Optional: copy the file to `content/levels/lvlN`, or open the campaign file (`bin/editor content/levels/lvlN`) and save it in
   place. Typing a new name in the name field drops the opened path: `Save` and `Load` use `tools/editor/saved/<name>` again.
   A file that does not load, or more than one argument, exits 1 with a message on stderr. The game starts on `content/levels/lvl1` and each exit loads `lvl<N+1>`.

Limits: attribute max. 3 digits, value max. 4 digits, digits only.
A new map is all `Wall`. You carve the playable space out of it: paint `Empty` structure, or paint objects (they
carve their cell).

Tile and prop icons (`prop_<name>.png`, `torch.png`, `no_torch.png`, `bare.png`) are PNG files in
`tools/editor/icons/`, drawn by `tools/editor/icons/make_icons.py` (Pillow; a tile's file is the `icon` in
`src/level/tile_defs.cpp`, every tile but `None`, `Unknown` and the structures has one,
[editor-icons-and-small-font](../../docs/plan/solved/editor-icons-and-small-font.md)); the decals show their cell of the game's
`media/textures/decorations/decals.png`. Fonts and backgrounds come from the game's `media/textures/fonts/` and
`media/textures/ui/`: the title, headings and body text are parchment, as in the game; the small text (palette names,
the readout, the hints, the footer) is DejaVu Sans Condensed Bold (`dejavu-condensed-bold.png`, loaded mipmapped so it
stays sharp this small). A palette name too wide for its tile is drawn a size smaller.

## Structure reference

| Glyph | Structure | Number | In game |
|---|---|---|---|
| `#` | Wall | 0 | Solid rock. The player stands on it and cannot walk through it. Holds no object. |
| `.` | Empty | 1 | Open space. With no wall below, the player falls. |
| `~` | Half water | 2 | Open space, half filled with water. Any object may stand in it. The player wades at half speed and cannot sprint or jump in it (the Favour of Hapi skill and the Blessing of Hapi potion allow both); put deep water or a wall under it ([docs/levels.md](../../docs/level-format.md)). |
| `=` | Deep water | 3 | Full of water, solid like a wall. Holds no object. |
| `,` | Sand | 4 | Open space, a drift of sand on the floor (plan desert-chapter). Any object may stand in it. The player walks at 3/4 speed and cannot sprint in it; jumping is fine. A sand worm placed in it lies under the surface (a ripple) and comes up under the player within 2.5 cells on its row (at the sand's edge for a player beside it), later dives to come up under them again. Put a wall under it. |
| `%` | Quicksand | 5 | Looks like sand, with a slow swirl up close. Standing in it sinks the player: slower each second, stuck after 3 s, 5% of the max HP a second from 5 s (no armour); a jump frees them, for 40 stamina (twice a jump's). `levelcheck` treats it as a passable trap (a jump crosses one cell of it) and warns about runs of more than 2 cells. Put a wall under it. |

The glyph is the one in the level file's structure drawing and in `levelcheck --map`. A shaped cell (see
[Shapes](#shapes-sub-cells)) has a glyph of its own: `_` empty, `-` half water, `;` sand, `&` quicksand.

## Tile reference

The object and wall layers share the type numbers. The wall types (`Lever`, `Inscription`, `WallSpikes`, `SunDisc`,
`FireVent`, `PoisonDrip`, `SandGust`, `CeilingSpikes`, `CeilingDeath`) go in the
wall layer, the others in the object layer. Objects palette: `None`, `Door`, `Death`, `Monster`, `Spike`, `Ladder`,
`Treasure`; then `Ankh`, `Key`, `Gate`, `RockFall`, `DartPlate`, ..., `TrappedTreasure`. Wall palette: `None`, `Lever`, `Inscription`,
`WallSpikes`, `SunDisc`, `FireVent`, `PoisonDrip`, `SandGust`, `CeilingSpikes`, `CeilingDeath`. Types 0 (the wall, now a structure) and 7 (`Area3D`, unused) are gone.

| Type | Tile | Attribute | Value | In game |
|---|---|---|---|---|
| 1 | None | - | - | No object: the cell is just its structure. |
| 2 | Door (sphinx gate) | Gate type, see below | - | Sphinx statue. Behaviour depends on the gate type. |
| 3 | Death | - | - | Large spike trap (scale 40). Damages the player on contact. |
| 4 | Monster | Monster type, see below | - | Monster spawns on this cell when the cell is in view. |
| 5 | Spike | - | - | Small spike trap (scale 16). Damages the player on contact. |
| 6 | Ladder | - | - | Column. The player climbs up and down only between vertically adjacent ladder cells, and does not fall on it. |
| 8 | Treasure | Item type, see below | Item id, see below | A treasure container (chest, coffer, basket, canopic jars, jar, offering table, coffin: rolled by depth; some coffins carry a curse text on the lid) holding the item. Interact to open it, again to take the item. The cell is then left without an object. |
| 9 | Ankh | - | - | Level goal. Interact with it to win the game. |
| 10 | Key | Lock colour, see below | - | Key on the floor. The player picks it up on touch. The cell is then left without an object. |
| 11 | Gate (lock gate) | Lock colour, see below | 0 closed, 1 open | Portcullis. A closed gate blocks the corridor. It opens when the player comes up to it with the key of its colour, or when a lever of its colour is pulled. It slides up in 1.2 s. |
| 12 | Lever | Lock colour, see below | 0 | Interact to pull it. Opens every gate of the same colour. |
| 13 | RockFall | - | 0 | Loose ceiling, walkable. When the player steps into the cell, grit trickles down and a rock falls after approx. 0.65 s + 0.3 s. Under its centre (0.3 cells) it crushes (1000 damage), nearer its edge (0.6 cells) it grazes (50 damage); armor does not help. Walk on without stopping, sprint on or step back to get clear; a jump in place does not dodge it. The rock stays on the floor (walkable). Attribute 1 (`&` in the ASCII sources): a sand fall, sand pours from a crack instead (40 damage, no armour, and a 1.5 s slow to 40% for the player under it); the cell turns to sand. Put a wall above it. |
| 14 | DartPlate | - | 0 armed, 1 pressed | Pressure plate in the floor ([dart trap](../../docs/plan/solved/poison-dart-trap.md)). When the player or a monster of weight 2 and up (not rats, scarabs, scorpions, cobras or flyers) stands on the slab, it clicks and the 3 dart holes in the back wall above it shoot, 0.2 s later and 0.16 s apart, out across the corridor. At 0.3 cells over the floor (a jump at its top lets them pass, small monsters too) each takes the body in front of its hole: 2 pierce damage and medium poison. Jumping over the plate does not press it. It re-arms 3 s after the click, once nothing heavy stands on it. Cowards stop at its edge like at spikes. |
| 15 | Inscription | Text id (an `id:` of the files); 100 a name cartouche ([names](../../docs/names.md)) | - | Hieroglyphs on the back wall ([inscriptions](../../docs/inscriptions.md)). Interact to read them: the status line shows the title and the archaeologist's comment, the translation goes into the journal. The text id is an `id:` in `content/inscriptions/*.txt`. On every campaign level. Put a floor under it. |
| 16 | WallSpikes | Speed: 0 normal, 1 fast, 2 slow | Phase, 0 to 7 (eighths of the cycle) | Rhythm trap ([plan](../../docs/plan/solved/rhythm-traps.md)): poison spikes shoot out of the back wall and go back in on a fixed cycle: out 1.2 s of every 4 s (fast: 0.8 of 3.2 s, slow: 1.6 of 4.8 s), a scrape and a twitch 0.6 s before they come out. Out, they block like a closed gate (no jump over them). Whoever stands in the cell as they come out takes one stab (10 damage, weak poison; the trap ward and the Dodge skill help) and is shoved out. The phase shifts a trap against the others: a rhythm. The checker warns when a run of them in a row cannot be walked through both ways; at normal speed a run of three never can. Put a floor under it and a wall above it. |
| 17 | LightShaft | Way: 0 down, 1 left, 2 up, 3 right | - | Sunlight through a crack in the rock ([sunlight mirrors](../../docs/plan/solved/sunlight-mirrors.md)): a beam runs from this cell the way it shines, through open cells, until rock, deep water or a gate not fully open stops it. Mirrors turn it; it burns mummies standing in it and keeps camel spiders out like a torch. Put rock behind it (above it for a beam down; the checker warns). |
| 18 | Mirror | - | Way it points: 0 down, 1 left, 2 up, 3 right | A bronze mirror on a stand: a beam that falls on it goes on the way it points (the gold pointer on its hub); pointing the way the beam goes it stands edge-on and lets it by, pointing back at it, it catches it. Interact turns it a quarter turn clockwise. Not in front of an inscription (reading it takes the key; the checker warns). |
| 19 | SunDisc | Lock colour, see below | 0 | A sun disc (the Aten) on the back wall. A beam on it lights it for good and opens every gate of its colour, as a lever. The checker warns when no beam can ever reach it, and when one lights it as the level starts. |
| 20 | Page | Page number, 1 to 24, in order of depth over the campaign | 0 (1 once taken) | A torn page of the earlier expedition's notebook ([expedition notes](../../docs/plan/solved/expedition-notes.md)), floating over a skeleton (the scatter puts one there). Picked up on touch, it goes into the journal's Expedition section; the skeleton stays. Optional: keep it off the exit path, but for the last page (24), which lies on it. The checker warns about one out of reach or without a number. |
| 21 | TrappedTreasure | Item type, as Treasure | Item id, as Treasure | A trapped treasure ([trapped chests](../../docs/plan/solved/trapped-chests.md)): it looks like any container (a darker one only up close) and opens the same way, but its opening sets off a trap, rolled then: poison darts (6 damage, the trap ward cuts it, medium poison), a puff of poison gas (weak poison while the player stands in it, 4 s), a curse (an execration text on the lid) or, deep down, 3 scarabs digging out; darts and gas early, curses and the swarm from the worked tunnels and the tombs on. Canopic jars and pottery jars always curse (disease). The trap ward amulet's percentage is also the chance the trap does not go off. Half of them hold the item and one bonus more, the rest are empty. Never put what the level needs in one: the checker does not count its antidote. |
| 22 | FireVent | Speed: 0 normal, 1 fast, 2 slow | Phase, 0 to 7 (eighths of the cycle) | Rhythm trap ([plan](../../docs/plan/solved/fire-jet-trap.md)): a vent low in the back wall breathes a jet of fire up through the cell, on the wall spikes' cycle (out 1.2 s of every 4 s, fast 0.8 of 3.2 s, slow 1.6 of 4.8 s); 0.6 s before, it glows red, sparks and hisses, then roars. The jet blocks nothing: whoever stands in the cell while it burns takes half their max HP a second (at least 450, a whole jet 540 and more; monsters 15), all fire (through armour, the trap ward and the Fire Warding amulet both cut it) and burns on for 3 s (a fifth of that a second); a jump does not clear it. Monsters as at spikes, their fire resistance counts (a mummy burns double). Runs of them with the phases staggered: the checker warns when a run of rhythm traps (wall spikes and fire vents) cannot be walked through both ways. The jet lights the corridor. Put a floor under it. |
| 23 | PoisonDrip | Speed: 0 normal, 1 fast, 2 slow | Phase, 0 to 7 (eighths of the cycle) | Rhythm and floor trap ([plan](../../docs/plan/solved/poison-drip-trap.md)), in the wall layer for the wall spikes' cycle though it hangs from the ceiling: a green puddle on the floor that never dries and a lump of venom in a stain on the ceiling over it. Walking into the puddle poisons (weak, once per entry, no damage); a jump clears it, flyers too. The lump swells, and in the last 0.6 s it wobbles and creaks, then falls into the puddle and the venom pours after it for the spikes' out time (1.2 s of every 4 s, fast 0.8 of 3.2 s, slow 1.6 of 4.8 s): whoever is in the cell then, in a jump too, takes 8 (the trap ward cuts it, not the poison) and medium poison, once per drop. Cowards stop at the puddle (walk-jumpers leap it), the reckless walk in and are poisoned. A row of them, staggered, with free cells between: jump each puddle between drops. The checker counts the puddle as a floor trap (jumped over), warns when a run of rhythm traps cannot be walked through both ways, and wants an antidote in reach. Put a floor under it and a ceiling over it; nothing in the cell but a key or a page. |
| 24 | SandGust | Wind: speed (0 normal, 1 fast, 2 slow), plus 3 for a wind to the left (else to the right) | Phase, 0 to 7 (eighths of the cycle) | A sandstorm hall's wind ([plan](../../docs/plan/solved/sandstorm-halls.md)), in the wall layer for its cycle: gusts of sand blow along the row through the cell, 2 s of every 7 s (fast 1.6 of 5.6 s, slow 2.4 of 8.4 s); for 1.2 s before, the hiss rises and grit skitters along the floor. A gust pushes the player downwind at 1.6 tiles a second, faster than the walk (not on a ladder), and the light monsters (the small walkers, the flyers; no boss) at 1.2; arrows and spit drift. Near a hall the haze closes in, in a gust the wind drowns the other sounds. No hurt. Mark each cell of the hall; the cells left out are its shelters, where the wind does not reach; the game draws a sandstone pillar on the windward side of each (no prop needed). Give the gusts upwind a higher phase: the gust rolls down the hall. The checker warns when a run of them cannot be crossed against the wind in one calm (in sand at most 2 cells at normal speed, 3 slow; on stone 3, 4 slow), blows both ways, has no floor under it or spikes, a death trap or quicksand in it, or blows the player into a drop, a trap or quicksand at its downwind end. Put a floor under it. |
| 25 | CeilingSpikes | Unused (0) | Unused (0) | Ceiling trap ([plan](../../docs/plan/solved/ceiling-spikes.md)), in the wall layer: the floor spikes upside down on the underside of every solid sub-cell over open space in the cell's shape (not the bottom row). Walking, crouching and standing under them are safe. A jump into them, the player's or a monster's, at any speed, costs what a walk over the floor spikes costs (200 HP or 20% of the max HP, through armour, the trap ward helps) instead of the head bump, with no daze; a monster takes its walk through the floor kind. Cowardly leapers do not leap under them; the reckless and the pouncers do (a lure: wait under them for a pounce). Only on a shaped cell with a ceiling a jump reaches: the loader and the checker reject any other. The checker warns when the only way on jumps into them (a quarter step under them). |
| 26 | CeilingDeath | Unused (0) | Unused (0) | Ceiling death trap, as `CeilingSpikes`: a jump into it costs a walk over the death trap (400 HP or 40%) and its tips poison strongly. |
| 27 | PressurePlate | Lock colour, see below | The weight it needs, 1 and up | A pressure plate in the floor with a gem of its colour ([pressure plate](../../docs/plan/solved/pressure-plate.feature.md)), in the wall layer so a thing can stand on it. While every plate of its colour carries its weight, the silver plate gates of that colour (Gate, colour 6 to 9) stand open; under it they come down (not on anyone in the gate's cell). Weights: the player 2, a stone 4, an emptied chest or a mirror 2, a monster or its body its own weight (rats and scarabs 1, most 2, the big ones 3 to 6). The player drags things onto it (the drag key: stones, mirrors, emptied chests, bodies). The checker warns when the stones, mirrors, chests and monsters on its flat floor run weigh less than it needs, and about a plate without plate gates or plate gates without a plate. Put a wall under it. |
| 28 | Stone | - | - | A loose stone block ([pressure plate](../../docs/plan/solved/pressure-plate.feature.md)): the player drags it along the bare flat floor (no water, sand or steps), onto a pressure plate (weight 4). |

Spikes and death traps: walking over one costs the player a bell curve of damage over it (the middle most, the edge a
nick), about 200 HP for the spikes and 400 for the death trap, or 20% / 40% of the max HP when more; a quarter more at
a sprint. Armour does not help, the trap ward does. A jump clears the spikes and only grazes the death trap. On top,
standing in one hurts 1 HP every 0.1 s, rising while the player stays. Monsters only take that second part.

### Door: gate type (attribute)

| Attribute | Gate | Behaviour |
|---|---|---|
| 1 | Entrance | Player start position. Shows a plasma portal. Use exactly one per level; if there are more, the last one in the file wins. |
| 2 | Exit | Plasma portal. Interact to load the next level (`content/levels/lvl<N+1>`). |
| 3 | Riddle | Question mark above the sphinx. Interact to get a riddle; the gate then becomes type 4. |
| 4 | Empty gate | Decoration only (also the state of a used riddle gate). `levelcheck` warns about one in a level file. |
| 5 | Teleporter | Two columns facing the camera, plasma between them (no sphinx). Interact to jump to the other teleporter with the same value (pair id); the player can go back the same way. Use exactly two per pair id. A value of pair id + 256 makes a name gate: sealed by the level's royal name ([names](../../docs/names.md)). A value of pair id + 512 makes a ring platform ([ring-transporter](../../docs/plan/solved/ring-transporter.md)): rings drop from the ceiling, rock above it; + 768 a ring platform sealed by the name. |
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
| 9 | Mimic (game name: Chest of Seth) |
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
| 29 | Cursed mimic (game name: Cursed chest of Seth) |
| 30 | Seth (boss) |
| 31 | Baboon (game name: Baboon of the Lake of Fire) |
| 32 | Rerek (boss) |
| 33 | Mother of webs (boss) |
| 34 | Babi (boss) |
| 35 | Mimic jar (game name: Jar of Seth) |
| 36 | Tiny spider |

Bats hang on the ceiling of their cell until the player comes within 1.75 cells in the same row, then fly through the player (a bite on the way), 1.5 cells on, turn and come back. They fly over traps and turn at walls. A mimic looks like a treasure chest until the player comes within 1.5 cells, then bites; killed, it leaves a real chest with a random weapon or potion. A mummy lies in an open coffin on its cell until the player comes within 1.6 cells (or hits it), climbs out, then walks like the others. The giant rat and giant scarab leap over pits and traps up to 2 cells wide. From 0.6 to 1 cell away the giant scarab turns its back, shapes a dung ball and kicks it at the player (every 4 s): it rolls along the floor over traps (a jump clears it), falls into a pit, sinks in half water; Khepri rolls a bigger, faster, burning one from up to 1.1 cells. A crocodile lies in the water (only its eyes and back show) until the player comes within 1.4 cells of its centre in the same row, then walks like the others; slow on land, in half water 2.5 times as fast (faster than the player walks). Put it in or next to water. A scorpion's sting poisons the player (weak poison: 1 HP a second for 20 s). A cobra lies coiled until the player comes within 1.8 cells in the same row, rears up, then walks like the others; from up to 2.5 cells it stops and spits venom at the player's chest (a jump dodges it). Its bite and its venom poison (medium: 3 HP a second for 25 s). The giant cobra does the same, bigger, from up to 3 cells. The giant scorpion's sting poisons (medium). A blue lotus floats on the water like a flower (only a twitch now and then gives it away) until the player comes within 1.2 cells of its centre in the same row, rears up, then bites where it stands; each bite drains a quarter of the player's stamina. Put it in half water. A camel spider runs faster than the player but does not step into the light: a cell within 1 cell along the row of a wall torch, a brazier or an oil lamp. Light between it and the player keeps it off (it runs out of the bow's range, like a coward that cannot reach the player); placed in the light, it runs for the nearest dark. Only at full HP: hurt, it forgets the fear for good, walks and climbs through the light and bites in it. A tiny spider is the camel spider's small kin: as fast as a rat, two club blows kill it, it keeps out of the light the same way and fits through crawl spaces. Place them in groups of 2 to 4 side by side. A hippo lies in the water like the crocodile (only its eyes, ears and nostrils show) until the player comes within 4 cells of its centre in the same row, then gapes and charges along the row from 2 to 4 cells away (3.2 cells a second, through the player for double damage, on into a wall that stuns it for 2.5 s); between charges it walks, slowly on land, three times as fast in half water. Put it in or next to water, best in a long pool. A living ushabti stands on its cell among the decor ushabti (the game puts them there) as one of them, its arms crossed (a turn of its head now and then gives it away), until the player comes within 1.3 cells of its centre in the same row (or hits it), steps off, then walks slowly like the others. Glazed faience: a club breaks it at double damage, an arrow skids off. A cursed mimic is a darker chest (purple where the treasure chest is blue) that lies in wait like the mimic, then hops after the player (a bit slower than they walk) and leaps pits and traps like the giant rat; a quarter of its bites lay insanity. Killed, it leaves its chest where it died: half the time a lesser or minor amulet, else now and then a ring, else the mimic's loot. A monster bites a moment after it opens its jaws; a quick jump over a low one dodges the bite. The Anubis shoots a bolt of fire from its sceptre from up to 1.4 cells (the Anubis boss from up to 2); it sets the player burning. In half water a crocodile's bite may hold the player for a death roll; the giant scorpion's pincers may hold the player for the sting (a hit on it ends the hold sooner). Climbers follow the player up and down ladders: rats, giant rats, scorpions, giant scorpions, mummies, the Anubis, camel spiders, tiny spiders and their bosses; a camel or tiny spider takes no way through the light, so a lit ladder top keeps it below. A baboon keeps its distance: it stands 0.55 to 1.1 cells from the player and hurls a burning coal every 2 s, from up to 1.3 cells (a screech first; a jump dodges it, half water puts the fire out), backs off when the player comes at it (a bit faster than they walk, never past its range, not over a drop, a trap or a ladder) and bites only when cornered. It climbs ladders. A mimic jar looks like the sealed pottery jar container and lies in wait like the mimic (open it or hit it to wake it), then bites where it stands; killed, it leaves its smashed jar with a potion. Put it in the tombs (levels 14-18, 46-50), where the jars stand among the containers. Any other value spawns a copy of the player model. Max. 32 monsters are live at one time.

A boss (boss scarab, vampire bat, scorpion queen, Apep, Sobek, mummy priest, Anubis boss, Seth, Rerek, Mother of Webs, Babi) summons minions next to itself, on the side away from the player: some when it
appears, then one every few seconds up to a limit. The boss scarab's scarabs dig out of the floor, the vampire bat's
bats drop from the ceiling. The vampire bat flies like a bat and heals by 30% of the HP its bites take. The mummy
priest lies in a large coffin like a mummy, swings a staff, shoots a purple bolt from up to 1.8 cells and lays many
curses; his mummies climb out of coffins: one stands on every empty floor cell within 3 cells of the boss's tile on its
row; a mummy rises from the free coffin nearest the boss, not beyond the player or next to them. The Anubis boss's
guards step out of the wall the player looks at (the end of their row), onto the free cell in front of it; every other one is a living ushabti. The scorpion queen's giant scorpions hatch from the egg clusters (monster 20: rooted, harmless, killable) placed in her room; with none left she summons no more. Apep dives into the floor and comes up 1.6 cells behind the player; his cobras dig out. Sobek lies in the water, wakes from 4 cells, charges along the row (a jump clears it) and is stunned by a wall; his crocodiles come out beside him. Seth, the red hippo, does the same, charging from 2.5 to 7 cells at 4.5 cells a second; as a stun ends he roars a sandstorm: within 2 cells of him on his row the player is hit, shoved 1.5 cells away and blinded by sand for a second. His hippos come out beside him. Put him in or next to water, in a long pool. Rerek, the sand serpent, lies under the sand like the sand worm and wakes from 3.5 cells; he dives for the player from 2 to 9 cells away and swims under the sand fast, coming up under the player's feet, every other time 1.6 cells past them, in a spray of sand (within 1.2 cells: shoved 0.8 cells away, blinded for a moment). Some of his bites swallow the player into the sand (a grinding hit each second for 3 s; each hit on him ends it sooner). Out of sand he walks, slowly. His worms dig out into the sand beside him and lie in wait there. Put him in a long run of sand. The Mother of Webs, the camel spiders' boss, runs faster than they do and keeps out of the light like them, but (hurt, she too ignores the light) puts out every wall torch and oil lamp within 2 cells of her along her row as she comes, for good (the braziers and sunlight stay: only they keep her off). Every 1.2 s she spins a web over the floor of the cell she stands in (up to 8): in it the player walks at 0.4 speed; a blade's swing cuts it, a fire arrow over it or a fire vent's jet burns it. Her camel spiders drop from the ceiling beside her. Give her room for braziers. Babi, the baboons' boss, keeps his distance like them, wider (0.6 to 1.25 cells) and backing off faster (still slower than a sprint); with a gap behind him he leaps it to the next ledge instead of turning to fight. He throws from up to 1.4 cells every 1.6 s: a volley of two or three burning coals a quarter second apart, each aimed as it leaves; every third throw one big coal at the player's feet, and where it lands (on them or on the floor) the floor burns for 4 s (30 HP every 0.5 s to the player standing in it, not in a jump; not in half water). Cornered, a two-handed blow and a bite. His baboons dig out beside him. Give him ledges with one-cell gaps between them, over fire. Its HP shows in a bar at the bottom of the screen. Its death opens every boss
gate (Gate with lock colour 5); the boss does not come back, also not after loading a save. Minions give 1 XP while
the boss lives, half the normal XP after its death. Put at most one boss on a level, in a room only a teleporter
leads to (`levelcheck` warns otherwise).

### Key, Gate, Lever: lock colour (attribute)

| Attribute | Colour | Gem |
|---|---|---|
| 1 | Red | Carnelian |
| 2 | Blue | Lapis |
| 3 | Green | Turquoise |
| 4 | Gold | Gold |
| 5 | Boss (gates only) | Obsidian |
| 6 to 9 | Plate gate (gates only): red, blue, green, gold | the plate's gem |

Keys, gates and levers with the same colour belong together. A boss gate (colour 5) has no key or lever: it opens
when the level's boss dies. A plate gate (colour 5 + the lock colour, a silver gate) has no key or lever: the
pressure plates (type 27) of the lock colour hold it open.

### Treasure: item type (attribute) and item id (value)

| Attribute | Item type | Value |
|---|---|---|
| 1 | Melee weapon | 0 club, 1 short sword, 2 spear, 3 dagger, 4 khopesh, 5 epsilon axe, 6 duckbill axe, 7 mace |
| 2 | Ranged weapon | 0 self-bow, 1 composite bow, 2 sling, 3 throwing stick, 4 javelin |
| 3 | Potion | 0 small health (25% of max. HP), 1 large health (50% of max. HP), 2 might (+2 might), 3 armor (+2 armor), 4 life (+5% max. HP, full heal), 5 small stamina (50% stamina), 6 large stamina (full stamina), 7 antidote (cures all poison), 8 lesser resistance (resists poison 50% for 2 min, adds to the amulet), 9 greater resistance (cures, resists poison 95% for 2 min), 10 water blessing (Blessing of Hapi: wade, sprint and jump in half water for 2 min, 4 with the Favour of Hapi skill) |
| 4 | Amulet | 0 lesser amulet of strength, 1 minor amulet of strength, 2 amulet of strength, 3 grand amulet of strength (+1, +2, +4, +6 might); 4 lesser amulet of armor, 5 minor amulet of armor, 6 amulet of armor, 7 grand amulet of armor (+1, +2, +4, +6 armor); 8 lesser amulet of health, 9 minor amulet of health, 10 amulet of health, 11 grand amulet of health (+5, +10, +20, +30% max. HP); 12 lesser amulet of poison guard, 13 minor amulet of poison guard, 14 amulet of poison guard, 15 grand amulet of poison guard (10, 25, 50, 80% chance a poisoned hit does not poison); 16 lesser amulet of trap guard, 17 minor amulet of trap guard, 18 amulet of trap guard, 19 grand amulet of trap guard (25, 50, 75% less trap damage, grand: immune); 20 lesser amulet of smash guard, 21 minor amulet of smash guard, 22 amulet of smash guard, 23 grand amulet of smash guard (8, 16, 28, 40% less smash damage); 24 lesser amulet of cut guard, 25 minor amulet of cut guard, 26 amulet of cut guard, 27 grand amulet of cut guard (8, 16, 28, 40% less cut damage); 28 lesser amulet of stab guard, 29 minor amulet of stab guard, 30 amulet of stab guard, 31 grand amulet of stab guard (8, 16, 28, 40% less stab damage); 32 amulet of regeneration, 33 grand amulet of regeneration (1, 2 HP a second while no monster chases the player); 34 lesser amulet of venom, 35 minor amulet of venom, 36 amulet of venom, 37 grand amulet of venom (10, 15, 20, 30% of weapon hits poison the monster: weak, weak, medium, strong); 38 lesser amulet of fire guard, 39 minor amulet of fire guard, 40 amulet of fire guard, 41 grand amulet of fire guard (20, 40, 60, 80% less fire damage); 42 lesser amulet of crushing, 43 minor amulet of crushing, 44 amulet of crushing, 45 grand amulet of crushing (10, 20, 30, 50% more smash damage: only the smash share of the weapon's damage); 46 lesser amulet of cleaving, 47 minor amulet of cleaving, 48 amulet of cleaving, 49 grand amulet of cleaving (10, 20, 30, 50% more cut damage); 50 lesser amulet of piercing, 51 minor amulet of piercing, 52 amulet of piercing, 53 grand amulet of piercing (10, 20, 30, 50% more stab damage); 54 lesser amulet of shots, 55 minor amulet of shots, 56 amulet of shots, 57 grand amulet of shots (25, 50, 75, 100% more shots in every ranged weapon, rounded up, at least one more); 58 lesser amulet of quick shots, 59 minor amulet of quick shots, 60 amulet of quick shots, 61 grand amulet of quick shots (the shots come back 25, 50, 75, 100% faster) |
| 5 | Ring | Two worn at once, one per hand; both count, two of the same too ([rings](../../docs/plan/solved/ring-accessories.md)): 0 copper ring of regeneration, 1 silver ring of regeneration, 2 gold ring of regeneration (garnet: 1 HP every 8, 5, 3 s, also in a fight; not while poisoned); 3 copper ring of vigor, 4 silver ring of vigor, 5 gold ring of vigor (lapis: stamina returns 15, 25, 40% faster); 6 copper ring of cleansing, 7 silver ring of cleansing, 8 gold ring of cleansing (feldspar: poison wears off 20, 35, 50% faster); 9 copper ring of speed, 10 silver ring of speed, 11 gold ring of speed (jasper: walk and sprint 5, 8, 12% faster, not the jump); 12 copper ring of reach, 13 silver ring of reach, 14 gold ring of reach (rock crystal: weapon reach +10, 15, 25%); 15 copper ring of sundering, 16 silver ring of sundering, 17 gold ring of sundering (amethyst: a monster's rates move 10, 20, 50% of a resistance tier toward more damage); 18 sky-iron ring of regeneration (1% of max HP, at least 1, every 2 s), 19 sky-iron ring of vigor (60%), 20 sky-iron ring of cleansing (70%), 21 sky-iron ring of speed (17%), 22 sky-iron ring of reach (35%), 23 sky-iron ring of sundering (a whole tier, also moves immune) ([sky iron](../../docs/plan/solved/sky-iron-rings.feature.md): only from merging two gold rings at a shrine). Rings come by chance only: not placed by hand |
| 0 | Empty chest | - |

Invalid ids write an error to the log and give nothing.

## File format

Level format v7, plain text (`src/level/level.h`). A header, the level's zone, the structure layer as a drawing,
then the wall layer, the object layer, the decor layer, the decal layer, the shape layer and the looks layer as lists:

```
DCLEVEL 7 40 47
zone tomb_entrance                        the zone's slug (ZONE_INFO, src/level/campaign.h), or none
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
decor 2
8 1 24 0                                  column row prop mirror
9 1 -1 0
decals 1
7 1 -2 0 2                                column row decal mirror torch
shapes 1
3 1 000f                                  column row mask
containers 1
4 1 6 2                                   column row kind lid
```

The structure glyphs are in the [Structure reference](#structure-reference). The wall list holds only the cells with
a fixture (wall types only), the object list only the cells with an object, row 0 (the bottom) first, left to right.
Rows count from the bottom, columns from the left.

The decor list holds the cells whose prop is set, the decal list those whose decal or torch is set. `prop` is an
index of `DECOR_NAMES`, `decal` one of `DECAL_DEFS` (`src/level/decor.h`, `levelcheck --legend` lists both), `-1`
bare, `-2` unset (a decal line with only its torch set). `mirror` is 0 or 1, `torch` 0 unset, 1 none, 2 a torch.

The shape list holds the shaped cells, `mask` 4 hex digits: bit = sub-column + 4 x sub-row, sub-row 0 at the bottom
(`000f` a quarter step over the floor, `f000` a ceiling a quarter lower). A shaped cell has its shaped glyph in the
structure drawing, and only a shaped cell does.

The containers list holds the cells whose look is set: `kind` an index of `CONTAINER_NAMES`
(`src/level/containers.h`: 0 chest, 1 coffer, 2 basket, 3 canopic, 4 jar, 5 table, 6 coffin), `lid` 0 rolled, 1
plain, 2 inscribed (a coffin only).

The old formats still load: the game, the editor and `levelcheck` convert them as they read them (`levelcheck`
warns). v6 has no containers list: every look rolls. v5 has no shape list: no shapes. v4 has no zone line: no zone. v3 has no decor and decal lists: every cell is unset. v2 has no wall list: its wall types stand in the object list and move to the wall layer. v1 is the cell count
`1881`, then one `type attribute value` line per cell, type 0 a wall. `levelconvert FILE...` rewrites files in v7 for
good.
