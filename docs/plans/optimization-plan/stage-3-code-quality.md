# Stage 3: Code Quality — Naming, Constants & Error Handling

**Goal:** Eliminate inconsistencies that make the code hard to read and maintain —
magic numbers, mixed naming conventions, stale printf calls, and missing error
propagation. These changes are mostly mechanical but high-leverage for long-term
maintainability.

---

## Anti-patterns addressed

| # | Anti-pattern | Severity |
|---|---|---|
| 1 | Magic numbers in rendering and physics | Medium |
| 2 | Inconsistent naming conventions (case, abbreviations) | Medium |
| 3 | `printf` mixed with structured `Logger` | Medium |
| 4 | Missing or silent error handling on file I/O | Medium |
| 5 | Commented-out dead code | Low–Medium |
| 6 | C-style casts (`(float)`, `(int)`) | Low |

---

## 3.1 — Extract magic rendering constants

**Affected files:** `src/world/Dungeon.cpp`, `src/entities/monster.cpp`,
`src/graphics/particles.cpp`

Every unexplained numeric literal in a rendering or physics calculation should become
a named `constexpr` constant in the appropriate header. Follow the pattern already
established in `src/input/gameplay_config.h`.

Create `src/graphics/render_config.h`:

```cpp
namespace RenderConfig {
    // Tile/world scale
    constexpr float TILE_SIZE       = 40.f;   // world units per dungeon tile
    constexpr float TILE_HALF       = TILE_SIZE / 2.f;
    constexpr float TILE_RENDER_Y   = -120.f; // vertical offset for tile origin
    constexpr float HUD_OFFSET_X    = -320.f; // HUD left edge in screen coords

    // Particle physics
    constexpr float PARTICLE_DRIFT  = 0.1f;   // horizontal drift per frame

    // Monster positioning
    constexpr float MONSTER_Z_DEPTH = -30.f;
    constexpr float MONSTER_X_SCALE = 40.f;
    constexpr float MONSTER_X_BIAS  = -20.f;
}
```

Then replace each literal with its constant:

```cpp
// Before (Dungeon.cpp:590)
glTranslatef(-40 * (x - (int)x), -40 * (y - (int)y), 0);

// After
glTranslatef(-TILE_SIZE * (x - (int)x), -TILE_SIZE * (y - (int)y), 0);
```

### Acceptance criteria
- All numeric literals in rendering/physics functions have a named constant or a
  one-line comment explaining the value.
- `src/graphics/render_config.h` exists and is the single source of truth for tile scale.

---

## 3.2 — Standardize naming conventions

**Scope:** All `src/` headers and implementation files.

### Agreed conventions (choose one set and apply consistently)

| Category | Convention | Examples |
|---|---|---|
| Classes | `PascalCase` | `AnimatedCartoonModel`, `TextureRegistry`, `JumpState` |
| Methods / free functions | `camelCase` | `getHit()`, `changeMdl()`, `loadTextures()` |
| Member variables | `camelCase` with no prefix | `frameCount`, `healthPoints`, `walkModel` |
| Local variables | `camelCase` | `tileX`, `frameIndex` |
| Constants / `constexpr` | `UPPER_SNAKE_CASE` | `TILE_SIZE`, `MAX_MONSTERS` |
| Enums / enum class values | `PascalCase` | `ModelState::Walk`, `ScreenState::Menu` |

### Priority renames

The following are the most-read identifiers that currently violate the convention:

| Current | Proposed | Files affected |
|---|---|---|
| `Cashe` | `GameState` | `cashe.h/cpp`, all `GAME_STATE.` usages |
| `mdl` | `model` | `item.h`, `trap.h`, `monster.h` |
| `ani_e`, `ani_f` | `frameTimer`, `decayTimer` | `particles.h/cpp` |
| `t_cred` | `creditsTimer` | `menu.h/cpp` |
| `Mt[4]` | `monsterTextures[4]` | `cashe.h` |
| `TNull` | `nullTexture` | wherever used |
| `HP`, `MaxHP` | `health`, `maxHealth` | `monster.h/cpp` |
| `DX`, `DY` | `dungeonX`, `dungeonY` | `monster.h/cpp` |
| `xx`, `yy`, `zz` | Use meaningful names at call site | `dungeon_render.cpp` |
| `AtDir()` | `attackDirection()` | `monster.h` |

> **Note:** Rename `Cashe` → `GameState` with a find-and-replace across all files.
> Update the class name, filename (`cashe.h` → `game_state.h`), and the
> `GAME_STATE` macro (or replace the macro with an inline function call).

### Acceptance criteria
- `grep -rn "\bCashe\b" src/` returns 0 results.
- No single-letter member variables except loop indices.
- No `_e` / `_f` / `_t` suffixed members.

---

## 3.3 — Replace all `printf` with Logger

**Affected files:** Every file with raw `printf` — ~56 call sites identified.

The `Logger` system (`src/core/logger.h`) is already in place. The mapping is:

```cpp
// Before
printf("Loading model: %s\n", tmp1);
printf("Error [%d]: Object Already loaded\n", stat);

// After
LOG_INFOF("graphics", "Loading model: %s", tmp1);
LOG_ERRORF("graphics", "Object already loaded: error %d", stat);
```

Use these categories (consistent with existing usage):
- `"game"` — startup, shutdown, high-level flow
- `"graphics"` — textures, models, rendering
- `"world"` — dungeon, map, tiles
- `"entities"` — monsters, player, items, traps
- `"ui"` — menus, inventory, stats
- `"input"` — keyboard, mouse events
- `"audio"` — sound/music

### Acceptance criteria
- `grep -rn "printf(" src/` returns 0 results.
- Debug-only output uses `LOG_DEBUG` so it can be silenced in release builds.

---

## 3.4 — Add proper error handling to file I/O paths

**Affected files:** `src/world/dungeon_io.cpp`, `src/state/cashe.cpp`,
`src/graphics/font.cpp`, `src/graphics/textures.cpp`

Current failures are either silent or log-only. Define a clear policy:

- **Fatal failures** (texture/model missing, save file corrupt): throw
  `std::runtime_error` with a descriptive message. Let `main()` catch and display a
  dialog / clean shutdown. (`LoadTGA` already does this — make it consistent.)
- **Recoverable failures** (save slot not found, optional audio file): return
  `std::optional<T>` or `bool` and log at `LOG_WARN` level.

```cpp
// Font loading (currently falls through silently)
if (!t.LoadTGA("Fonts/font.tga") && !t.LoadBMP("Fonts/font.bmp")) {
    throw std::runtime_error("Font texture not found: Fonts/font.{tga,bmp}");
}
```

### Acceptance criteria
- No load function returns silently on failure without either throwing or returning a
  distinguishable error value.
- `main()` (or `game.cpp`) has a `try/catch` around the game loop that logs and exits
  cleanly on `std::exception`.

---

## 3.5 — Remove dead and commented-out code

Review and delete (not just comment) the following:

| Location | Dead code |
|---|---|
| `src/entities/monster.h:56` | Commented `Attack(monster*)` — delete or implement |
| `src/graphics/shader.h:35` | Commented destructor |
| `src/graphics/font.h:9` | Commented `int loaded` member |
| `src/entities/monster_ai.cpp:28` | Commented `walk_timer->TimePassed()` check |
| `src/ui/riddle.cpp:110` | Commented print |
| `src/graphics/particles.cpp:117` | Commented OpenGL call |
| `src/input/input.cpp:123` | Commented debug printf |

If code is commented out because it is "not yet implemented", open a GitHub issue or
add a `// TODO(#issue):` marker instead of leaving a block comment.

### Acceptance criteria
- `grep -rn "^[[:space:]]*//" src/` shows no block-commented function bodies.

---

## 3.6 — Replace C-style casts with `static_cast`

**Affected files:** `src/world/Dungeon.cpp:66`, `src/graphics/textures.cpp:60`,
`src/graphics/ani.cpp:114`

```cpp
// Before
(float)i
(int)frame

// After
static_cast<float>(i)
static_cast<int>(frame)
```

The `dungeon_base.cpp:34` use of `static_cast` already shows the right pattern.
Enable `-Wold-style-cast` in the Makefile to catch future regressions.

### Acceptance criteria
- Makefile includes `-Wold-style-cast` in `CXXFLAGS`.
- Build produces zero old-style-cast warnings.

---

## Effort estimate

| Task | Estimated time |
|---|---|
| 3.1 Magic constants | 2–3 h |
| 3.2 Naming (Cashe rename + priority list) | 3–4 h |
| 3.3 printf → Logger (56 sites) | 2–3 h |
| 3.4 Error handling | 2 h |
| 3.5 Dead code removal | 1 h |
| 3.6 C-style casts + warning flag | 30 min |
| **Total** | **10–13 h** |

---

## Done criteria for Stage 3

- [ ] `grep -rn "printf(" src/` → 0 results.
- [ ] `grep -rn "\bCashe\b" src/` → 0 results.
- [ ] Build with `-Wold-style-cast` produces 0 warnings.
- [ ] All load functions either throw or return a typed error on failure.
- [ ] No block-commented function bodies remain.
- [ ] `src/graphics/render_config.h` created; no unexplained numeric literals in render functions.
