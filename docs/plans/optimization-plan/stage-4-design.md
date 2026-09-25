# Stage 4: Design — Separation of Concerns & Structural Improvements

**Goal:** Reduce function complexity, separate rendering from logic, consolidate
duplicated patterns, and eliminate platform-specific boilerplate. By this stage the
code is safe (Stage 1), structurally sound (Stage 2), and readable (Stage 3); Stage 4
makes it maintainable and extensible.

---

## Anti-patterns addressed

| # | Anti-pattern | Severity |
|---|---|---|
| 1 | UI classes contain game logic | Medium |
| 2 | `Dungeon.cpp` mixes state, collision, rendering, spawning | Medium |
| 3 | Monolithic functions — `Draw()` 147 lines, `DrawSegment()` 400+ lines | Medium |
| 4 | Monster state represented as raw pointer comparison | Medium |
| 5 | Model loading boilerplate repeated per monster | Medium |
| 6 | Coordinate system ambiguity — three parallel `x/y`, `X/Y`, `DX/DY` systems | Medium |
| 7 | Platform `#ifdef WIN32` in 5+ unrelated files | Low |

---

## 4.1 — Separate update logic from rendering in `draw.cpp`

**File:** `src/graphics/draw.cpp`

The main render callback currently handles:
1. Screen-state branching
2. Calling `dungeon.Update()` (game logic)
3. Camera transform
4. HUD drawing

This means logic and rendering are interleaved in the GLUT display callback.

**Proposed split:**

```
glutDisplayFunc  →  Draw()
    |
    ├── Update(deltaTime)   ← pure logic, no GL calls
    │       ├── dungeon.Update()
    │       ├── player input application
    │       └── screen state transitions
    │
    └── Render()            ← pure GL calls, reads state only
            ├── dungeon.Render()
            ├── hud.Render()
            └── ui screen Render()
```

Wire `Update()` to a `glutTimerFunc` at fixed 16 ms (≈60 Hz) so game speed is
framerate-independent. `Render()` stays in the display callback.

### Acceptance criteria
- `Draw()` / `Render()` contains no calls to game-logic mutators.
- `Update()` contains no `gl*` calls.

---

## 4.2 — Split `Dungeon.cpp` (886 lines) into focused units

**File:** `src/world/Dungeon.cpp`

The file already has companion files (`dungeon_render.cpp`, `dungeon_monsters.cpp`,
`dungeon_base.cpp`, `dungeon_io.cpp`). Complete the split by moving the remaining
mixed responsibilities:

| Responsibility | Move to |
|---|---|
| Tile collision logic | `dungeon_base.cpp` |
| Item pickup / interaction | `dungeon_items.cpp` (new) |
| Remaining render calls in `Dungeon.cpp` | `dungeon_render.cpp` |
| State accessors (`Map()`, `GetTile()`) | `dungeon.h` inline or `dungeon_base.cpp` |

After the split, `Dungeon.cpp` should only contain the class constructor, destructor,
and the `Update()` coordinator that delegates to the other files. Target: < 150 lines.

---

## 4.3 — Break down `DrawSegment()` (~400 lines)

**File:** `src/world/dungeon_render.cpp`

A single `switch` statement with ~20 tile-type cases, each containing nested GL
transforms, is fragile and nearly impossible to test.

**Approach:** Replace the switch with a dispatch table.

```cpp
using TileRenderer = std::function<void(const TileRenderContext&)>;
static const std::unordered_map<TileType, TileRenderer> TILE_RENDERERS = {
    { TileType::Floor,   renderFloor   },
    { TileType::Wall,    renderWall    },
    { TileType::Ladder,  renderLadder  },
    // ...
};
```

Each `renderXxx` function is a free function of ~15–30 lines. This makes individual
tile types independently testable and swappable.

### Acceptance criteria
- `DrawSegment()` is ≤ 40 lines (just the dispatch + matrix setup).
- Each tile-type function is ≤ 50 lines.

---

## 4.4 — Replace pointer-comparison model state with `enum class`

**File:** `src/entities/monster.cpp`

```cpp
// Before — fragile pointer comparison
if (mdl == walk.get()) return 1;
if (mdl == attack.get()) return 2;
return 0;  // die

// After — explicit state
enum class ModelState { Walk, Attack, Die };
ModelState currentState = ModelState::Walk;
```

Store `currentState` on the `monster` class. Replace all `mdl ==` comparisons with
`currentState ==`. This eliminates a class of bugs where `mdl` could be a dangling
pointer after a move/swap.

---

## 4.5 — Extract monster model loading into a factory

**File:** `src/entities/monster.cpp`

Every monster type repeats the same five-step model loading sequence:

```cpp
walk = std::make_unique<AnimatedCartoonModel>();
walk->Load(path);
walk->BindTexture(tex.ID());
walk->Centrify();
walk->setSpeed(speed);
```

Extract a helper:

```cpp
static std::unique_ptr<AnimatedCartoonModel>
loadModel(const std::string& path, GLuint texId, int speed) {
    auto m = std::make_unique<AnimatedCartoonModel>();
    m->Load(path);
    m->BindTexture(texId);
    m->Centrify();
    m->setSpeed(speed);
    return m;
}
```

Call sites become one line each. The `Load()` method on `monster` reduces from ~60
lines to ~15.

---

## 4.6 — Clarify the coordinate system

**Affected files:** `src/entities/monster.h`, `src/world/dungeon.h`,
`src/world/dungeon_render.cpp`

Three overlapping coordinate systems cause constant confusion:

| Variable pattern | Meaning | Rename to |
|---|---|---|
| `float x, y` (on monster) | Map position (tile units, fractional) | `mapX`, `mapY` |
| `float X, Y` (on monster) | Screen/render offset | `renderOffsetX`, `renderOffsetY` |
| `float* DX, *DY` (on monster) | Pointer to dungeon camera position | `dungeonCamX`, `dungeonCamY` |

Document the coordinate space in a single comment block at the top of `dungeon.h`:

```cpp
// Coordinate spaces used throughout the world system:
//   Map space:    float [0, MAP_W] x [0, MAP_H], tile units, used for collision
//   World space:  map * TILE_SIZE, OpenGL units, used for rendering
//   Screen space: projection of world space, origin top-left
```

### Acceptance criteria
- No variable named `x`/`y`/`X`/`Y` at class scope (loop indices exempt).
- Coordinate-space comment block present in `dungeon.h`.

---

## 4.7 — Consolidate platform `#ifdef WIN32` includes

**Affected files:** `src/core/game.cpp`, `src/graphics/draw.cpp`,
`src/ui/menu.cpp`, `src/ui/inventory.cpp`, `src/ui/stats.cpp`

Each file independently guards the same GLUT/FreeGLUT include:

```cpp
#ifndef WIN32
#include <GL/glut.h>
#endif
#ifdef WIN32
#include <GL/freeglut.h>
#endif
```

Create `src/graphics/gl_includes.h`:

```cpp
#pragma once
#ifdef WIN32
#  include <GL/freeglut.h>
#else
#  include <GL/glut.h>
#endif
```

Replace the 5-line block in every file with `#include "graphics/gl_includes.h"`.

### Acceptance criteria
- `grep -rn "freeglut\|#ifndef WIN32" src/` returns only `gl_includes.h`.

---

## 4.8 — Consolidate `UsePotion()` switch statement

**File:** `src/ui/inventory.cpp`, lines 62–97

The five potion cases each follow the same pattern: check quantity, apply stat delta,
decrement. Extract to a data-driven table:

```cpp
struct PotionEffect {
    int& stat;      // reference to the affected stat
    int& maxStat;
    int  delta;
    int  maxDelta;
};
```

Build a `std::array<PotionEffect, 5>` and loop over it. Eliminates ~35 lines of
near-identical code.

---

## Effort estimate

| Task | Estimated time |
|---|---|
| 4.1 Update/render split | 3–4 h |
| 4.2 Dungeon.cpp split | 2–3 h |
| 4.3 DrawSegment dispatch table | 4–6 h |
| 4.4 Model state enum | 1 h |
| 4.5 Monster model factory | 1 h |
| 4.6 Coordinate naming | 2–3 h |
| 4.7 GL includes consolidation | 30 min |
| 4.8 Potion switch → table | 1 h |
| **Total** | **14–19 h** |

---

## Done criteria for Stage 4

- [ ] `Dungeon.cpp` is < 150 lines.
- [ ] `DrawSegment()` dispatches via table; no function exceeds 50 lines.
- [ ] `Draw()` / `Render()` contains no game-logic calls.
- [ ] `ModelState` enum used everywhere; no `mdl == somePtr.get()` comparisons.
- [ ] `grep -rn "freeglut\|#ifndef WIN32" src/` → only `gl_includes.h`.
- [ ] No class-scope `x`, `y`, `X`, `Y` variables — all renamed for clarity.
- [ ] Game compiles and runs without behavioural regression.

---

## Overall plan summary

| Stage | Theme | Est. effort |
|---|---|---|
| **Stage 1** | Memory Safety — RAII, smart pointers, buffer safety | 5–9 h |
| **Stage 2** | Architecture — decompose god object, remove globals | 9–11 h |
| **Stage 3** | Code Quality — naming, constants, logging, error handling | 10–13 h |
| **Stage 4** | Design — SoC, function size, patterns, platform cleanup | 14–19 h |
| **Total** | | **38–52 h** |

Stages are ordered by risk: Stage 1 fixes bugs, Stages 2–4 improve structure.
Each stage leaves the game in a buildable and runnable state.
