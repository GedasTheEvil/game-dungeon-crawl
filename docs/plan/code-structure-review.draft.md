# Code structure review

Status: all stages worked through 2026-09-30, verified in play. Done: 1, 2, 3 (verified in play), 4, 5,
8, 10, 11, 12, the random streams of 9, part of 6. Waiting for a decision: held-key movement (9,
[fixed-timestep.draft.md](fixed-timestep.draft.md)), how far `Game()` goes (6). Deferred with a reason: 7. See
[Stages](#stages).

* [code-structure-review-audit.draft.md](code-structure-review-audit.draft.md): what the code has, what it lacks, anti-patterns.
* [code-structure-review-patterns.draft.md](code-structure-review-patterns.draft.md): web research on good game code
  patterns, with sources.

## Why

The code grew feature by feature. Patterns that were fine for a small game start to hurt as more systems arrive
(teleporters, bosses, minions, new decorations). Example from the audit: the teleporter needed edits in about 7 places,
because per-tile behaviour is spread over if-chains in render, interact, checker, editor and `ascii2level`.

This is a new plan. The old `docs/plan/optimization-plan/` was implemented long ago and is not used here.

## Process

1. Audit and research (done 2026-09-30).
2. Re-check the audit against the code, decide the stages and their order (done 2026-09-30, at `bfd0ec8`: see the
   audit's marks for what was fixed, what grew and what is new).
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
  definition table). They unblock the others.
* **Order after that** (2026-09-30): 10, 4, 5, 8, 7, 9, 6, 11, 12, with 13 spread over all of them. Reasons in
  [Stages](#stages).
* **Stage 7 (game events): not now** (2026-09-30). The world calls sound, the status line, XP and the UI directly
  in 33 places. An event list pays off once the world has to run without `Game()` (headless simulation, unit tests
  of `Dungeon`); before that it trades direct calls for indirection and moves XP and level-ups to the end of the
  tick for no problem solved today. Revisit with stage 6.
* **Unit tests: doctest** (one vendored header). Set up in stage 2, step 1; every later stage adds tests for the code
  it moves before it moves it.
* **Bugs found in the audit are fixed on their own**, outside this plan: [trap-and-font-bugs.md](solved/trap-and-font-bugs.md).
* **Game and tools share one library** (or one library per layer). The editor, levelcheck, levelgen and model-viewer
  link it instead of listing objects or recompiling sources. Done in stage 1: `liblevel.a`, `librender.a`.
* Also fixed on its own: the save list read overrunning `SaveName::name` (`480b4d6`).

## Stages

| # | Stage | Status | Main audit findings | Pattern |
|---|---|---|---|---|
| 1 | **Layered build and shared libraries** | done: [solved/layered-build.md](solved/layered-build.md) | hand-listed objects, level tools recompiled, SDL via the timer | layered libraries |
| 2 | **Ids and rules out of the UI**, unit tests set up | done: [solved/rules-out-of-ui.md](solved/rules-out-of-ui.md) | ids in `inventory.h` (GL), copies in tools, rules in screens, HUD view model in `draw.cpp`, keys outside `GameplayAction` | separation of concerns |
| 3 | **Tile and monster definition tables** | done: [solved/tile-table.md](solved/tile-table.md) | 12-20 places per tile kind, two boss sources, lossy glyph round trip, raw 0/1/2 states | type object, data-driven |
| 10 | **One movement model:** the checker's `Walker` uses the game's rules and constants | done: [movement-model.md](solved/movement-model.md) | 14 divergences (jumps over spikes and from ladders, stamina, gate approach, pulled levers, ...) | shared sim |
| 4 | **Split sim from render:** nothing in `Draw` changes game state; `Update`/`updateAttack` out of `draw.cpp` | done: [sim-render-split.md](solved/sim-render-split.md) | monster spawn and the boss fight start in `Draw`, `rotA++` on prototypes, HUD damage trail, model advance in `Draw` | update method, const render |
| 5 | **Break up `Dungeon`:** grid, mechanisms, monsters and boss director, projectiles, decor, renderer; the player owns its position and physics; one view-window helper; one player width | done (partly): [dungeon-split.md](solved/dungeon-split.md) | 10 responsibilities, 1785 lines, `jump` written 12 times, 7 view-window copies | composition |
| 8 | **Screen stack** and a shared screen base (canvas, fonts, toast, frame end, clicks) | done: [screens.md](solved/screens.md) | 4 `show` bools, 5 frame-end copies, 5 square-canvas setups | pushdown automaton |
| 7 | **Game events:** a per-tick event list for sound, status text, XP and scenario asserts | not now (see Decided) | gameplay calls sound, `ShowStatus`, `AddXP` directly (also from `Monster::takeHit`) | event queue (light) |
| 9 | **Timestep, input and RNG:** fixed-dt update, held-key movement, one seeded gameplay RNG (no `rand()` / `random()`) | RNG done: [random-streams.md](solved/random-streams.md); movement waits for a decision: [fixed-timestep.draft.md](fixed-timestep.draft.md) | key-repeat movement, scripted walk speed differs, unseeded `random()` in scenarios | fixed timestep |
| 6 | **Replace `Game()` step by step** | part: the renderer ([cleanups.md](solved/cleanups.md)); how far to go stays open | 438 calls, the `game_state.h` hub | explicit dependencies |
| 11 | **RAII for GL resources** | done: [gl-resources.md](solved/gl-resources.md) | copyable `Texture` / `Font`, nothing freed | RAII |
| 12 | **Smaller cleanups:** long functions, duplicated helpers, magic numbers, save format version tags, the two scene projections | done: [cleanups.md](solved/cleanups.md) | see the audit | |
| 13 | **Unit tests for pure logic** | 44 test cases so far: [cleanups.md](solved/cleanups.md) | only GL scenario tests | testability |

Why this order:

* **2 and 3 first:** they are the most shotgun surgery per feature (every new item, tile or monster) and they give
  `liblevel` the ids and tables the later stages build on. Stage 2 also brings the unit test runner.
* **10 next:** the checker judges every campaign level; its divergences hide real problems (a chain of jumps the
  player has no stamina for, a pulled lever that opens nothing). It needs the tables from stage 3 and is small.
* **4 before 5:** once `Draw` is const, splitting `Dungeon` does not move hidden state changes around.
  Do 4 and 5 before the next bosses ([boss-rooms.draft.md](boss-rooms.draft.md) steps 3-4), because the boss director
  and minion spawning live in exactly that code.
* **8 and 7:** screens and events are self-contained and get easier once `Game()` has fewer writers (after 2, 5).
* **9 late:** the riskiest for game feel and for every scenario's timing; it needs 4 (sim apart from render).
* **6 last among the big ones:** most of it falls out of 2, 4, 5, 7 and 8; what is left decides how far to go.
* **11, 12:** any time, in small pieces, when an area is touched anyway.

## Open questions

* How far should `Game()` go: remove it fully, or keep a few services (log, audio) behind it? Decide later (stage 6).
* Fixed timestep: is it worth the risk to feel and to the scenario test timings? Decide in stage 9.

Answered: unit test framework: doctest (2026-09-30). Stage 2: `enum class ItemKind`, and `Inventory` split into
`ItemBag` (model) and the screen (2026-09-30). Stage 3: a `switch` per tile type, the docs checked by a unit test
(2026-09-30).
