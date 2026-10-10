# No torches under water

Status: implemented 2026-10-10, not play-tested yet (see [Implemented](#implemented-2026-10-10)). From the user: no wall torches under water.

## Today

* `scatterTorches` (`src/world/decor_scatter.cpp`) places a torch on any non-wall cell whose tile allows one
  (`tileDef(...).torch`), skipping braziers, lamps and Thoth props, at least `TORCH_MIN_GAP` apart, with
  `TORCH_CHANCE_PERCENT`. It does not check the water: a flooded cell (`Structure::DeepWater`, `Structure::HalfWater`,
  [crocodiles-and-flooded-cells](solved/crocodiles-and-flooded-cells.md)) can get a burning torch.
* The torch flame sits at `TORCH_FIRE` (`src/world/dungeon_render_decor.cpp`, y 0.68 of the cell), and it lights the
  cell (`Lighting::TORCH`).

## Idea

* Skip deep water cells in `scatterTorches`.
* Half water: kept. The flame (y 0.68 of the cell, about 27 units above the floor) clears the surface
  (`RenderConfig::WATER_SURFACE`, 3 under the floor) by far.
* Moving the torches changes the scatter. `lastTorch` and the gap shift the torches further along the row, so
  flooded levels look different afterwards. Check them with screenshots.
* A unit test over the campaign levels: no torch stands in deep water.

## Decided (2026-10-09)

* **Deep water:** skipped in `scatterTorches`.
* **Half water:** kept; the torch head clears the water.
* **Dark stretch:** fine. No moving the torch to a dry cell; the gap puts the next torch on a dry cell nearby anyway.
* **Check:** a unit test over the campaign levels' scatter: no torch in a `Structure::DeepWater` cell.

## Open (for the implementer)

* Screenshots of the flooded levels before and after: the scatter shifts along the rows.

## Implemented (2026-10-10)

* `scatterTorches` (`src/world/decor_scatter.cpp`) skips `Structure::DeepWater` cells.
* `tests/unit/decor_test.cpp`: no torch in deep water on any campaign level, and some still over half water.
* The scatter shifted on 16 levels (8, 9, 16-25, 27-30): `git diff` of the golden dumps
  (`tests/unit/golden/decor_lvlNN.txt`, from [scenarios-to-unit-tests](solved/scenarios-to-unit-tests.md)) lists every
  torch that went and every decal that moved into its place. No screenshots taken: the flooded cells are deep in the
  levels, no scenario walks there; look at them in play.
