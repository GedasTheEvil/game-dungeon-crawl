# Stage 2: Architecture — Decompose God Object & Remove Global State

**Goal:** Break the `Cashe` god object into cohesive, independently testable subsystems
and eliminate hidden global state. This is the highest-impact structural change; it
unblocks all later refactoring.

---

## Anti-patterns addressed

| # | Anti-pattern | Severity |
|---|---|---|
| 1 | `Cashe` god object — 88 public members, 375+ `GAME_STATE.` usages | Critical |
| 2 | 5+ bare `extern` global variables in input/graphics | High |
| 3 | Service Locator returning mutable reference to everything | High |
| 4 | `extern Cashe* c;` in `cashe.h` — duplicate access path | High |
| 5 | Circular init dependency (`Dungeon` → `GAME_STATE` in constructor) | Medium |

---

## 2.1 — Identify cohesive groups inside `Cashe`

Before splitting, map the 88 members into logical subsystems. Proposed groupings:

| Subsystem | Members | New class |
|---|---|---|
| Textures | `Mt[4]`, `anubis_t`, `scarab_t`, `plant_t`, `worm_t`, `floor_*`, `wall_*`, … (35+ `Textura`) | `TextureRegistry` |
| Entities | `std::unique_ptr<monster> anubis, scarab, plant, worm`, `Player player` | `EntityRegistry` |
| UI screens | `inventory inv`, `stats stats_screen`, `MainMenu menu`, `Riddle riddle` | Keep in `Cashe` or a `UIContext` |
| Physics/movement | `jumping`, `falling`, `jump_dir_x`, `jump_speed`, `jump_vel`, `jump_start_y`, `jump_timer` | `JumpState` (value struct) |
| World | `Dungeon dungeon` | standalone — already a class |
| Render flags | `bool Cartoon`, `int mdl_select` | `RenderSettings` |

---

## 2.2 — Extract `TextureRegistry`

**File:** new `src/graphics/texture_registry.h/cpp`

All `Textura` members of `Cashe` move here. Classes that currently do
`GAME_STATE.anubis_t` receive a `const TextureRegistry&` reference instead.

```cpp
struct TextureRegistry {
    Textura floor_t, wall_t, ceiling_t;
    Textura anubis_t, scarab_t, plant_t, worm_t;
    Textura Mt[4];
    // ...
    void loadAll();   // centralise all texture loading here
};
```

**Migration path:**
1. Create `TextureRegistry`.
2. In `Cashe`, replace the raw members with `TextureRegistry textures;`.
3. Add `GAME_STATE.textures.` prefix to all usages (mechanical search-replace).
4. Pass `const TextureRegistry&` by reference where systems only read textures.

---

## 2.3 — Extract `JumpState`

**File:** `src/entities/player.h` (or `src/core/physics_state.h`)

The six jump-related fields and the jump timer belong on the player, not on the global
game state.

```cpp
struct JumpState {
    bool jumping = false;
    bool falling = false;
    float dir_x   = 0.f;
    float speed    = 0.f;
    float velocity = 0.f;
    float start_y  = 0.f;
    std::unique_ptr<timer> jump_timer;
};
```

Move to `Player` class. Update `dungeon_base.cpp` / `input.cpp` to access via
`player.jump` instead of `GAME_STATE.jumping`.

---

## 2.4 — Remove bare `extern` globals

**Affected files:**

| Variable | Declared in | Used by | Fix |
|---|---|---|---|
| `rotW, rotM, rotN` | `src/input/input.cpp` | `draw.cpp`, `dungeon_render.cpp` | Move to a `Camera` struct passed to renderer |
| `attacking` | `src/input/input.cpp` | `monster_ai.cpp` | Move to `Player` |
| `resX, resY` | `src/graphics/draw.cpp` | Several render files | Move to `RenderSettings` or pass as args |
| `plasma` | `src/world/Dungeon.cpp` | `dungeon_render.cpp` | Move to `Dungeon` as private member |
| `qRot` | `src/world/Dungeon.cpp` | `dungeon_render.cpp` | Move to `Dungeon` as private member |

**Fix pattern:** Make each a member of the class that owns the concept. Pass it through
function parameters or as a const reference where other systems need to read it.

### Acceptance criteria
- `grep -rn "^extern " src/` returns 0 results (forward declarations in headers for C linkage are OK but none exist here).

---

## 2.5 — Simplify `ServiceLocator`

**File:** `src/core/service_locator.h/cpp`

Current issues:
- Returns a `Cashe&` — exposes the entire god object.
- Has a `nullState` fallback that silently creates a dummy `Cashe` and hides missing initialization.

After Stage 2.2–2.4 shrink `Cashe`, evaluate whether ServiceLocator is still needed or
whether the remaining state can be passed explicitly. If kept:

```cpp
// Remove nullState entirely — fail loudly if not initialized
static Cashe& getGameState() {
    assert(gameState && "ServiceLocator not initialized");
    return *gameState;
}
```

Remove `extern Cashe* c;` from `cashe.h` — it duplicates the ServiceLocator path and
creates a second mutable alias.

---

## 2.6 — Fix circular initialization dependency

**File:** `src/state/cashe.cpp`, `src/world/Dungeon.cpp`

Currently `Dungeon`'s constructor sets `GAME_STATE.falling = false`, which requires the
ServiceLocator to be initialized before `Dungeon` can be constructed. The comment at
line 53 acknowledges this.

**Fix:**
1. Remove all `GAME_STATE.*` writes from `Dungeon`'s constructor entirely — `Dungeon`
   must not write game state during construction.
2. Initialize `falling` / other flags directly in `Cashe`'s initializer list or
   `JumpState`'s default member initializers (see 2.3).
3. Keep `Dungeon::InitializeAfterServiceLocator()` only for operations that genuinely
   need post-construction wiring, and document why.

---

## Effort estimate

| Task | Estimated time |
|---|---|
| 2.1 Member mapping / design | 1 h |
| 2.2 TextureRegistry extraction | 3–4 h |
| 2.3 JumpState extraction | 1 h |
| 2.4 Remove extern globals (5 vars) | 2–3 h |
| 2.5 Simplify ServiceLocator | 1 h |
| 2.6 Fix circular init | 1 h |
| **Total** | **9–11 h** |

---

## Done criteria for Stage 2

- [ ] `Cashe` has fewer than 20 direct members (remainder delegated to sub-structs).
- [ ] `grep -rn "^extern " src/` returns 0 results.
- [ ] `extern Cashe* c;` removed from `cashe.h`.
- [ ] `ServiceLocator::nullState` removed; missing init is an assertion failure.
- [ ] Dungeon constructor does not write to `GAME_STATE`.
- [ ] Game compiles and runs without behavioural regression.
