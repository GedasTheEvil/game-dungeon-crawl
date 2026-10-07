# Blender review tooling: easier model and animation review for the agent

Status: draft 2026-10-07.

## Problem

Models are built headless (`blender -b --python`, see [remodeling.md](../remodeling.md)); the agent reviews them from
`render_sheet.py` stills tiled with `montage`. Weak spots:

* Motion: stills show poses, not timing, arcs or foot sliding. The agent cannot play GIFs or video.
* Organic shape and proportions are judged from memory, with no reference.
* Blender renders do not match the game's renderer (lighting, camera, texture filtering).
* Every headless run pays Blender's startup cost (minor).

The Blender MCP (live GUI session) is optional: handy for quick live iteration, not needed for the headless flow.

## Ideas (by payoff)

1. **Reference images.** `docs/plan/references/<model>/` with photos or concept art, kept out of git like
   `docs/plan/screenshots/` (own `.gitignore` with `*`; the folder is already set up locally). The agent reads
   images and compares renders against them.
2. **Model viewer screenshot mode.** `tools/model-viewer/viewer.cpp` CLI, e.g.
   `model-viewer --shot <md3> --clip walk --frames 0,4,8 --yaw 90 --out dir/`, which writes in-game-renderer
   screenshots and exits. Fits with [model-viewer-speed-and-text.md](solved/model-viewer-speed-and-text.md).
3. **Motion diagnostics as images** (extend `tools/blender/render_sheet.py`):
   * Onion skin: N frames alpha-blended into one image.
   * Joint trajectory plots (matplotlib or a plain PIL/ImageMagick plot): foot, hand and tail tip paths over the clip
     (side and top views), for arcs, foot sliding and loop seams.
   * Loop seam check: first vs last frame difference (numbers + diff image).
4. **More numeric checks** next to the floor-penetration check: self-intersection count per frame, bounding box
   change vs the previous export, slide distance of planted feet.
5. **Optional:** a Blender 5 instance with the MCP add-on kept open during modelling sessions for fast live iteration.

No Blender plugins needed (Rigify, Mixamo etc. do not fit procedural scripts). `ffmpeg` and ImageMagick are
installed (ffmpeg for videos the user watches).

The implementing agent picks the order.
