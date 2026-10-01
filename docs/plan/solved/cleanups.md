# Stages 6 (part), 12 and 13: Game() in the renderer, smaller cleanups, unit tests

Status: implemented 2026-09-30, verified in play 2026-09-30. Stages 6, 12 and 13 of the
[code structure review](code-structure-review.md).

## Stage 12, smaller cleanups (`c2c8f15`)

* `cellHash` for the five copies of the decor seed hash; `Dungeon::isRock` for the two `rock` lambdas.
* `graphics/shader` (`linkProgram`) for the compile / link code the lighting and ink shaders copied.
* `bumpGate`: one locked-hint path for the boss gate and the coloured gates.
* `Assets::Load` (168 lines) split into one loader per asset group; `loadProp` replaces four copies.
* Named: the tile size and half size in the draw code, the row step from `ViewWindow::WIDTH` (`HUD_OFFSET_X`, a
  misnamed -400, is gone).
* Removed: the fullscreen flag nothing set, the resize handler's projection every frame overwrote.

Pixel-identical screenshots. Not done: `Scenario::parseLine` (174 lines, a 30-branch `if` chain) and `checkLevel`
(166 lines) stay long: each branch is short and they read top to bottom; a command table would move the length, not
remove it. The save format keeps its one version tag (`INV2`): adding tags to the other sections changes the files
and the old-save paths for no bug seen.

## Stage 6, part: the renderer without `Game()` (`1917b30`)

* `Ink` owns the toon setting (`Ink::toon()` / `setToon()`, `RenderSettings::Cartoon` removed); `Ink::begin` takes
  the window size; `Lighting` asks `Ink`. `lighting`, `ink` and `shader` joined `librender` (the layer check
  passes).
* `Game()` calls in `src/`: 370 (438 at the audit). The rest of the stage (how far `Game()` goes) stays open; the
  biggest users are the input (77), the scenario runner (55) and the draw code (37), which are the app layer that is
  meant to know the game.

## Stage 13, unit tests

Added with the stages that moved code into `liblevel`: 44 test cases in `tests/unit/` (level format, items and the
item bag with the save format, quick potions, progression, the tile / monster / lock tables, the level docs, the
jump arc, the checker's movement rules, loot).
