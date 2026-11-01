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
