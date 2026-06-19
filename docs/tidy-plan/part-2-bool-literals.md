# Part 2 — Bool Literals

**36 source locations** (each fires two paired warnings).
Checks: `modernize-use-bool-literals` + `readability-implicit-bool-conversion`.

---

## What the warnings mean

`modernize-use-bool-literals` fires when an integer literal (`0` or `1`) is used where a `bool` is expected — e.g. in a `bool` field initialiser or a `bool` return.

`readability-implicit-bool-conversion` fires on the same line because the compiler silently widens/narrows the integer to bool.

Both are fixed by the same change: replace `0` with `false` and `1` with `true`.

---

## Affected files

| File | Occurrences (locations) |
|------|------------------------|
| `src/core/sound.cpp` | 12 |
| `src/entities/item.cpp` | 7 |
| `src/entities/monster.cpp` | 6 |
| `src/graphics/ani.cpp` | 4 |
| `src/core/timer.cpp` | 4 |
| `src/entities/monster_ai.cpp` | 2 |
| `src/entities/trap.cpp` | 1 |

---

## Fix pattern

```cpp
// Before
bool loaded = 0;
bool active = 1;
return 0; // in a bool-returning function

// After
bool loaded = false;
bool active = true;
return false;
```

---

## Notes

- `sound.cpp` has the most occurrences (12) — likely SDL return values assigned to `bool` fields without an explicit cast.
- `make tidy-fix` with `--fix` can apply most of these automatically. Review the diff afterwards to confirm no semantic change.
- The `.clang-tidy` config sets `AllowIntegerConditions: 1` and `AllowPointerConditions: 1`, so `if (ptr)` and `if (count)` are intentionally not flagged — only assignments and initialisers are in scope here.
