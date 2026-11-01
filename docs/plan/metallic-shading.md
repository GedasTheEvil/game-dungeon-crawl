# Metallic shading

Status: draft 2026-11-01, needs nothing first. From the user. Brightness settled with the user: the player's own light
(`Lighting::PLAYER`, `src/graphics/draw.cpp`, always on) gives metal a medium shine where no other light reaches;
torches nearby make it bright. Keep it simple: no extra rim term unless the screenshots show metal still reads as
paint.

## Idea

Today every model is one material: its texture, lit by the same wrapped Lambert (`src/graphics/lighting.cpp`). Metal
(gold, bronze, copper, silver) looks like painted wood. Make metal parts shine, without splitting models.

## Why not composite parts

The ring's gem is a second model (`Item::loadPart`, `drawPartScaled`) because of its draw: see-through, no depth write,
an additive glint. Metal is opaque, it only shades differently. A part per metal piece would split every model, and on
animated monsters each part would have to follow the same frames. The MD3 loader merges the surfaces into one buffer.

## Plan

* **Mask in the texture alpha.** No item, monster or mechanism texture uses alpha today (0 of 121, checked
  2026-11-01). Alpha = metal strength (255 = none, matte; lower = more metal, or the other way, implementer's choice,
  but old opaque textures must stay matte). No new files, no second texture unit.
* **Bakers.** The Blender palettes (`tools/blender/models/*.py`) get a `metal` flag per colour entry; the bake writes
  it to alpha. A helper shared by the bakers, documented in `docs/modeling.md`.
* **Shader** (`FRAGMENT_SRC`): where the mask is on: diffuse darker (metal is mostly reflection), a Blinn-Phong
  highlight per light in the existing loop, tinted by the base colour (gold gives a yellow highlight), eye vector
  `-vPos`. The player's light is one of the loop's lights, so metal always has a highlight; tune its strength so
  that with only the player's light it is medium, not blinding (between matte and a torch-lit shine). A rim term
  only if needed (see Status). Toon mode: one hard highlight band. Emissive draws unchanged.
* **Alpha out.** The shader writes `base.a` as output alpha; with the mask that would leak into blending (fades, the
  gem's pass). Output `gl_Color.a` for the lit path, or only use `base.a` where a draw wants texture alpha.
* **Check texture loading** keeps the alpha channel (`Texture::LoadPNG`) and no draw enables alpha test/blend on the
  masked models by accident.

## Steps (each its own commit, screenshots per step)

1. Shader and mask; ring bands (copper, silver, gold: three textures), khopesh blade, an amulet's gold bail.
2. Keys, gates and mechanisms: bronze portcullis, mirrors, sun discs, key bows.
3. Boss and monster accents (Sobek's gold scutes and collar, the priest's ankh and finger stalls, Serket's gold),
   re-bakes only.

## Tests

* Scenario screenshots: a ring and a khopesh by a torch, normal and toon mode, before and after; a gate in a dark corner.
* Unit: the shader still compiles (if a GL-free check exists), baked textures of step 1 have alpha < 255 somewhere.

## Implemented (2026-11-01)

* Mask: alpha 255 matte, 255 - 255 x strength metal (`common.metal(rgb[, strength])` in a palette; `make_material`
  keeps the strengths, `bake_texture` / `save_texture` bake and write them, `bake_metal_mask`). A script's own node
  material: `metal_palette` + `"metal_mask_material"` (the crocodile's collar). Documented in docs/modeling.md.
* Shader: `uMetalMask`, set by `AnimatedModel::Show` around its draw (every model, nothing else: decals and
  sprites keep their see-through alpha). Diffuse x 0.7 on full metal, Blinn-Phong (exponent 10, x 2.2) per light,
  tinted by the base colour; toon: the bands as the paint's (x 0.75) and one hard highlight band. No rim term: the
  player's light alone gives the medium shine. Emissive draws unchanged; the lit and emissive paths output
  `gl_Color.a` on a masked draw.
* Alpha out, loading: `LoadPNG` keeps RGBA. The journal's photo draws models without the shader and without
  blending: no shine there, and the alpha does nothing.
* Inventory (after the play-test, 2026-11-01): its models are drawn with the scene shader too
  (`Inventory::Draw`): ambient 0.78, so matte items stay as flat as before, and one far light above, left and in
  front (`MODEL_LIGHT`), so metal catches a highlight as the models turn. `Lighting::begin` reads the projection: in
  an orthographic one the eye looks down -z (`uOrtho`), for the two-sided normal and the highlight.
* Step 1 wider than listed: every gold, bronze and blade colour in `items.py` (all weapons, amulets, the chest's
  trims, gilded potion vessels), the three ring bands. Step 2: `mechanism.py` gold, bronze and the polished mirror
  (keys, gates, levers, mirrors, sun discs, spikes, the ring transporter); the wood grain got its own matte colour.
  Step 3: Sobek's scutes and collar, Serket's carapace (0.6) and sting, the mummy priest's gold.
* The re-exports of `items.py` and part of `mechanism.py` gave new UV layouts (geometry unchanged, checked per
  vertex): those `.md3` files went in with their textures. `rings.py` now exports ring geometry different from the
  committed `.md3` files (UVs the same): only the ring textures were replaced, the `.md3` files kept.
* Tests: `tests/scenarios/metallic_shading.txt` (khopesh by a torch, normal and toon, the chest and coffer, key,
  gate, mirror, sun disc, the inventory's weapons, amulets and rings), `tests/unit/metal_mask_test.cpp` (the baked textures have metal, matte ones none). No
  GL-free shader compile check exists; the scenario compiles it. Play: `bin/game --play tests/play/metallic-shading.txt`.
