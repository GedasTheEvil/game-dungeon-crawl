# Model viewer: texture lookup by the full name first

Status: done 2026-10-07, from the user.

## Problem

`tools/model-viewer/viewer.cpp` finds a model's texture from its file name, cut at the first `_` (`ParentStem`):
`models/monsters/anubis_att.md3` -> `textures/monsters/anubis.png`. That suits a monster's clips, which share one
texture (`anubis`, `anubis_att`, `anubis_die`).

It fails for models whose own name has an underscore and that have a texture of their own: every decoration
(`models/decorations/decor_osiris.md3` -> `textures/decorations/decor.png`, which doesn't exist), so they show
`textures/null.png`. The same goes for `items/treasure_chest`, `items/composite_bow`, `items/sling_stone`, the
`mechanisms/` and `ladders/` models, and any monster named with an underscore.

## Idea

`LoadTextureForModel`: try the texture of the full stem first (`textures/<category>/<stem>.png`); only if that file
doesn't exist, fall back to the parent stem (before the first `_`), as today; then `textures/null.png`.

## Also to check

* `ScanSiblingModels` uses the same `ParentStem`: the space key cycles through every `decor_*` model as if they were
  one model's clips. Maybe the same rule there: a sibling is a clip only if the full-stem texture is missing (or only
  for the known clip suffixes `_att`, `_die`, ...).
* Monsters whose texture is not their model's name (the giant rat, bat, scarab and cobra share a model with their own
  texture, `MONSTER_DEFS` in `src/state/assets.cpp`): the viewer can't know that from the file name. A `--texture`
  argument, if needed.
* Log which texture was picked, so a wrong one is easy to spot.

A tooling plan: no user confirmation needed; once implemented and checked it goes straight to `solved/`. Fits with
[model-viewer-speed-and-text.md](model-viewer-speed-and-text.md).

## Done

* `BaseStem`: the shortest cut of the stem at a `_` that names a `.md3` in the same directory, else the whole stem.
  Replaces `ParentStem` for clip grouping too, so `decor_*` models are one clip each and `egg_cluster_att` groups with
  `egg_cluster` (the first-`_` cut made it `egg`).
* `TextureCandidates`: `<stem>.png`, `<base>.png`, then the variants `<base>_*.png` (`anubis_boss`, `rat_giant`,
  `key_blue`, `lever_base_gold`); `T` cycles them, which covers the shared-model monsters without a `--texture`
  argument. The pick is kept across clip switches. Falls back to `textures/null.png`. The picked texture is logged and
  shown in the stats panel.
