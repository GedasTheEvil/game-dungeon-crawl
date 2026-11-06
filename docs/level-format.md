# Level format

How a level file and a cell are laid out. The campaign, levelcheck and levelgen: [levels.md](levels.md).
Level files, tile types and the editor: [tools/editor/readme.md](../tools/editor/readme.md).
Tile types and structures in code: `src/world/level.h`.

A cell has three layers ([level-format-layers.md](plan/solved/level-format-layers.md),
[wall-layer.md](plan/solved/wall-layer.md)):

* **Structure**: `Wall`, `Empty`, `HalfWater`, `DeepWater`, `Sand`, `Quicksand`. A cell blocks when it is `Wall` or
  `DeepWater`.
* **Wall**: at most one fixture on the cell's back wall, with attribute and value: lever, inscription, wall spikes,
  sun disc, fire vent, poison drip, sand gust, ceiling spikes, ceiling death trap (the wall types, `isWallType` in
  `src/world/level.h`).
* **Object**: at most one thing standing in the cell, with attribute and value: ladder, door, gate, key, treasure,
  trapped treasure, ankh, monster spawn, spike, death trap, rock fall, dart plate, light shaft, mirror, expedition page.

An open structure can carry a **shape** ([level-subcells](plan/solved/level-subcells.md)): the cell splits into
4 x 4 sub-cells, a quarter tile each way, and the shape is the mask of the solid ones (steps, stairs, low ceilings,
crawl spaces). The game asks `solidAt`, `floorHeight` and `ceilingHeight` (`src/world/level.h`), never the bits.
The game draws the solid sub-cells as blocks; the player is blocked by them, jumps onto a quarter step (not a half),
bumps a low ceiling and stands on a step with a foot ([subcell-physics](plan/solved/subcell-physics.md)); arrows and sunbeams
stop at them. Monsters take steps and crawl spaces by their size ([subcell-monsters](plan/solved/subcell-monsters.md)).
The map screen draws its solid sub-cells. Test level: `tests/levels/subcells`.

Sub-cell rules (the checker, [subcell-checker](plan/solved/subcell-checker.md)): `levelcheck` walks a shaped cell by
its sub-cells (below, "Sub-cells"). It warns about a shape in a wall or deep water cell; a ladder, gate or door
(entrance, exit, teleporter) in a shaped cell (they need the cell open from floor to ceiling); a chest with no room
to stand at it (half a tile over its floor in the cell's middle); a step two or three quarters high whose top nothing
reaches (no jump gets there: give it a ladder or a way down onto it); a crawl space that ends at a drop of a cell or
more, or at a trap (spikes, a death trap, a dart plate, a rock fall, a rhythm trap); a placed prop on a shape that is
not one flat floor with nothing over it (on a flat step it stands on the step); a placed decal or torch a solid
sub-cell hides (a floor decal: the bottom row, a ceiling one: the top row, any other and a torch: the middle).
A large monster does not reach the path through a crawl space it does not fit (the difficulty's monsters near).

Ceiling traps ([ceiling-spikes](plan/solved/ceiling-spikes.md)): ceiling spikes (`"`) and a ceiling death trap (`'`), in the
wall layer of a shaped cell, hang from every underside of its solid sub-cells over open space in it. A walk under them
is safe, a jump into them costs a walk over the floor kind: a "never jump here" the player reads off the ceiling, and
a lure for pouncers. Only on a cell with a ceiling a jump reaches (the loader and the checker reject any other); the
checker warns when the only way on jumps into them (a quarter step under them). Test level:
`tests/levels/ceiling_spikes`.

Two more say how the cell is dressed ([placed-decor](plan/solved/placed-decor.md)), each unset (the game scatters
it as on any level), bare (none) or one picked with its mirror, whatever the depth:

* **Decor**: the cell's prop (`DECOR_NAMES`, `src/world/decor.h`), among them the decoy, a coffin with a mummy asleep
  in it for good. A mummy's spawn keeps its own coffin, a page its skeleton, a living ushabti its decor ushabti.
* **Decal**: the cell's wall decal (`DECAL_DEFS`) and, as a second field, its wall torch (unset, none, a torch).
  Random torches keep their gap to a placed one.

One more, the **looks** layer ([placed-container-looks](plan/solved/placed-container-looks.feature.md)), picks a
treasure's container: rolled (by depth, as on any level) or one of `CONTAINER_NAMES` (`src/world/containers.h`), a
coffin's lid also plain or inscribed (a cursed coffin). The stance still rolls.

Rules and the editor's palettes: [tools/editor/readme.md](../tools/editor/readme.md#decor-and-decal-layers),
[looks layer](../tools/editor/readme.md#looks-layer).

Fixtures and objects stand in `Empty`, `HalfWater`, `Sand` or `Quicksand`. A cell can hold one of each: an inscription over a chest, a
lever behind a monster spawn. Interact takes the chest first, then the fixture (pull the lever, read the inscription)
with the next press, then a mirror (turn it); a gate (exit, teleporter) comes last. Wall spikes block their cell when out, so `levelcheck` warns
about a monster spawn, ladder, gate, door, treasure (trapped too) or mirror in front of them; a key or a floor trap there is fine. The map
screen shows one symbol a cell, the object's, else the fixture's.

Water rules (the game, `levelcheck` as far as it models them): the player wades through half water at half speed and
cannot sprint or jump while standing in it (the Favour of Hapi skill and the Blessing of Hapi potion allow both) (jumping into it is fine, so a pool can be a trap); a fall into it lands like on a
floor. Walkers wade too: slowed (scarabs, the worm, the mummy), unaffected (rats, Anubis), or faster (a swimmer: the
crocodile and Sobek 2.5 times their land speed, the hippo and Seth 3, the cobras and Apep 1.25; `waterSpeed`,
`Wading` and `KINDS` in `src/world/monster_kinds.*`). An arrow hits a monster standing in water for half its damage. Traps
work in it and show through the surface. `levelcheck` warns about half water that is not on deep water or a wall,
deep water not under water, a ladder that goes down into water (one may start in it), a crocodile, hippo, Sobek or Seth not
in or next to water and a lotus not in half water.

Sand rules (plan desert-chapter): a drift of sand (`,` in the structure drawing) slows the player's walk to 3/4 and
stops the sprint; jumping is fine. Quicksand (`%`) looks like sand (a slow swirl up close): standing in it the player
sinks, slower each second, stuck after 3 s, hurt from 5 s (5% of the max HP a second); a jump frees them, for twice a
jump's stamina. A sand fall (a rock fall with attribute 1, `&`) pours sand on whoever stands under it (40, a short slow)
and turns its cell to sand. A sand worm placed in sand lies under it and comes up under the player. `levelcheck`
counts sand and quicksand as a little more costly on the path, jumps a quicksand cell like a trap but never lands in
quicksand nor walks on from quicksand into more of it (the walk there stops short of a cell), and warns about
sand or quicksand not on a wall and a run of more than 2 quicksand cells.

Sandstorm halls ([sandstorm-halls](plan/solved/sandstorm-halls.md)): a sand gust (`SandGust`, `{`, in the wall layer;
attribute: speed 0 normal, 1 fast, 2 slow, plus 3 for a wind to the left; value: phase) blows sand along the row
through its cell on a cycle (2 s of every 7 s at normal speed; the hiss and the grit 1.2 s before). A gust pushes the
player downwind faster than they walk, and the light monsters (the small walkers, the flyers); arrows drift, the haze
closes in, the other sounds go quiet. Mark every cell of the hall; the cells left out are its shelters: the game draws
a pillar on the side the wind comes from. Runs of 2 or 3 gusts between shelters (in sand at most 2 at normal speed),
the upwind runs at a higher phase so the gust rolls down the hall. `levelcheck` counts a gust as a little more costly
on the path, and warns about a run that cannot be crossed against the wind in one calm, blows both ways, has no floor
or spikes, a death trap or quicksand in it, or blows the player into a drop, a trap or quicksand. Halls today: lvl33
row 40 (sand, the wind to the right), lvl34 row 31 and lvl36 row 15 (stone, to the left; lvl36's slow).

Sunlight rules ([sunlight-mirrors](plan/solved/sunlight-mirrors.md), `src/world/sun_beam.h`): a light shaft (`l`, attribute: its way, 0 down, 1
left, 2 up, 3 right) sends a beam of sunlight through open cells until rock, deep water or a gate not fully open stops
it; the player does not. A mirror (`(`, value: the way it points) sends a beam that falls on it on that way: pointing
the way the beam goes it stands edge-on and lets it by, pointing back it catches it. Interact turns a mirror a quarter
turn clockwise (down, left, up, right). A sun disc (`a`, a fixture, attribute: lock colour) a beam falls on is lit for
good and opens every gate of its colour, as a lever. A beam burns mummies standing in it (fire, `SUN_BURN_FIRE`) and
counts as light for the camel spiders. `levelcheck` models the beam: turning a mirror lights every sun disc a beam can
reach with the mirrors the player reaches from there (with the colours open by then) set as needed; it warns about a
light shaft without rock behind it, a mirror in front of an inscription, a sun disc no beam can ever reach and one lit
as the level starts.

Level files are text, version 7: a `DCLEVEL 7 40 47` header, the level's zone (`zone cisterns_of_sobek`, or
`zone none`; [Zones](#zones)), an optional level title (`title The Hall of Drifts`, the rest of the line; every campaign
level has one, plan campaign-texts), the structure as a drawing (a shaped cell with its own glyph), then the lists of fixtures, objects, props, decals,
shapes and container looks (format: [tools/editor/readme.md](../tools/editor/readme.md#file-format)). Save games hold
the same block for the current level, with the fixtures' live state (a pulled lever, the wall spikes' state). Old (v1
to v6) level files and saves still load, converted as they are read (a v2 file's wall types move to the wall layer, a
v3 file's cells are all unset, a v4 file has no zone, a v5 file no shapes, a v6 file no placed looks); `levelcheck`
warns about a file older than v7, and `bin/levelconvert FILE...` (built by `make`) rewrites one as v7. Water: [crocodiles-and-flooded-cells.md](plan/solved/crocodiles-and-flooded-cells.md).

## Single cells by hand

`tools/level/lvledit.py LEVEL OP...` sets or clears one cell's object or back-wall fixture in place (`obj COL ROW TYPE
ATTR VALUE`, `wall ...`, `noobj COL ROW`, `nowall COL ROW`; `show COL ROW` prints the cell), the columns and rows as
`levelcheck --map` prints them, the type numbers from `levelcheck --legend`. The rest of the file stays as it is: a
quick edit of a campaign level without the editor. Run `levelcheck` after.
