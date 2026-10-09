# No torches under water

Status: draft 2026-10-09 (idea, not decided). From the user: no wall torches under water.

## Today

* `scatterTorches` (`src/world/decor_scatter.cpp`) places a torch on any non-wall cell whose tile allows one
  (`tileDef(...).torch`), skipping braziers, lamps and Thoth props, at least `TORCH_MIN_GAP` apart, with
  `TORCH_CHANCE_PERCENT`. It does not check the water: a flooded cell (`Structure::DeepWater`, `Structure::HalfWater`,
  [crocodiles-and-flooded-cells](solved/crocodiles-and-flooded-cells.md)) can get a burning torch.
* The torch flame sits at `TORCH_FIRE` (`src/world/dungeon_render_decor.cpp`, y 0.68 of the cell), and it lights the
  cell (`Lighting::TORCH`).

## Idea

* Skip deep water cells in `scatterTorches`.
* Half water: open. The flame (y 0.68) may sit above the surface. Choose one: skip these cells too, or keep them
  only when the torch head clears the water.
* Moving the torches changes the scatter. `lastTorch` and the gap shift the torches further along the row, so
  flooded levels look different afterwards. Check them with screenshots.
* Optional: a check (a unit test over the campaign levels, or `levelcheck`) that no torch stands in water.

## Open

* Half water: skip or keep (see above).
* Is a dark flooded stretch fine, or should the torch move to the nearest dry cell in the row?
