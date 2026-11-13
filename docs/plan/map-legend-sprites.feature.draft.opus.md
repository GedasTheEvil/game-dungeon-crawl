# Map: legend symbols from a sprite sheet, shrines and other missing marks

Status: draft 2026-11-13, size M, needs nothing first. From the user.

## Idea

Two things on the draft map (`src/ui/map_view.cpp`, [ui.md](../ui.md#draft-map)):

1. The symbols ("legend items", `MapMark`) are pencil strokes in C++ (`symbol()`, about 240 lines). Move them to a
   sprite sheet, so a symbol is corrected in an image editor, without touching code.
2. Bug: a shrine does not show on the map. The map reads only a cell's object and fixture (`markOf`, `shownTile`),
   never its prop (`decoration.decor`), and a shrine is a prop (`shrineGod`, `src/level/shrines.h`). Other things are
   missing the same way (audit below).

## Missing marks (audit, 2026-11-13)

What the player can use or must watch out for, against `markOf`:

| Thing | Where it lives | On the map today |
|---|---|---|
| Shrine (Isis, Montu, Ptah, Amun, Serket) | prop `DECOR_SHRINE_*`, verb Pray | no (the reported bug) |
| Stargate | prop `DECOR_STARGATE`, verb Dial | no |
| Dial device | prop `DECOR_DIAL`, verb Press | no |
| Brazier | prop `DECOR_BRAZIER`, verb Light (with a bow); light keeps camel spiders off | no |
| Ceiling spikes, ceiling death trap | fixtures `CeilingSpikes`, `CeilingDeath` (plan ceiling-spikes) | no: `markOf` has no case |
| Opened chest | `containers[cell].opened` | drawn as a full one (X) |

Must: the shrines (one mark "Shrine", or one a god: the implementer's choice; the god's name in the legend row helps
the player plan a blessing), the Stargate, the dial, both ceiling traps. Nice to have: the brazier, an opened chest
drawn fainter or crossed out. Stays off the map on purpose: monsters (but a mimic as a chest), quicksand (looks like
sand), plain decor.

Do the audit again when implementing: go through `DungeonTileType`, `isWallType`, the props `InteractVerb`
(`src/sim/dungeon_interact.cpp`) gives a verb for, and `Structure`.

## Plan

* **Mark table apart from the drawing.** Move `MapMark`, `MARK_NAMES`, `markOf` and `doorMark` out of the anonymous
  namespace into a pure header (`src/level/map_marks.h` or `src/ui/map_marks.h`; no GL), so unit tests reach them.
  `markOf` takes the cell's prop too (the runtime `decoration.decor[...]` type, not only `dressing.prop`: the game
  reads the first for the Pray verb). Order of what a cell shows: object, else fixture, else prop.
* **One table row a mark**: name, atlas cell, tint rule (none, lock colour, fixed colour), turn rule (none, by
  `BeamDir` / `gustDir`), state cell (the second sprite of a mark with two looks). No per-mark draw code is left.
* **Sprite sheet** `media/textures/ui/map_marks.png`: a grid of square cells (128 px as `hud_icons.png`), drawn with
  `ui::atlasIcon`, `TexFilter::Mipmapped` (the symbols are drawn far smaller than their cells). One draw path for the
  sketch and the legend slip (the legend row keeps its sample tile, so it shows the colour and state first seen).
* **State and colour without a sprite per case**:
  * lock colour (key, gate, lever, sun disc): a neutral pencil sprite, tinted with `lockPencil(attr)`;
  * two looks, a cell each: gate closed / open, boss gate sealed / open, lever up / pulled, rock fall armed / fallen;
  * direction (light shaft, mirror, sand gust): one sprite, turned by quarter turns;
  * fixed two-colour marks (entrance red, exit blue, teleporter teal, name gate gold, sphinx gate): the colour is
    in the sprite; entrance and exit get a cell each.
* **The first sheet**: bake today's strokes so nothing changes at first, in the pencil look (graphite on clear, the
  wobble kept). Then add the new marks in the same hand. After that the PNG is the source of truth, edited by hand:
  no generator in `tools/textures/` that would overwrite the corrections (unlike the other UI atlases; say so in
  ui.md). A layout note for the editor: which cell is which mark (ui.md table, or a labelled guide image beside it).
* **Stays in code**: walls, hatching, outlines, water, the "you are here" ring, the paper. The per-cell wobble of a
  symbol goes (every cell shows the same sprite): fine.
* Delete `symbol()` and its helpers (`arch`, `zigzag`, `columns`, `gate`) once the sheet draws everything.
* Docs: ui.md "Draft map" (the sheet, the cell layout, how to add a mark: a sprite cell + a table row + a `markOf`
  case), development.md "Where is X", the glossary rows for new marks.

Open points (the implementer's choice unless the user says otherwise):

* Hand-edited PNG (assumed above) or a generator script as for the other atlases. "Outside of the code" was read as
  "in an image editor".
* The mark names stay in the C++ table. A text file beside the sheet (`map_marks.txt`: cell, name, tint) would let
  a name or a new plain mark be changed without a build too; only worth it if cheap.
* A multi-cell prop (the Stargate ring is wider than a cell): one symbol on its cell, or a wide one.

## Tests

* Unit (`tests/unit/`, names plan map-legend-sprites; fails before the fix): a cell with each `DECOR_SHRINE_*` prop
  has a mark; so do the Stargate, the dial, `CeilingSpikes`, `CeilingDeath`.
* Unit, the guard against the next gap: every `DungeonTileType` but `NoObject` and `MonsterSpawn` has a mark, every
  prop with a verb in `InteractVerb` has one, every mark has a name and an atlas cell inside the sheet's grid.
* Scenario with a screenshot: a test level with every mark, the map open, the legend complete.

## Play-test

* `bin/game --level 9` (a Shrine of Isis), walk to the shrine, open the map `[M]`: the shrine has a symbol and a
  legend row.
* `bin/game --level 13` (Montu) or 7 (Amun): the same; with a mark a god, the row names the god.
* Any level: the old symbols (ladder, chest, key, gate, lever, traps) look as before, sharp at 1x and zoomed in.
* A level with a Stargate chamber and one with ceiling spikes: both show.
