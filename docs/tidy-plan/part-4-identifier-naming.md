# Part 4 — Identifier Naming

**9 warnings** across 4 files.
Check: `readability-identifier-naming`.

---

## What the warning means

The `.clang-tidy` config requires:
- Local `const` variables → `UPPER_CASE`
- Functions → `camelBack`

All 9 violations are local `const` variables and one function that use `camelBack` instead of `UPPER_CASE`.

---

## All violations

### `src/graphics/hud.cpp` — 5 constants (lines 37–41)

| Current name | Required name |
|-------------|--------------|
| `barLeft` | `BAR_LEFT` |
| `barWidth` | `BAR_WIDTH` |
| `barHeight` | `BAR_HEIGHT` |
| `hpBottom` | `HP_BOTTOM` |
| `staminaBottom` | `STAMINA_BOTTOM` |

All are `const float` layout parameters in the HUD draw function. Rename declaration and all uses within the same function.

### `src/ui/menu.cpp` — 1 constant (line 66)

| Current name | Required name |
|-------------|--------------|
| `label` | `LABEL` |

`const std::string label = formatSaveLabel();` — rename to `LABEL`.

### `src/world/dungeon_render.cpp` — 2 constants (lines 170, 230)

| Current name | Required name |
|-------------|--------------|
| `tile` (line 170) | `TILE` |
| `tile` (line 230) | `TILE` |

Two separate scopes, both `const Tint tile = MapAt(i, j)`. Rename each independently within its block.

### `src/core/game.cpp` — 1 function (line 17)

| Current name | Required name |
|-------------|--------------|
| `UpdateTimerCallback` | `updateTimerCallback` |

Function case must be `camelBack`. This is a callback passed to a timer — also update the call site(s) and any forward declarations.

---

## Notes

- `clang-tidy --fix` can apply renaming for local variables automatically. For the function rename in `game.cpp`, check all call sites first (grep for `UpdateTimerCallback`).
- After renaming `TILE` in `dungeon_render.cpp`, verify the two scopes don't accidentally collide — they shouldn't since they're in separate blocks, but confirm.
