# Placed decor: a decor layer the editor paints

Status: draft, ready. Needs nothing first. From the user.

## Today

`scatterProps` (`src/world/decor_scatter.cpp`) puts the props (`DECOR_NAMES`, `decor.h`) on load: 30% of free dry
floor cells (`DECOR_CHANCE_PERCENT`), 50% of free half water cells, by tier and a hash of the cell. The coffin goes on
every mummy spawn. Wall decals (`DECAL_CHANCE_PERCENT`, 35%) and torches are scattered on their own. A level designer
cannot pick a prop or keep one off a cell.

## Idea

A fourth layer per cell, **decor**: what the prop on the cell is. Not the wall layer: the wall layer is for back wall
things that do something (lever, inscription, spikes), and most props stand on the floor. A layer of its own also
lets a prop share its cell with a fixture or an object, which the scatter never does.

Three states per cell:

* **unset** (default, every cell of today's levels): the scatter rolls as now (30% / 50%, tiers, coffins).
* **bare**: no prop, the scatter skips the cell.
* **a prop**: that one, with its mirror. The jitter (`offsetX`) still comes from the cell hash.

A placed prop ignores the tiers: the designer picks it. Coffins on mummy spawns stay automatic.

Use: a row of coffins next to the one with the real mummy, the decor lotus among the blue lotus, a statue where a
room needs it, a bare corridor before a trap.

### Decoy: the sleeping mummy

An empty coffin next to a mummy's coffin gives the mummy away. Add a prop **mummy in coffin**: the coffin with the
mummy model lying in it, frozen in its entombed pose, that never wakes. Draw it with the monster model (no new Blender
model if the entombed pose can be drawn static). Then a room of five coffins holds one real mummy. Further decoys (a
coiled cobra, a chest without a mimic) are the implementer's choice or a later draft.

## Format and code

* Level format v4: a `decor N` list after `objects N`, `column row prop mirror`, only cells that are not unset.
  `prop` is the `DECOR_NAMES` index, `-1` for bare (or a name; the implementer's choice, glyphs are for tools only).
  v3 and older load with every cell unset; `levelconvert` writes v4.
* The level holds the layer (`LevelGrid` / level loading, `level.h`); `scatterProps` reads it before rolling.
* Checker (`bin/levelcheck`): warn on a dry prop in water or with no floor under it, a water prop out of half water,
  a web with no ceiling, a prop in rock. A prop in the same cell as an object or fixture: warn where they would
  overlap on screen (a statue and a chest), the implementer decides which pairs. `--map` prints a `decor` drawing,
  `--legend` the prop names.
* Editor (`tools/editor`): a `Decor` layer button (key `L` goes round four layers), a palette of the props plus
  `Unset` and `Bare`, an icon per prop in `tools/editor/icons/make_icons.py`. Right click picks a cell's decor.
  Update `tools/editor/readme.md` (layers, palette, file format).
* Docs: `docs/levels.md` (format), glossary row for **decor layer** and **decoy**.

## Tests

* Unit: v4 round trip; a v3 level loads all unset and scatters as before (same props, same cells); bare and placed
  cells beat the roll.
* Scenario with a screenshot: a row of coffins, one real mummy, one sleeping mummy decoy; walk past them.
* Every campaign level still passes `bin/levelcheck` with no warnings.

## Open points

* Placed decals and torches in the same layer (or a decal field)? Left out here; a later draft if wanted.
* A level-wide "no random props" flag instead of painting bare cells? Not needed for now.
