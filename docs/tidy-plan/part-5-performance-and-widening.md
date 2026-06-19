# Part 5 — Performance and Widening

**6 warnings** across 2 files.
Checks: `performance-type-promotion-in-math-fn`, `bugprone-implicit-widening-of-multiplication-result`.

---

## 1. Float promoted to double in math calls — `src/graphics/particles.cpp:90–91`

```
warning: call to 'cos' promotes float to double [performance-type-promotion-in-math-fn]
warning: call to 'sin' promotes float to double [performance-type-promotion-in-math-fn]
```

**Problem:** `cos()` and `sin()` take `double`. Passing a `float` causes a promotion to double, a double-precision computation, then a narrowing back to `float` — unnecessary extra work.

**Fix:** Use the float overloads from `<cmath>`:

```cpp
// Before
pt[i].x += explosionForce * cos(randomAngle) * ...;
pt[i].y += explosionForce * sin(randomAngle) * ...;

// After
pt[i].x += explosionForce * std::cos(randomAngle) * ...;
pt[i].y += explosionForce * std::sin(randomAngle) * ...;
```

`std::cos(float)` and `std::sin(float)` resolve to the float overload, avoiding the promotion. Ensure `<cmath>` is included (not `<math.h>`).

---

## 2. Implicit widening of multiplication result — `src/graphics/ani.cpp:35–37, 49`

```
warning: performing an implicit widening conversion to type 'size_type' (aka 'unsigned long')
         of a multiplication performed in type 'int'
         [bugprone-implicit-widening-of-multiplication-result]
```

**Problem:** A multiplication of two `int` values is implicitly widened to `size_t` (e.g. when used as a `std::vector` index or in a `resize()` call). If the product overflows `int` before the widening, the result is undefined behaviour.

Lines: 35, 36, 37, 49 in `src/graphics/ani.cpp`.

**Fix:** Cast one operand to `size_t` before multiplying:

```cpp
// Before
vec.resize(width * height * channels);

// After
vec.resize(static_cast<size_t>(width) * height * channels);
```

Only one operand needs the cast — the multiplication will be performed in `size_t` from that point.

---

## Notes

- Both issues are in hot-path graphics code (`particles.cpp`, `ani.cpp`), so the performance fix (float math functions) is meaningful.
- After fixing `ani.cpp`, re-run `make tidy` to confirm the widening warnings are gone — there are 4 of them clustered near each other and they may share a single expression that is computed in multiple sub-expressions.
