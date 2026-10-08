# Shared assets

Status: done 2026-10-08. Idea: load each file once and share it, instead of once per user.

## Seen duplicates

* **Fonts**: `fonts/papyrus.png` is decoded and uploaded 5 times: `loading`, `font`, `status`, `hudBody`,
  `hudSmall` (`loadFonts`, `LoadLoadingScreen`). `Font::Load` loads its own texture each time and only the
  size or spacing differs.
* **Monster models**: `loadMonsterTypes` calls `type.model.Load(kind.model, ...)` once per type, so a shared
  `.md3` (and its clips) is parsed once per kin:
  * scarab ×3 (normal, giant, boss), bat ×3 (normal, giant, vampire), scorpion ×3 (normal, giant, queen),
    cobra ×3 (normal, giant, Apep), rat ×2, anubis ×2, crocodile ×2 (normal, Sobek).
  * Only the texture differs.
* **Mechanisms**: `key.md3`, `gate.md3`, `lever_base.md3` are loaded once per lock colour, and `gate.md3`
  once more for the boss gate. Only the texture differs.
* **Traps**: `spikes.md3` is loaded twice (spikes, death trap). Only the scale differs.

## Ideas

* Font: one texture shared by every size (a texture cache keyed by path, or `Font` takes a `const Texture&`).
  The display lists stay per size.
* Models: split the parsed mesh and animation (shared, keyed by path) from the per-user texture binding and
  compiled lists. Check whether `Compile()` bakes the texture into the display list. If it does, either
  keep one compiled list per texture or bind the texture at draw time.
* A small `AssetCache` (path → `shared_ptr`) in `Assets` for both, if more than one place needs it.

## First step

Measure before changing anything: the load time and RSS/GPU memory at the main menu, then the same with
the duplicates removed. If the gain is small, drop the plan.

## Done

Measured with a scenario that loads `tests/levels/classic1` and quits, under Xvfb (llvmpipe: GPU memory counts in
RSS), `/usr/bin/time`, 5 runs each:

| | wall time | peak RSS |
|---|---|---|
| before | 9.7-10.2 s | 1.50 GB |
| after | 6.1-6.3 s | 1.06 GB |

* **Monster models**: the clips moved into `CharacterClips`, held by `shared_ptr` in `CharacterModel`.
  `loadMonsterTypes` loads a model once; a later kind with the same model and clip files `Share`s its clips with
  its own texture and sounds. `CharacterModel::Show` binds the type's texture at draw time (the display lists
  never held it).
* **Fonts**: `Font` holds a `FontSheet` (texture and glyph edges) from a cache keyed by path (`font.cpp`), kept
  while a font of it lives. Covers the game, the UI screens, the journal hands and the tools.
* **Mechanisms**: one key, gate and lever plate model; `drawKeyTile` and the others pass the lock colour's texture
  (or the boss gate's) to `AnimatedModel::Show`.
* **Traps**: the death trap is a copy of the spikes `Trap`, sharing its model, with its own scale.
