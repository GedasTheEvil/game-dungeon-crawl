# Placed decor: decor and decal layers the editor paints (props, decals, torches)

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

### Decal layer

Props and decals are scattered apart today (`scatterDecals`): a cell can have both. Links: a cell with a prop gets no
floor decal (`DecalAnchor::Floor`, nothing grows behind the prop), a torch blocks the free decals, and a cell whose
object or fixture says `TileDef::decals = false` gets none. Decals do go behind monsters, items and fixtures.

So a fifth layer, **decal**, the same three states: unset (the 35% roll as now), bare, or a decal (`DECAL_DEFS` index)
with its mirror. Its height comes from its anchor as now; x from the cell hash, or stored (the implementer's choice).
A placed decal ignores the tiers and the prop rule; the checker warns where its anchor does not fit (a ceiling decal
with no rock above, a floor decal with no floor, in water or behind a prop), behind a torch, and on a cell whose tile
says no decals.

Use: hieroglyph columns around an inscription, vines over a hidden passage, a winged sun over the ankh.

### Torches, in the decal layer

The wall torch is back wall dressing like a decal, and today's rules tie it to the decals: it takes the middle of the
wall, so no free decal goes with it (`scatterDecals`). A prop stands on the floor and may share the cell, except the
brazier, the lamp and Thoth's statues (`scatterTorches`: they bring their own fire, the crowns reach up to the torch).
So the torch goes in the decal layer, as a second field of its cell with the same three states: unset (the 25% roll,
`TORCH_CHANCE_PERCENT`), none, or a torch. A cell can hold a torch and a ceiling or floor decal.

* Scatter: a placed torch counts for the row's gap (`TORCH_MIN_GAP`), so random torches keep their distance from it;
  placed torches ignore the gap. A cell set to none gets no torch.
* Editor, in the `Decals` palette: `Torch` and `No torch` set only the torch field; a decal brush sets only the decal
  field; `Unset` and `Bare` set both. Painting a torch over a free decal, or a free decal over a torch, clears the
  other one (they would overlap).
* Checker: warn on a torch with a free decal, with a brazier, a lamp or a Thoth statue, in deep water, on a cell whose
  tile says no torch (`TileDef::torch`). Placed torches may stand closer than the gap: the designer's choice.
* A placed torch lights like a scattered one (check the light setup reads `layout.torch` only).

### Decoy: the sleeping mummy

An empty coffin next to a mummy's coffin gives the mummy away. Add a prop **mummy in coffin**: the coffin with the
mummy model lying in it, frozen in its entombed pose, that never wakes. Draw it with the monster model (no new Blender
model if the entombed pose can be drawn static). Then a room of five coffins holds one real mummy. Further decoys (a
coiled cobra, a chest without a mimic) are the implementer's choice or a later draft.

## Format and code

* Level format v4: a `decor N` list after `objects N`, `column row prop mirror`, then a `decals N` list,
  `column row decal mirror torch` (torch: unset, none, torch), only cells with either field set.
  `prop` / `decal` is the index, `-1` for bare (or a name; the implementer's choice, glyphs are for tools only).
  v3 and older load with every cell unset; `levelconvert` writes v4.
* The level holds both layers (`LevelGrid` / level loading, `level.h`); `scatterProps` and `scatterDecals` read them
  before rolling.
* Checker (`bin/levelcheck`): warn on a dry prop in water or with no floor under it, a water prop out of half water,
  a web with no ceiling, a prop in rock. A prop in the same cell as an object or fixture: warn where they would
  overlap on screen (a statue and a chest), the implementer decides which pairs. `--map` prints a `decor` drawing,
  `--legend` the prop names; the same for decals.
* Editor (`tools/editor`): `Decor` and `Decals` layer buttons (key `L` goes round five layers), each a palette plus
  `Unset` and `Bare`. Icons: one per prop in `tools/editor/icons/make_icons.py`; the decals can use their cell of
  `decals.png`. Right click picks a cell's decor or decal. The right panel's layer button row needs room
  (`STRUCTURE_BUTTON` .. `OBJECTS_BUTTON`).
  Update `tools/editor/readme.md` (layers, palette, file format).
* Docs: `docs/levels.md` (format), glossary rows for **decor layer**, **decal layer** and **decoy**.

## Tests

* Unit: v4 round trip; a v3 level loads all unset and scatters as before (same props and decals, same cells); bare
  and placed cells beat the roll, in both layers and for torches; random torches keep the gap to a placed one.
* Scenario with a screenshot: a row of coffins, one real mummy, one sleeping mummy decoy; walk past them.
* Every campaign level still passes `bin/levelcheck` with no warnings.

## Open points

* A level-wide "no random props" flag instead of painting bare cells? Not needed for now.
