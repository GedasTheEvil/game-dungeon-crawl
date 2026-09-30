# Code structure review

Status: audit and research done (2026-09-30). Not planned; other plans come first. Nothing is implemented from here.

* [code-structure-review-audit.draft.md](code-structure-review-audit.draft.md): what the code has, what it lacks, anti-patterns.
* [code-structure-review-patterns.draft.md](code-structure-review-patterns.draft.md): web research on good game code
  patterns, with sources.

## Why

The code grew feature by feature. Patterns that were fine for a small game start to hurt as more systems arrive
(teleporters, bosses, minions, new decorations). Example from the audit: the teleporter needed edits in about 7 places,
because per-tile behaviour is spread over if-chains in render, interact, checker, editor and `ascii2level`.

This is a new plan. The old `docs/plan/optimization-plan/` was implemented long ago and is not used here.

## Process

1. **Now:** audit and research (done). This file lists candidate stages.
2. **When there is time:** re-check the audit against the code (other plans will have changed it), then decide which
   stages to do and in what order.
3. **Per stage:** write a separate sub-plan `<stage-slug>.draft.md` in `docs/plan/` for one structure at a time.
   Implement it, rename it to `<stage-slug>.md`, and tick it off here.

Rules for every stage:

* Small steps. Each step keeps `make`, `make tidy`, all scenario tests and `./levelcheck levels/lvl*` green.
* Every change has a written reason: what it makes easier or what bug class it removes. No reason, no change.
* Add scenario tests for an area before refactoring it, if it has none.
* Prefer to refactor an area just before a feature needs it (for example `Dungeon` and monsters before
  [boss-rooms.draft.md](boss-rooms.draft.md)).
* Skip what does not pay off at this size: ECS, a generic event bus, scripting VMs, plugin systems. See
  [the research](code-structure-review-patterns.draft.md#priority-for-this-game).

## Decided

* **First stages: 1, 2, 3** (layered build and shared library, then ids and rules out of the UI, then the tile
  definition table). They unblock the others. Re-check the audit for these areas before writing their sub-plans.
* **Bugs found in the audit are fixed on their own**, outside this plan: [trap-and-font-bugs.draft.md](trap-and-font-bugs.draft.md).
* **Game and tools share one library** (or one library per layer). The editor, levelcheck, levelgen and model-viewer
  link it instead of listing objects or recompiling sources.

## Candidate stages

Stages 1-3 go first (see Decided). The order of the rest is not final.

| # | Stage | Main audit findings | Pattern |
|---|---|---|---|
| 1 | **Layered build and shared library.** Static libraries such as `core`, `world` (no GL), `graphics`, `ui`; tools link only what they need; `make tidy` covers all tools; a check that `world/` does not include `graphics/`, `ui/` or `state/` | object lists by hand, levelcheck recompiles, include cycle, SDL dragged in by timer | layered engine library |
| 2 | **Move ids and rules out of UI.** Item/potion ids as `enum class` in a GL-free header; weapon math, `CanUse` and potion effects move to an item-rules module; the save I/O out of `MainMenu`; the XP reward out of `Riddle` | `loot.cpp` → `ui/inventory.h`, ids copied in tools | separation of concerns |
| 3 | **Tile definition table.** One table per tile type (name, glyph, editor label, solid, interact kind, draw function, checker rules), used by the game, editor, checker and `ascii2level`; typed `attr`/`value` accessors | teleporter touched ~7 places, `TILE_COUNT = 14`, `GateType` vs `Gate` | type object, data-driven |
| 4 | **Split sim from render.** Nothing in `Draw` changes game state (monster spawn, trap damage, `rotA++`, animation advance); `Update()`/`updateAttack` move out of `graphics/draw.cpp` | trap damage only when drawn, shared trap timer | update method, const render |
| 5 | **Break up `Dungeon`.** Separate the grid, mechanisms, monsters, projectiles, decor and dungeon renderer; the player owns its position and physics; a shared view-window helper | 9 responsibilities, `player->jump` written 20× | composition |
| 6 | **Replace `Game()` step by step.** Pass dependencies or a small context struct; graphics (`lighting`, `ink`) gets settings as parameters; `GameState` without GL drawing | ~400 `Game()` calls, state ↔ ui cycle | explicit dependencies |
| 7 | **Game events.** A per-tick event list (monster died, gate opened, item picked); sound, status text, XP and scenario asserts read it | gameplay calls sound, UI and `AddXP` directly | event queue (light) |
| 8 | **Screen stack.** One screen FSM or stack instead of 4 `show` bools and menu sub-bools; a shared screen base for canvas, fonts, toast, frame end, click handling | copies in 4 screens | pushdown automaton |
| 9 | **Timestep and determinism.** Fixed-dt update with held-key movement (not key repeat); one seeded gameplay RNG in the world; goal: a headless sim for tests | movement tied to key repeat, global `rand()` | fixed timestep |
| 10 | **One movement model.** The `Walker` in `level_check` uses the same movement rules and constants as the game | `GRIP`/`GATE_STOP` copies | shared sim |
| 11 | **RAII for GL resources.** Move-only `Texture`, `Font`, display list, shader, FBO wrappers | leaks, copyable handles | RAII |
| 12 | **Smaller cleanups.** Long functions (`checkLevel`, `Assets::Load`, scenario `parseLine`/`runInstant`, `Dungeon::Draw`); duplicated seed hash, portal quad, blood logic, shader compile; magic numbers; save format version tags | see audit | |
| 13 | **Unit tests for pure logic** (riddle parsing, item rules, save round-trip, checker rules), once stages 1-2 make them possible | only GL scenario tests exist | testability |

## Open questions

* Unit test framework, or a small hand-made runner in the makefile?
* How far should `Game()` go: remove it fully, or keep a few services (log, audio) behind it?
* Fixed timestep: is it worth the risk to feel and to the scenario test timings?
