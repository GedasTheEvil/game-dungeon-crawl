# Part 1 — Critical Analyzer Issues

**4 warnings** across 3 files.
Checks: `clang-analyzer-valist.Uninitialized`, `clang-analyzer-deadcode.DeadStores`, `bugprone-integer-division`.

These are the highest-priority issues because two are real bugs (not style).

---

## 1. Uninitialized va_list — `src/graphics/font.cpp:17`

```
warning: Function 'vsprintf' is called with an uninitialized va_list argument
          [clang-analyzer-valist.Uninitialized]
```

**Problem:** `vsprintf` is called before `va_start` (or `va_start` is missing entirely), so the va_list is uninitialized — undefined behaviour.

**Fix:** Ensure `va_start` is called before `vsprintf` and `va_end` is called after. Pattern:
```cpp
va_list args;
va_start(args, fmt);
vsprintf(buffer, fmt, args);
va_end(args);
```

---

## 2. Integer division used in float context — `src/graphics/font.cpp:41`

```
warning: result of integer division used in a floating point context; possible loss of precision
          [bugprone-integer-division]
```

**Problem:** An expression like `a / b` where both operands are integers is computed as integer division, then widened to float — the fractional part is silently discarded.

**Fix:** Cast one operand to `float` before dividing:
```cpp
// before
float f = a / b;
// after
float f = static_cast<float>(a) / b;
```

---

## 3. Dead stores — `src/core/logger.cpp:38` and `src/graphics/ani.cpp:237`

```
warning: Value stored to 'levelStr' during its initialization is never read
          [clang-analyzer-deadcode.DeadStores]   (logger.cpp:38)

warning: Value stored to 'scale' is never read
          [clang-analyzer-deadcode.DeadStores]    (ani.cpp:237)
```

**Problem:** A variable is assigned a value that is never subsequently read — likely leftover from a refactor or a write-before-read bug.

**Fix options (pick whichever fits the intent):**
- If the variable is truly unused: remove the declaration entirely.
- If it should be used: find the missing read and restore it.
- If it's needed for side-effects only: restructure so no dead store occurs.
