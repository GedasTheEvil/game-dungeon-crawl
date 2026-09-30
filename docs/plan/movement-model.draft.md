# Stage 10: one movement model

Status: planned 2026-09-30. Stage 10 of the [code structure review](code-structure-review.draft.md). The 14
differences between the checker's `Walker` and the game: [the audit](code-structure-review-audit.draft.md#walker-vs-game).

## Why

The checker (`src/world/level_check.cpp`) judges every campaign level with its own copy of the movement rules, and
`levelcheck --script` turns its path into a scenario that plays it in the real game. Today that replay fails on 2 of
the 15 campaign levels (lvl6, lvl8): the scripts walk by relative distances, the small differences add up, and the
player stops short of a gate's opening distance. The checker's constants are copies (`GRIP`, `GATE_STOP`, the
`wait 1400ms` for a gate), and some of its rules differ from the game's.

## Decisions, per difference

| # | Difference | Decision |
|---|---|---|
| 1 | The walker jumps only over a gap; the game hops spikes, death traps, rock falls on a floor too | **Match the game**: a jump edge over one hazard cell with a floor |
| 2 | The walker never jumps from a ladder cell; the game does | **Match** where the player stands on a floor (a ladder's foot). A jump from mid-ladder: not modelled (its reach depends on the held walk key, see 4) |
| 3 | A `jump` branch in `Dungeon::Move` no caller uses | **Remove** it |
| 4 | Jump height documented as ~0.45 tiles; the reach is not modelled | **Share**: the jump's peak, length and duration computed from the game's constants (`src/world/movement.h`), with `static_assert`s on what the walker assumes (no step up onto a ledge). The reach depends on the walk key held during the jump (stage 9), so the walker keeps "one cell over" |
| 5 | Jumps cost no stamina in the walker | **Keep**: stamina comes back while the player waits, so it slows a jump chain but never blocks it. Documented |
| 6 | The walker opens a keyed gate on touch; the game at `GATE_APPROACH`, after `GATE_OPEN_MS` | **Share the constants**: the script stops `GATE_APPROACH` minus a margin before the gate and waits `GATE_OPEN_MS` plus a margin |
| 7 | A gate with value 2 (opening) in a file: the game opens it on load, the walker treats it as closed | **Match**: only a closed gate (0) is closed in level data |
| 8 | A pulled lever (value 1) in a file opens nothing in the game; the walker still pulls it | **Match**: no pull edge for a pulled lever |
| 9 | A key of colour 5 (boss lock) opens the boss gates in the walker only | **Match**: a key opens only lock colours 1-4 (the checker still reports the bad colour) |
| 10 | Reaching the boss's cell counts as killing it | **Keep**: the checker cannot fight. Documented |
| 11 | Teleport | Already the same (`TELEPORT_ARRIVAL_X` is shared) |
| 12 | Traps: the walker counts the path cell; the game's death trap hurts into the next cells | **Keep** for now: a difficulty cost, not reachability. Noted for a difficulty review |
| 13 | Rocks: a cost only | **Keep**: timing and damage are not reachability |
| 14 | Whole cells; the game has sub-cell positions (the grip, walking off edges) | **Script**: a new scenario command `walk to X` (absolute), so the generated scripts cannot drift |

`LADDER_GRIP_X`, `GATE_APPROACH` and the jump constants move to `src/core/gameplay_config.h` (already in
`liblevel`) or `src/world/movement.h`, and levelcheck uses them.

## Steps

1. Scenario command `walk to X`; `levelcheck --script` emits absolute targets and uses the shared constants.
   Check: all 15 campaign replays pass.
2. `movement.h` with the shared constants and the jump numbers; remove the dead `jump` branch; fix the docs'
   jump height.
3. Walker rules 7, 8, 9 (only narrower where a level has those values; the campaign has none, checked).
4. Walker rules 1, 2 (more edges: paths and difficulty scores may change). Every campaign level still without
   warnings (AGENTS.md), the replays pass, the ranking order checked against the campaign order.
5. Unit tests for each rule on small hand-made grids.

## Checks

`./levelcheck levels/lvl*` without warnings; `levelcheck --script` replays pass for all 15 levels and the test
levels with a path; unit tests; scenarios.
