# Stage 1: layered build and shared libraries

Status: implemented 2026-09-30 (see [Implementation](#implementation)), verified in play 2026-09-30 (up to level
9). Stage 1 of the [code structure review](../code-structure-review.draft.md).

## Why

* The tools get game code three different ways: the editor and the model viewer list game objects by hand, and
  levelcheck / levelgen recompile `src/world/level*.cpp` in one `g++` call with other flags (`-O2 -std=c++17`, no
  `-MMD`, headers listed by hand; `level_gen.h` is missing from levelcheck's list). Each list can drift from the game.
* Nothing stops a GL-free file from growing a GL, SDL or `Game()` dependency. The level tools only build because
  nobody added one yet.
* `core/timer.cpp` needs SDL only for `SDL_GetTicks`, so every tool that uses models or timers links SDL.
* `graphics/hud.cpp` is only used by the model viewer, yet the game builds and links it.
* `make tidy` skips `tools/level/*.cpp` and the model viewer.

## Libraries

Chosen by what the tools need today (include graph of 2026-09-30), not by directory. Files stay where they are.

| Library | Sources | May include | Used by |
|---|---|---|---|
| `build/liblevel.a` | `world/level`, `world/level_check`, `world/level_gen`, `world/campaign` | each other, the standard library | game, editor, levelcheck, levelgen |
| `build/librender.a` | `core/logger`, `core/timer`, `graphics/textures`, `graphics/font`, `graphics/animated_model`, `ui/ui_draw`, stb | each other, GL / GLU, stb, the standard library | game, editor, model viewer |
| (game) | every other `src/` file | anything | game |

Rules, checked by `tools/check_layers.sh` (run by `make layers`, and by `make tidy`):

* A library file's quoted includes resolve to headers of the same library (or `external/`).
* No SDL in either library, no GL in `liblevel`, no `Game()` in either.

## Steps

1. **Timer without SDL.** `GameClock::now()` reads `std::chrono::steady_clock` (ms since the first call) instead of
   `SDL_GetTicks` (ms since `SDL_Init`). Both start near 0 at startup; nothing reads the absolute value.
   The viewer uses `GameClock::now()` instead of `SDL_GetTicks` and drops `SDL_Init`, so it links no SDL.
2. **`hud.{h,cpp}` to `tools/model-viewer/`.** The game no longer builds it; fix the viewer comment that says the
   game's bars use it.
3. **Makefile libraries.** `LEVEL_LIB` and `RENDER_LIB` source lists, `ar` rules, all built with the game's
   `CXXFLAGS` into `build/`. The game links `$(APP_OBJECTS) librender.a liblevel.a`; the editor links its objects and
   both libraries with GL libs only (no SDL); the viewer `librender.a` with GL libs; levelcheck and levelgen their one
   object and `liblevel.a`, no GL. Dependency files for every object. `levelcheck` / `levelgen` stay in the repo root
   (docs and AGENTS.md call `./levelcheck`).
4. **Layer check.** `tools/check_layers.sh`, target `layers`; `tidy` runs it first.
5. **Tidy covers the tools.** `tools/level/*.cpp` and `tools/model-viewer/viewer.cpp` join `tidy` / `tidy-fix`;
   fix what it reports. `tidy-fix` into `.PHONY`.

## Checks per step

`make`, `make editor model-viewer level-tools`, `make tidy`, `make test`, `./levelcheck levels/lvl*` (no warnings).

## Not in this stage

* Breaking the `state/game_state.h` hub, moving ids out of `ui/inventory.h` (stage 2) or making `Dungeon` GL-free
  (stages 4-6). Once stage 2 moves the item ids, `world/loot` can join `liblevel` and the id copies in
  `level_gen.cpp` / `tile_info.cpp` can go.
* Moving files into per-layer directories. Worth it once the layers are real (after stages 2-6); today most of
  `world/` is not GL-free.

## Implementation

Done 2026-09-30, as planned:

* `GameClock::now()` uses `std::chrono::steady_clock`. The model viewer uses it too and has no `SDL_Init`.
* `hud.{h,cpp}` live in `tools/model-viewer/`.
* `make` (target `all`) builds the game, the editor, the model viewer, levelcheck and levelgen. `ldd`: the editor
  and viewer link no SDL, the level tools no GL.
* `tools/check_layers.sh` (`make layers`, run by `make tidy`). Tested by adding a `game_state.h` include and a
  `Game()` call to `campaign.cpp`: both reported.
* `make tidy` covers `tools/level/*.cpp` and the viewer (`.clang-tidy` header filter includes
  `tools/model-viewer`). The viewer's `g_*` / `k*` names became `gCamelCase` / `UPPER_CASE`, two narrowing
  conversions fixed.
* Checks: `make` from clean with no warnings, `make tidy` clean, 58/58 scenarios, `./levelcheck levels/lvl*` output
  byte-identical to a binary built the old way (`-O2 -std=c++17`), `levelgen` output identical for seeds 1, 7, 42.
  Editor and viewer start under Xvfb.
* Docs: [../development.md](../../development.md) (Libraries).
