# Game code patterns: research notes

Web research, 2026-09-30. General guidance for a ~14k-line C++ grid dungeon crawler with tools. Not checked against
our code; for that see [the audit](code-structure-review-audit.draft.md). Back to [the plan](code-structure-review.draft.md).

## Game Programming Patterns (Nystrom) catalogue

| Pattern | What it is | For a game this size |
|---|---|---|
| Game Loop | Game time separate from wall time; input, update, render | **Use.** Injectable time source so tests can step it |
| Update Method | Each active object gets `update(dt)` per tick | **Use.** Loop over value arrays, no virtual `Entity::update` tree |
| Component | Entity split into parts (AI, render, physics) by composition | **Light.** Plain structs / `std::optional` members, no framework |
| Command | Actions as objects (`MoveCmd`, `AttackCmd`) | **High value.** Player, AI and scenario scripts share one action path; editor undo |
| State | Behaviour depends on the current state | **Use.** `enum class` + `switch` or `std::variant`; pushdown stack for screens |
| Observer | Synchronous notify of listeners | **Sparingly.** Sound/UI hooks, not game rules |
| Event Queue | Messages handled later at a fixed point | **Maybe.** One per-tick event list feeding sound, UI, tests |
| Service Locator | Global lookup with a null fallback | **Last resort.** Prefer params or a context struct; OK for audio/log |
| Type Object | A kind is data (a definition table) | **Strongly yes.** Monsters, items, traps, tiles |
| Flyweight | Share intrinsic data across instances | **Yes.** Tiles hold type ids, models shared |
| Spatial Partition | Index by position | **Already have it: the grid.** Add `cell -> occupant` if needed |
| Dirty Flag | Recompute derived data on change only | **Targeted.** Map view, visibility, geometry rebuilds |
| Object Pool | Reuse fixed slots | **Only particles/projectiles** |
| Double Buffer | Read old state, write new, swap | Only if monster update order bugs appear |
| Data Locality | Contiguous arrays, SoA | **Mostly overkill.** Just `std::vector<T>` by value |
| Subclass Sandbox, Prototype, Bytecode | Behaviour subclasses, cloning, scripting VM | **Overkill.** The scenario command interpreter is enough |
| Singleton | One global instance | **Avoid** |

Nystrom on roguelikes ("Is There More to Game Architecture than ECS?"): no ECS needed. Use components for
capabilities (an item with optional melee/ranged/use parts). Actors return Action objects, the loop runs them, and an
action can fall back to an alternative.

## Data-driven design

* Type Object: `struct MonsterDef { hp, speed, model, sound, flags }` in a `constexpr` array; instances store the id.
* Smell: the same `switch (type)` in several files. A new kind should be a table row, not edits in render, AI, save,
  editor and checker.
* One table per kind holds all per-type facts: name, ascii glyph, editor label, model, sound, walkability, checker
  rules. Game, editor, checker and `ascii2level` read the same table and cannot drift apart.
* Parameters in data, really different behaviour (boss AI, teleport) in code. One `switch` on a behaviour kind in one
  place is fine.
* Start with C++ tables plus `static_assert(size == Count)`. External data files only when needed.

## ECS vs composition vs inheritance

* Deep inheritance (`Entity > Actor > Monster > FlyingMonster`) is the classic anti-pattern: features drift to the
  base, which becomes a blob.
* Simple composition (concrete structs with optional parts, free functions) is the sweet spot for tens to hundreds of
  entities and a small team.
* Full ECS pays off with thousands of similar entities, heavy runtime component mixing, cache-bound loops or threads.
  It is overkill for few bespoke entities, grid games and a hand-written framework. If ever needed, use EnTT or flecs.
* One level of interface inheritance (for example `Screen`) is fine. Inheritance for code reuse is the smell.

## Global state

C++ Core Guidelines I.2/I.3: avoid mutable globals and singletons (hidden dependencies, no tests, init order).
Alternatives, best first:

1. Pass dependencies explicitly: `update(World&, const Input&, Events&)`.
2. A context struct (`GameContext { Assets&, Audio&, Rng&, Log& }`) built in `main`, swappable in tests and tools.
3. Service locator with a null default. Still hides dependencies.
4. Function-local static only for process-wide data that never changes after init.

`constexpr` constants are fine. Mutable globals are the main blocker to a library shared with tools.

## Simulation vs rendering, timestep, determinism

* The sim (`World`: grid, actors, mechanisms) has no GL types and no draw calls. The renderer reads `const World&`.
  This gives headless tests, a checker that links the sim, and replays.
* Fixed timestep (Fiedler, "Fix Your Timestep"): accumulator, fixed `dt` steps, render interpolates, cap steps per
  frame. Sample input once per frame and do not lose presses between steps.
* Determinism for stable scenario tests: fixed `dt`, one seeded RNG owned by the world (no `rand()`), no unordered
  iteration in logic, no wall clock in the sim, input recorded as commands. Check by hashing world state after N ticks
  (Factorio does this).

## State machines

* Screens: pushdown stack with `push`, `pop`, `replace`, `set`; enter/exit/obscured/revealed hooks. The top screen gets
  input; lower screens may still render (inventory over the game). Apply stack changes after the current update.
* AI: `enum class State` + `switch` per behaviour, transitions in one function. Hierarchical states only when states
  share transitions. Behaviour trees and utility AI are overkill here.
* `std::variant<Idle, Chase, Attack>` carries per-state data without a class hierarchy.

## Events

* Benefits: decoupling (the sim emits `DoorOpened{pos}`; audio, UI and tests consume it), replay, one place to log.
* Pitfalls: hard to trace, hidden listener order, re-entrancy, dangling listeners, events misused as commands, stale
  payloads.
* Rules: events are past-tense facts; drain at one fixed point per tick; payloads are values or ids, not pointers;
  core rules (lever opens gate) stay direct calls or data links.
* For this size: a per-tick `std::vector<GameEvent>` (a variant). No string-topic pub/sub bus.

## Engine as a library for game and tools

* Layers, dependencies point down only: `core` → `world`/sim (level data, rules, IO, checker) →
  `graphics`/`audio`/`platform` → `ui`/screens → executables (game, editor, levelcheck, model-viewer).
* Key rule: world/sim does not include graphics, audio or UI headers. The checker and tests then link only core and
  world.
* Tools link only the layers they need. No `#ifdef EDITOR` in shared code.
* Enforce it: one static library per layer in the makefile with explicit dependencies, plus a grep or
  include-what-you-use check that forbids `graphics/` includes from `world/`.
* Small headers, forward declarations, implementation includes in `.cpp`.

## Resources

* Move-only RAII wrappers (`Texture`, `Mesh`, `Shader`) whose destructor calls `glDelete*`. Copy deleted.
* Handles (`{index, generation}`) instead of raw pointers between systems (floooh, "Handles are the better pointers").
* Registry: load once, look up by typed id, no string lookup in hot paths. Validate level assets in the checker.
* Asset id/name tables live in a GL-free layer so tools can use them.
* For this size: one owner per resource kind, `std::vector<Texture>` + enum ids. No ref counting, no streaming.

## Testability

* Headless sim, or an offscreen context for screenshots.
* Scenario scripts: commands, N ticks, asserts on state and events, optional screenshots (like Factorio's tests).
* Replay: command stream + seed; hash state per tick to find divergence.
* Golden files: text dumps of events or state, easier to diff than screenshots.
* Unit tests for pure logic (pathing, mechanisms, level IO round-trip, checker rules). Only possible without globals
  and GL.
* Tests never read user saves or config.

## Modern C++

* RAII for every resource. Value types by default: `std::vector<Monster>`, `std::optional`, `std::variant` for closed
  sets (events, commands, AI states).
* `enum class` ids, strong typedefs (`MonsterId`, `CellPos`), `std::span` / `std::string_view` for views.
* `unique_ptr` for ownership, `shared_ptr` rarely. Shallow hierarchies, free functions over data.
* Const-correct: the renderer takes `const World&`.
* `constexpr` tables with `static_assert`; `-Wswitch-enum` so a new enum value fails loudly.
* Data-oriented basics (Acton): design around the data and its transforms. At this size for clarity, not speed.

## Anti-patterns

* God object (`Game`, `Level`, `Dungeon` owning IO, rules, rendering, audio, UI). Split into data, rules,
  presentation.
* Deep inheritance / blob base class.
* Vague `*Manager` classes. Name by job: `TextureCache`, `LevelLoader`.
* Stringly-typed ids and events. Strings only at the file-format boundary.
* Per-type `switch` duplicated across files.
* Singletons and mutable globals.
* Premature ECS, premature generic engine, plugin systems, reflection.
* Over-engineering: interfaces with one implementation, event buses for direct links, scripting VM for 5 behaviours.
* Sim tied to render and the wall clock (logic in draw calls, frame-rate dependent behaviour).
* `#ifdef EDITOR` forks of shared logic.
* Pointer soup between entities. Use ids or handles.
* Circular module dependencies.

## Priority for this game

1. Layered libraries: core → world/sim without GL → graphics/audio/ui → executables. Tools link the lower layers.
2. Definition tables for monsters, items, tiles, traps, mechanisms, shared by game, editor, checker, `ascii2level`.
3. Command path shared by player, AI and scenario scripts; deterministic seeded sim; per-tick event list.
4. Screen stack for UI; enum or variant state machines for AI.
5. RAII GL wrappers; typed ids or handles for assets.
6. Skip: ECS, bytecode, pools (except particles), generic event bus, service locator (except audio/log).

## Sources

1. [Game Programming Patterns](https://gameprogrammingpatterns.com/contents.html): [State](https://gameprogrammingpatterns.com/state.html), [Observer](https://gameprogrammingpatterns.com/observer.html), [Event Queue](https://gameprogrammingpatterns.com/event-queue.html)
2. [Nystrom: Is There More to Game Architecture than ECS?](https://www.youtube.com/watch?v=JxI3Eu5DPwE) ([notes](https://gist.github.com/remzmike/c188f8cc5e8970618a1a8d1f38780417))
3. [Fiedler: Fix Your Timestep!](https://gafferongames.com/post/fix_your_timestep/)
4. [Reliable fixed timestep and inputs](https://jakubtomsu.github.io/posts/input_in_fixed_timestep/)
5. [Mick West: Evolve Your Hierarchy](https://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
6. [ECS for Humans: When You Actually Need It](https://mojolabs.nz/entity-component-system-for-humans-when-you-actually-need-it/)
7. [floooh: Handles are the better pointers](https://floooh.github.io/2018/06/17/handles-vs-pointers.html)
8. [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
9. [ACCU: Alternatives to Singletons and Global Variables](https://accu.org/journals/overload/23/126/schmidt_2085/)
10. [Acton: Data-Oriented Design and C++](https://www.youtube.com/watch?v=rX0ItVEVjHc), [ACCU: Revisiting Data-Oriented Design](https://accu.org/journals/overload/30/167/teodorescu/)
11. [GAMES104: Layered Architecture of Game Engine](https://alalba221.github.io/blog/engine/LayeredArchitectureOfGameEngine)
12. [The Toolsmiths: The Dependency Question](https://thetoolsmiths.org/2009/09/01/the-dependency-question/)
13. [Isetta Engine: Engine Architecture](https://isetta.io/blogs/engine-architecture/)
14. [Nuclex: Game State Management](http://blog.nuclex-games.com/tutorials/cxx/game-state-management/)
15. [Factorio FFF #60](https://factorio.com/blog/post/fff-60), [FFF #62](https://www.factorio.com/blog/post/fff-62)
16. [Event pitfalls and how to avoid them](https://dev.to/peholmst/event-pitfalls-and-how-to-avoid-them-4d31)
