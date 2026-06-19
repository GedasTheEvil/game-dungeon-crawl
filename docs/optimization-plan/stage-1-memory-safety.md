# Stage 1: Memory Safety & Resource Management

**Goal:** Eliminate manual `new`/`delete` pairs and make the codebase exception-safe
through consistent RAII. This is the highest-risk category — raw pointer misuse is the
most common source of real bugs (double-free, leak, use-after-free).

---

## Anti-patterns addressed

| # | Anti-pattern | Severity |
|---|---|---|
| 1 | Raw `new`/`delete` pairs mixed with `unique_ptr` | Critical |
| 2 | Partial construction leaks (no RAII on multi-step init) | High |
| 3 | Fixed `char[]` buffers with `sprintf` — potential overflow | Medium |
| 4 | `SDL_Init()` called per-`timer` constructor | Medium |
| 5 | `texture = new TextureImage[1]()` array allocation | Medium |

---

## 1.1 — Convert raw timer pointers

**Affected files:** `src/entities/item.cpp`, `src/ui/menu.cpp`,
`src/ui/inventory.cpp`, `src/graphics/particles.cpp`, `src/entities/trap.cpp`,
`src/graphics/ani.cpp`

Every class that holds a `timer*` member and manually `delete`s it in its destructor
should instead hold `std::unique_ptr<timer>`.

### Pattern to apply

```cpp
// Before
timer* frameChange;
// constructor:  frameChange = new timer(100);
// destructor:   delete frameChange;

// After
std::unique_ptr<timer> frameChange;
// constructor:  frameChange = std::make_unique<timer>(100);
// destructor:   (nothing — handled automatically)
```

Classes to fix (all follow the same pattern):

- `AnimatedCartoonModel` — `timer* frameChange`
- `ParSys` — `timer *ani_e, *ani_f`
- `Item` — any timer members
- `Trap` — `timer* Hurt_timer`
- `MainMenu` — `timer* t_cred`
- `inventory` — `timer* inv_ani`

### Acceptance criteria
- `grep -r "new timer" src/` returns 0 results.
- `grep -r "delete.*timer" src/` returns 0 results.

---

## 1.2 — Convert raw model pointers

**Affected files:** `src/entities/item.cpp`, `src/entities/trap.cpp`

```cpp
// Before
AnimatedCartoonModel* mdl;
// constructor:  mdl = new AnimatedCartoonModel();
// destructor:   delete mdl;

// After
std::unique_ptr<AnimatedCartoonModel> mdl;
// constructor:  mdl = std::make_unique<AnimatedCartoonModel>();
```

### Acceptance criteria
- `grep -rn "new AnimatedCartoon" src/` returns 0 results outside of factory/make_unique calls.

---

## 1.3 — Fix `AnimatedCartoonModel` vertex array allocation

**File:** `src/graphics/ani.cpp`, lines 52–67

Currently allocates `Ver` (array of `VF`) and each `Ver[i].v` (float array) as raw
heap arrays. If any inner allocation throws, the outer allocations leak.

**Option A (minimal change):** Replace with `std::vector<VF>` where `VF.v` becomes
`std::vector<float>`. The destructor then becomes trivial and the allocation is
exception-safe automatically.

**Option B (if VF is a simple POD):** Keep `VF` struct but make `v` a
`std::unique_ptr<float[]>` or `std::vector<float>`.

### Acceptance criteria
- Destructor of `AnimatedCartoonModel` has no manual `delete` / `delete[]` calls.
- Valgrind (or AddressSanitizer) reports no leaks from model loading.

---

## 1.4 — Fix `Textura` heap allocation

**File:** `src/graphics/textures.cpp`, line 11

```cpp
// Before
texture = new TextureImage[1]();
// destructor: delete[] texture;

// After
std::unique_ptr<TextureImage[]> texture;
// or: TextureImage texture; (if only one element is ever used)
```

Single-element arrays are a smell — if only one image is ever needed, make `texture` a
plain value member.

---

## 1.5 — Fix `SDL_Init` repeated calls

**File:** `src/core/timer.cpp`, lines 6–7

`SDL_Init(SDL_INIT_EVERYTHING)` is called every time a `timer` is constructed. This
is wasteful and technically incorrect (though SDL tolerates it).

Move SDL initialization to `main()` (or `game.cpp`) and remove it from the `timer`
constructor. Add a matching `SDL_Quit()` on shutdown.

---

## 1.6 — Replace unsafe `char[]` / `sprintf` with `std::string`

**Affected files:**

| File | Issue |
|---|---|
| `src/entities/monster.cpp:182` | `sprintf(tmp1, "Models/%s.mdl", filename)` — no bounds check |
| `src/ui/menu.cpp:60–61` | `char filename[20]` for path that could exceed 20 bytes |
| `src/state/cashe.h:31` | `char status[255]` public member |

**Fix:** Replace with `std::string` / `std::format` (C++20) or `fmt::format`.
Where building paths, prefer `std::filesystem::path` concatenation.

```cpp
// Before
char tmp1[255];
sprintf(tmp1, "Models/%s.mdl", filename);
walk->Load(tmp1);

// After
const std::string path = "Models/" + std::string(filename) + ".mdl";
walk->Load(path);
```

### Acceptance criteria
- `grep -rn "sprintf" src/` returns 0 results.
- `grep -rn "char.*\[" src/` returns only intentional fixed-size buffers (e.g., OpenGL name buffers).

---

## Effort estimate

| Task | Estimated time |
|---|---|
| 1.1 Timer pointers (6 classes) | 1–2 h |
| 1.2 Model pointers (2 classes) | 30 min |
| 1.3 AnimatedModel vertex arrays | 2–3 h |
| 1.4 Textura allocation | 30 min |
| 1.5 SDL_Init fix | 15 min |
| 1.6 char[] → std::string | 1–2 h |
| **Total** | **5–9 h** |

---

## Done criteria for Stage 1

- [ ] No raw `new`/`delete` remains in any class that owns a resource.
- [ ] AddressSanitizer (`-fsanitize=address`) reports zero leaks on startup + load + quit.
- [ ] `grep -rn " new " src/` returns only placement-new or factory-delegated calls.
- [ ] Game compiles and runs without behavioural regression.
