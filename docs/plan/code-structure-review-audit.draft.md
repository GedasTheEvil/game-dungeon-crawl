# Code structure audit

Snapshot of 2026-09-30, taken with the uncommitted teleporter work in the tree. Line numbers drift as the code changes,
so re-check an area before planning a change to it. Research to compare against: [the research](code-structure-review-patterns.draft.md).
Back to [the plan](code-structure-review.draft.md).

## Size

About 14.3k lines of C++ in `src/` and `tools/`.

| Area | Lines | Largest files |
|---|---:|---|
| `src/world` + `src/entities` | 4.5k | `level_check.cpp` 550, `level_gen.cpp` 482, `dungeon_decor.cpp` 464 |
| `src/ui`, `state`, `core`, `input`, `test` | 5.9k | `scenario.cpp` 939, `inventory.cpp` 907, `menu.cpp` 681 |
| `src/graphics` | 2.3k | `animated_model.cpp` 345, `fire.cpp` 312, `draw.cpp` 245 |
| `tools/` C++ | 1.8k | `editor.cpp` 782, `viewer.cpp` 435, `levelcheck.cpp` 259 |

## What we have (good patterns)

* **Type object / flyweight:** `MonsterType` is shared by pointer (`monster.h:40-58`), with the defs in the
  `MONSTER_DEFS` / `ITEM_DEFS` tables (`assets.cpp:25, 103`). `CharacterModel` clips are shared, and each instance
  keeps its own playback (`character_model.h:14`).
* **Data tables:** `DECAL_DEFS`, `*_STYLE_NAMES` (`decor.h`), `WEAPON_GRADES` (`loot.cpp:15`), the clip lists, and
  the level_gen `PICKS`/`WEIGHTS`.
* **State machines, small ones:** the bat flight phases (`monster_ai.cpp:96-146`) and `Locomotion`-based behaviour.
* **Object pool:** `Monster monsters[MAX_MONSTERS]` with slot reuse; copying `Monster` is deleted.
* **GL-free core shared with tools:** `level.h`, `level_check` and `level_gen` have no GL and no `Game()`.
* **Determinism:** seeded hashes for decor (`dungeon_decor.cpp:37-56`); `Rng` (splitmix64) in level_gen; stateless
  hashed effects in `fire.cpp`; the virtual `GameClock` plus `srand(seed)` in scenarios.
* **Input action mapping:** keys map to `GameplayAction` (`input_actions.h`), and scenarios reuse the same path
  (`executeGameplayAction`).
* **Asset registry:** `Assets` is a set of typed groups, loaded once with progress (`assets.h:67-84`).
* **Ownership:** `unique_ptr` is used almost everywhere. The only raw `new` is `new Logger()` (`logger.cpp:21`).
* **Graceful fallback:** a shader or FBO failure falls back to fixed-function rendering or no outlines.
* **Scenario tests:** 48 scripts, run in parallel under Xvfb.

## Anti-patterns

### God objects and global state

* **`GameState` / `Game()`**, about 400 calls in `src/` (`input.cpp` 77, `draw.cpp` 52, `scenario.cpp` 47,
  `dungeon_render.cpp` 24, ...).
  * It holds assets, camera, render settings, player, dungeon, all UI screens, status, save names and timers
    (`game_state.h:55-69`).
  * It also draws the loading screen (`DrawLoad`, with GL and a buffer swap) and does save serialization.
  * Many places write to it: input, menu, inventory, riddle, scenario and draw.
* **`Dungeon`**, about 1580 lines over 7 `.cpp` files, about 70 methods. It has 9 responsibilities:
  * tile map
  * player position and physics (`mapX`/`mapY` live here, not in `Player`)
  * fog of war
  * mechanisms
  * monster pool and AI dispatch
  * arrows
  * decoration
  * lighting and rendering
  * save/load and interaction (with UI side effects)

  It writes `Game().player->jump.*` about 20 times.
* **Other globals:**
  * `int window, fs` (`game.cpp:16`), `lastKey`/`lastMx`/`lastMy` (`input.cpp:8`)
  * `gRunner`, `gVirtualClock`, `gAudioUsers`
  * `gHitStreak`/`gLastHitMs` (`trap.cpp:9`)
  * file-static shader and FBO state (`lighting.cpp:83`, `ink.cpp:69`)
  * about 15 `g_` globals in `viewer.cpp`, and the global `editor` pointer
* **Unseeded global `rand()`:** loot, blood, player and character_model, even though level_gen has its own `Rng`.
* **`scatterSurfaces` reads `Game().curMap`** (`dungeon_decor.cpp:397`): decor depends on global state, not only on
  the level.

### Simulation mixed with rendering

* `Dungeon::Draw()` spawns monsters and advances `portalScroll` and `riddleMarkYaw`. It does `rotA++` on shared item
  assets and sets `club->scale = 10` every frame (`dungeon_render.cpp:89-178`).
* **Trap damage is dealt inside `Trap::Show()`**, and one hurt timer is shared per trap kind. This bug has its own
  plan: [trap-and-font-bugs.draft.md](trap-and-font-bugs.draft.md).
* `Monster::Draw` and `Player::Draw` advance model state and the blood particles.
* The attack hit resolution `updateAttack` lives in `graphics/draw.cpp:93`.
* `Update()`/`Draw()` live in `graphics/draw.cpp` but are declared in `input/input.h:47`.
* Each UI screen ends the frame itself (ortho, `onFrameRendered`, `glutSwapBuffers`).

### Game loop

* GLUT timer every 16 ms, with no dt. Gameplay timers use the wall clock (`GameClock::now()`).
* Movement runs once per key event, so its speed depends on OS key repeat.
* Input callbacks change state right away, outside `Update`.
* Scenarios need GL and a window (Xvfb), because Draw advances animations. There is no headless sim.

### Per-type if-chains (shotgun surgery)

* `Tile{int type, attr, value}`: what `attr` and `value` mean depends on the type. Gate and rock state are the ints
  0, 2 and 1.
* The teleporter needed edits in about 7 places: `level.h`, `dungeon_io.cpp` `Interact`,
  `dungeon_render.cpp` `Draw`, `level_check.cpp` (4 functions), the editor and `ascii2level`.
* `Draw()` is an if-chain over 12 tile types; the Door branch alone is 55 lines inline.
* Other copies of type checks: `DrawTreasureTile` (magic 1/2/3), `Interact`, `level_check.cpp:151/285/489`, and the
  decor skip lists (`dungeon_decor.cpp:159/185`).
* The teleporter is `Door` + `attr=GateTeleport` + `value=pair id`. Having both a `GateType` enum and a `Gate` tile
  type is confusing.
* `ItemType`/`PotionId` are int constants, not enums. Potion effects are a `switch` in `Inventory`.

### Game logic in UI

* **`Inventory`** (907 lines) is also the item model:
  * weapon level math (`inventory.cpp:118`)
  * use rules (`CanUse`)
  * potion effects (`DrinkPotion`, with magic 25/50/2/5)
  * the save format and legacy migration

  `loot.cpp` and `PlayerStats::Damage` depend on it.
* **`MainMenu`** does the save-slot file I/O and calls `exit(666)`. Its sub-screens are the bools
  `saveD/loadD/optionsD/creditsD`.
* **`Riddle`** computes and grants the XP reward.
* **`PlayerActionController`** holds the jump physics setup and the camera clamp.
* **`Dungeon::PickUp`/`Interact`** call `ui.inventory->AddItem`, `ui.riddle->Ask()` and set `->show` directly.

### Screens

* There is no FSM. `GetDrawScreen` (`screen_state.h:15`) picks from 4 separate `show` bools in priority order,
  and nothing enforces that only one is open.
* Code copied into every screen:
  * `CANVAS_W/H` (4 copies)
  * the `visibleArea()` wrapper (3)
  * the four-font load (3)
  * toast timing and draw (2)
  * the local `GLUT_BUTTON_UP` (2)
  * the frame-end sequence (4)
  * the click press/hover/release logic (2)

### Layering violations

Include direction today (arrow = "includes"):

* **Cycle:** `state/game_state.h` includes world, entities and all ui screens, and almost every world, entities and
  ui `.cpp` includes `game_state.h` back.
* **world → ui:** `loot.cpp:2` includes `ui/inventory.h` just for the item ids.
* **entities → graphics in headers:** `monster.h`, `player.h`, `item.h` and `trap.h` pull in `AnimatedModel`,
  `Texture` and `ParticleSystem`. The sim types cannot build without GL.
* **graphics → state:** `lighting.cpp` and `ink.cpp` read `Game().render` and `resX/resY`. `hud.cpp` → `world/level.h`
  just for `LOCK_COLOUR_COUNT`. `draw.cpp` → input, ui, state, test.
* **ui/entities → test:** inventory, menu, riddle and map_view include `test/scenario.h` for `onFrameRendered`;
  `player.cpp` includes it for `godMode()`.
* **core → everything:** `core/game.cpp` is `main` and wires it all together. That is fine for `main`, but it lives
  in `core/`.
* `core/timer` needs SDL, and `animated_model.h`/`particles.h` include it, so any tool that uses models needs SDL.
* **Clean:** `level*`, `campaign`, `logger`, `gameplay_config`, `ui_draw`, `textures`, `font`, `animated_model`,
  `particles`.

### Duplication

* **Game vs tools:**
  * levelcheck `GRIP = 0.45f` copies `LADDER_GRIP_X`, and `GATE_STOP = 0.4f` shadows `GATE_APPROACH = 0.45f`.
  * `tile_info.h` hard-codes `TILE_COUNT = 14`.
  * `tile_info.cpp` `LOCK_COLOURS` copies `LOCK_COLOUR_NAMES`/`LOCK_GEM_NAMES`.
  * The item type constants are copied in `level_gen.cpp:19` and `tile_info.cpp:63`, because `inventory.h` pulls in GL.
  * The viewer's texture fallback and its perspective/ortho setup copy the game's.
  * GL init and GLUT `main` boilerplate exist 3 times.
* **Movement rules:** the `level_check` `Walker` re-implements `Dungeon::Move` by hand, and the two can drift apart.
* **Inside the game:**
  * the decor seed hash (5 copies in `dungeon_decor.cpp`)
  * the portal quad and scroll math (2), the `rock` lambda (2)
  * the view-window origin `mapX-3/-4`, `mapY-3` (6 files)
  * the blood/`TakeHit` logic (monster vs player)
  * shader compile and link (lighting vs ink)
  * the particle reset (2)

### Long functions (over ~75 lines)

| Function | Location | Lines |
|---|---|---:|
| `checkLevel` | `level_check.cpp:309` | 163 |
| `Assets::Load` | `assets.cpp:174` | 162 |
| `parseLine` (if-chain on command name) | `scenario.cpp:386` | 156 |
| `Dungeon::Draw` | `dungeon_render.cpp:154` | 122 |
| `PathScript` (class) | `levelcheck.cpp:29` | 115 |
| `runInstant` | `scenario.cpp:630` | 112 |
| `Draw()` | `graphics/draw.cpp:148` | 97 |
| `SunBeam::draw` | `fire.cpp:218` | 94 |
| `DraftMap::Draw` | `map_view.cpp:270` | 86 |
| `levelcheck main` | `levelcheck.cpp:177` | 82 |
| `drawIcon` | `menu.cpp:168` | 81 |
| `renderLevel`, `MainMenu::DrawOptions`, `viewer main` | | 78 each |

### Resources and memory

* **GL resources without RAII:**
  * `Texture` has no destructor and is copyable.
  * `Font` has a destructor but is copyable (risk of a double delete).
  * The `AnimatedModel` display lists and the fire sprite, shader programs and ink FBO are never freed.
* **No VBOs:** everything is immediate mode, client arrays or display lists.
* **Unbounded buffer:** `font.cpp:64` `vsprintf`. This bug has its own plan:
  [trap-and-font-bugs.draft.md](trap-and-font-bugs.draft.md).
* **Fixed buffers:** many fixed `char[]` + `snprintf` (inventory, assets, scenario), and `SaveName{char name[25]}`
  with `strncpy`.

### Save format

* Ad-hoc text written with `ofstream <<` inside `Dungeon`, `Inventory` and `PlayerStats`.
* Only the inventory has a version tag (`INV2`). The other sections rely on stream order, and `LoadDump` guesses the
  version by trying reads until one fails.

### Magic numbers

Mostly in rendering and UI:
* `glTranslatef(20,0,-25)`, `glTranslatef(40,0,0)` instead of `TILE_SIZE`, `40*x-20`, `-30` (`dungeon_render`,
  `monster`, `player`, `trap`)
* `draw.cpp:179-223`, `ink.cpp:192 720.f`
* camera yaw `-110/70`, `player->scale = 15`, `exit(666)`

## What we don't have

* A split between simulation and rendering. The sim cannot run without GL.
* A fixed timestep with dt. Movement depends on key repeat.
* An event list: gameplay calls sound, `ShowStatus`, `AddXP` and the UI directly.
* A per-tile-type definition table (the tile kind of `MONSTER_DEFS`).
* A screen stack or screen FSM, and a shared UI screen base or canvas helper.
* A player entity that owns its position and physics.
* One movement model shared by the game and `Walker`.
* A gameplay RNG owned by the world.
* A shared view-window/camera helper.
* A layered build: there is no static library; the tools list object files by hand, and levelcheck/levelgen
  recompile world sources with other flags. `make tidy` skips the viewer and level tools.
* Unit tests for pure logic (riddle parsing, inventory rules, save round-trip, checker rules).
* Save format versioning.
* A teleporter lookup index: `teleportPartner` scans the whole level on every call.
