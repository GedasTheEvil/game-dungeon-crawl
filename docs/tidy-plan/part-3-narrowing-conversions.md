# Part 3 — Narrowing Conversions

**60 warnings** across 10 files.
Check: `bugprone-narrowing-conversions`.

---

## What the warning means

A value of a wider type is silently narrowed to a smaller type without an explicit cast. The common patterns here are:

| Conversion | Risk |
|-----------|------|
| `int` → `float` | Loss of precision for large integers |
| `double` → `float` (literal `0.1`) | Double literal widened then narrowed |
| `float` → `GLint` (`int`) | Fractional part silently truncated |
| `int` → `GLfloat` | Same as int→float |

The fix in all cases is to add an explicit `static_cast<target_type>(expr)`, making the narrowing intentional and visible.

---

## Affected files

| File | Warnings | Primary conversions |
|------|---------|---------------------|
| `src/entities/monster.cpp` | 15 | `int` → `float` |
| `src/graphics/font.cpp` | 10 | `float` → `GLint`, `double` → `GLfloat` |
| `src/ui/inventory.cpp` | 8 | `int`/`double` → `GLfloat`, `int` → `GLfloat` |
| `src/graphics/particles.cpp` | 8 | `int` → `float` (rand results) |
| `src/core/timer.cpp` | 7 | `int` → `float` |
| `src/graphics/ani.cpp` | 6 | `int` → `float` |
| `src/input/input.cpp` | 2 | `int` → `float` |
| `src/graphics/draw.cpp` | 2 | `int` → `float` |
| `src/graphics/shader.cpp` | 1 | `int` → `GLfloat` |
| `src/entities/monster_ai.cpp` | 1 | `int` → `float` |

---

## Fix pattern

```cpp
// int → float
// Before
float x = someInt;
// After
float x = static_cast<float>(someInt);

// double literal → float (e.g. cx + 0.1 passed to glTexCoord2f)
// Before
glTexCoord2f(cx + 0.1, 1.0f - cy);
// After
glTexCoord2f(cx + 0.1f, 1.0f - cy);   // append f suffix to literal

// float → GLint (truncation inside OpenGL call)
// Before
glVertex2i(size, 0);   // size is float
// After
glVertex2i(static_cast<GLint>(size), 0);
```

---

## Notes

- **`particles.cpp`:** `rand() % N` returns `int`; cast the whole expression to `float` before arithmetic.
- **`font.cpp` double literals:** the simplest fix is to append an `f` suffix to the double literals (`0.1` → `0.1f`).
- **`inventory.cpp` `glTranslatef`:** arguments `9 + 13 * i` and `25 - 20 * j` are `int` — wrap in `static_cast<GLfloat>(...)`.
- **`monster.cpp`:** likely has a struct or loop counter (`int`) fed into a `float` array or position field — check the context before casting, to rule out a design issue (e.g. the field should remain `int`).
- Do **not** use C-style casts `(float)x`; use `static_cast<float>(x)` to stay consistent with the rest of the codebase style.
