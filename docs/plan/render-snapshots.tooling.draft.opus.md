# Render snapshots: reference screenshots and a pixel diff for the GL port

Status: draft 2026-11-18, size M, needs nothing first.

## Problem

The GL port ([gl-core-foundation](gl-core-foundation.feature.draft.opus.md) and the plans after it) rewrites how
every pixel is drawn, and must not change the look. Nothing checks that today: scenarios take screenshots, but no
test compares them with a reference (the only golden files are text dumps of decor, `golden_test.cpp`). Every port
step needs a quick "did the picture change, and where" answer, without the user play-testing each one.

## Plan

* A set of snapshot scenarios (`tests/scenarios/snapshots/`, skipped by `make test` like `solved/`), each a fixed
  scene with `screenshot` lines, together covering every draw path that has its own GL code:
  * the world: a few campaign levels per zone (walls, floors, water, sand, webs, drips, sun shafts, mechanisms,
    decor, fire, outlines, inscriptions), monsters mid-animation (models, motion blur), particles, traps, items;
  * the effects: brightness, ink, lighting, motion FX (sprint, damage), the death and ending screens;
  * every UI screen ([ui.md](../ui.md)): menu and options, credits, inventory, riddle, map, journal (with the page
    curl and its lit 3D page), skills, Heka, boss bar, HUD, zone banner, interact prompt, level gem, status box.
* `make snapshots`: runs them (xvfb, llvmpipe, `--no-sound`, as `tools/run_scenarios.sh` does) into
  `tests/out/snapshots/`.
* `make snapshots-ref`: writes the reference set to `tests/snapshots/ref/` (committed PNGs; keep the count and the
  resolution moderate, e.g. 640x360, so the repo stays small).
* A diff tool (`bin/snapdiff`, C++ with the vendored `stb_image`/`stb_image_write`, no new dependency): per image
  the share of pixels off by more than a threshold, the largest channel error, and a diff PNG (changed pixels in red
  over a dimmed copy). Exit code non-zero above a per-image tolerance. `make snapshots` ends with its table.
* Check first that two runs of the same build give identical images (scenario clock, seed, one llvmpipe thread).
  A source of noise (real time, an unseeded random, a frame-rate based value) is fixed at its root, not hidden by a
  wider tolerance.
* Docs: a "Render snapshots" section in [testing.md](../testing.md): when to run them (any change under
  `src/render/`, `src/graphics/`, `src/ui/`), how to accept a wanted change (`make snapshots-ref`, review the diff
  PNGs, commit).
* Take the reference set on the current `master` before any port plan starts.
