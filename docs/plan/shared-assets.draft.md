# Shared assets

Status: draft 2026-10-08. Idea: load each file once and share it, instead of once per user.
Seen in the loader code (`src/state/assets.cpp`), not measured: no numbers for memory or load time yet.

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
