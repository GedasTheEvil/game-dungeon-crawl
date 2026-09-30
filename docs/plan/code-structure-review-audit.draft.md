# Code structure audit

Re-checked 2026-09-30 at `bfd0ec8` (after the teleporter, boss framework, hitboxes, trap fix and HUD redesign), then
updated for [stage 1](solved/layered-build.md) (`229f2fa`). First version: `743fff9`. Line numbers drift as the code changes,
so re-check an area before planning a change to it. Marks: **fixed**, **worse** (grew since the first audit),
**new** (not in the first audit). Research to compare against: [the research](code-structure-review-patterns.draft.md).
Back to [the plan](code-structure-review.draft.md).

## Size

About 15k lines of C++ in `src/` and `tools/`.

| Area | Largest files |
|---|---|
| `src/world` + `src/entities` | `dungeon_*.cpp` 1785 over 7 files (was ~1580), `level_check.cpp` ~600, `dungeon_decor.cpp` 464 |
| `src/ui`, `state`, `core`, `input`, `test` | `scenario.cpp` 984, `inventory.cpp` 951, `menu.cpp` ~680 |
| `src/graphics` | `animated_model.cpp` 354, `fire.cpp` 312, `draw.cpp` 300 |
| `tools/` C++ | `editor.cpp` ~780, `viewer.cpp` ~430, `levelcheck.cpp` ~270 |

## What we have (good patterns)

* **Type object / flyweight:** `MonsterType` shared by pointer, defs in `MONSTER_DEFS` / `BOSS_DEFS` / `ITEM_DEFS`
  (`assets.cpp:23, 108, 125`). `CharacterModel` clips shared, playback per instance.
* **Data tables:** `DECAL_DEFS`, `*_STYLE_NAMES`, `WEAPON_GRADES`, clip lists, level_gen `PICKS`/`WEIGHTS`.
* **Small state machines:** bat flight phases, `Locomotion`.
* **Object pool:** `Monster monsters[MAX_MONSTERS]` with slot reuse, copy deleted.
* **Shared libraries** (stage 1): `liblevel.a` (GL-free) and `librender.a`, checked by `tools/check_layers.sh`.
* **Determinism:** seeded decor hashes, `Rng` in level_gen, hashed fire, virtual `GameClock`, `srand(seed)` in
  scenarios.
* **Input action mapping:** `GameplayAction` (7 actions), shared by scenarios for walk / jump / attack / interact.
* **Asset registry:** `Assets` groups (`assets.h:17-88`), loaded once with progress.
* **Value-parameter UI (new):** `PlayerHud`, `BossBar`, `StatusBox` take plain values, no `Game()`; `quickPotion()` is
  a pure function.
* **Ownership:** `unique_ptr` almost everywhere.
* **Graceful fallback:** shader / FBO failure falls back to fixed function / no outlines.
* **Scenario tests:** 58 scripts (was 48), parallel under Xvfb.

## Anti-patterns

### God objects and global state

* **`GameState` / `Game()`:** 438 calls in `src/` (was ~400). Top: `input.cpp` 81, `draw.cpp` 59, `scenario.cpp` 54,
  `inventory.cpp` 26, `dungeon_render.cpp` 25, `dungeon_mechanisms.cpp` 25, `dungeon_base.cpp` 21.
  * Holds assets, camera, render settings (now with the debug flag `Hitboxes`), player, dungeon, all UI screens,
    status, save names, timers (`game_state.h:56-68`).
  * Draws the loading screen with raw GL and a buffer swap (`DrawLoad`, `game_state.cpp:86-142`); save and load
    (`:144-191`).
* **`Dungeon`, worse:** 1785 lines, ~81 methods, now 10 responsibilities: tile map, player position and physics
  (`mapX`/`mapY`), fog, mechanisms, monster pool and AI, arrows, decor, lighting and rendering, save/load with UI side
  effects, and (new) the boss-fight director (`dungeon_monsters.cpp:145-208`) plus trap damage (`updateTraps`).
  * New query API for HUD and tests: `Boss`, `BossHealth`, `LivingMinions`, `NearestMonsterHealth`,
    `MonsterBarsShown`, scenario-only `SlayBoss` (`dungeon.h:156-164`).
  * Writes `Game().player->jump.*` 12 times (`dungeon_base.cpp:38-141`); 116 `Game()` calls in world + entities.
* **Other globals:** `window`, `fs` (`game.cpp:16`; `fs` is never set, fullscreen is dead code), `lastKey`/`lastMx`/
  `lastMy`, `gRunner`, `gVirtualClock`, `gAudioUsers`, file-static shader / FBO / sprite state (`lighting.cpp:83`,
  `ink.cpp:69`, `fire.cpp:33`), the viewer's 19 `g*` globals, the global `editor` pointer. `gHitStreak` / `gLastHitMs`:
  **fixed** (trap fix).
* **Unseeded randomness, worse:** `rand()` in `loot.cpp`, `character_model.cpp`, `particles.cpp` (8 calls),
  `riddle.cpp`; POSIX `random()` in `player.cpp:104, 112` and `monster.cpp:93, 105`. Scenarios seed only `srand`,
  so the blood spots stay nondeterministic.
* **`scatterSurfaces` reads `Game().curMap`** (`dungeon_decor.cpp:397`).

### Simulation mixed with rendering

* **`Dungeon::Draw` does simulation, worse:** `portalScroll`, `SpawnMonster` (which now also starts the boss fight and
  summons minions: `dungeon_monsters.cpp:296-297, 145-152`), `riddleMarkYaw`, `rotA++` on shared item prototypes
  and `club->scale = 10` in `DrawTreasureTile` (`dungeon_render.cpp:80-114`).
* `Monster::Draw` / `Player::Draw` change model state, facing, frame advance and blood (`monster.cpp:167-225`,
  `player.cpp:62-96`).
* **New:** `PlayerHud`'s `DamageTrail` is file-static state updated inside `draw()` from `GameClock::now()`, never
  reset on a new game or a load (`player_hud.cpp:56-78`). `drawWeapon` changes and restores the shared
  `weapon->rotA` (`draw.cpp:83-91`).
* `Update()` / `Draw()` live in `graphics/draw.cpp` but are declared in `input/input.h`; `Update()` runs the sim and
  `updateAttack` resolves hits there (`draw.cpp:95-114, 167-196`).
* Trap damage in `Trap::Show()`: **fixed** (`Dungeon::updateTraps`). New coupling: the trap hitbox reads the render
  asset's `scale` (`dungeon_base.cpp:116`).
* Each screen ends its own frame: 5 copies of glFlush + `onFrameRendered` + swap (`menu`, `inventory`, `riddle`,
  `map_view`, `draw.cpp`), plus `DrawLoad`.

### Game loop

* GLUT timer every 16 ms with no dt; gameplay timers on `GameClock::now()` (now `std::chrono`, stage 1).
* Movement once per key event, so its speed follows OS key repeat (`input.cpp:47-63`). Scenario `walk` moves once
  per 16 ms tick, so scripted speed differs from real play.
* Input callbacks change state outside `Update` (camera, render toggles, jump start, screens, quick drink).
* Scenarios need GL and Xvfb because Draw advances animations (1x1 scissor on frames without a screenshot,
  `scenario.cpp:942-951`).
* **New:** two projections for the same scene: `reSizeGlScene` (`gluPerspective(45, …, 0.1, 10000)`, `game.cpp:58`)
  and `Draw` (`SCENE_NEAR 10`, `SCENE_FAR 300`, `draw.cpp:227`).

### Per-type if-chains (shotgun surgery)

* `Tile{type, attr, value}`: the meaning of `attr` / `value` depends on the type; gate and rock state are the ints
  0/1/2. **Worse:** a gate's `attr` can now also be `BOSS_LOCK` (5), special-cased in `dungeon_mechanisms.cpp:149,
  222` and 6 places in `level_check.cpp`.
* **A new tile kind touches ~12-20 places** (was "~7"). Full lists for the teleporter, the boss gate and the boss
  monster: [Tile descriptions](#tile-descriptions).
* `Dungeon::Draw` is an if-chain of 12 branches (`dungeon_render.cpp:174-255`); the Door branch is 54 lines inline.
  `DrawTreasureTile` uses magic `attr` 1/2/3 and `value` 0/1/2 instead of the item ids. `Interact` is an if-chain
  (`dungeon_io.cpp:114-129`).
* `level_check.cpp` per-type logic: `Walker::solid`, `hazardCost`, `isGoal`, `countContent`, `checkDeadGates`,
  `renderLevel`, `monsterThreat`, and `MONSTER_CHARS`, which must match the `MonsterTypeId` order.
* Decor skip lists (`dungeon_decor.cpp:72, 159, 185`). `GateType` enum next to the `Gate` tile type; the teleporter
  is Door + `GateTeleport` + `value` = pair id.
* **New:** two sources of "is a boss": `isBossMonster` hard-codes `MonsterBossScarab` (`level.h:54`) while
  `MonsterType::isBoss()` comes from `BOSS_DEFS`. Dead field `BossRules::lifeStealPct`. Boss and mimic state kept by
  rewriting map tiles (`dungeon_monsters.cpp:68, 163`).
* `ItemType` / `WeaponId` / `PotionId` are int constants (`inventory.h:11-33`). **New** id switch: `weaponIcon`
  (`draw.cpp:132-143`).

### Game logic in UI

* **`Inventory`, worse:** 951 lines. Weapon level math (`upgradeCost`, `weaponDamage`, `inventory.cpp:118-123`),
  `CanUse` (reads `Game().player`), `DrinkPotion` (heal percent now named in `PotionEffect`, magic 2/2/5 left), new
  `QuickDrink` / `QuickChoice` rules with `ShowStatus`, the save format and legacy migration. Included by `loot.cpp`,
  `dungeon_monsters.cpp` (new, for the hitbox view), `draw.cpp`, `scenario.cpp`, `game_state.h`, `quick_potion.cpp`
  (new). `PlayerStats::Damage` calls `Game().ui.inventory`.
* **New:** the quick potion keys (`Inventory::IsQuickHealKey` / `IsQuickStaminaKey`) and the equip hotkeys bypass
  `GameplayAction` (`input.cpp:85-91, 189-190`); the HUD key labels are hard-coded separately (`draw.cpp:160-162`).
* **New:** the HUD view model (`quickSlot`, `weaponIcon`, `playerHudView`) is built in `graphics/draw.cpp:116-164`.
* **New:** the XP progress ratio is computed in 3 places (`draw.cpp:154`, `inventory.cpp:846`, `riddle.cpp:44`).
* `MainMenu`: save-slot file I/O (`menu.cpp:274-317`), `exit(666)`, sub-screen bools `saveD/loadD/optionsD/creditsD`.
* `Riddle` computes and grants its XP reward (`riddle.cpp:42-46, 257`).
* `PlayerActionController` holds the jump setup (`startJump`, `input.cpp:14-40`) and the camera clamp.
* `Dungeon::PickUp` / `Interact` call `AddItem`, `riddle->Ask()` and set `show` directly.

### Screens

* No FSM: `GetDrawScreen` (`screen_state.h:15-29`) picks from 4 `show` bools (menu, inventory, riddle, map);
  only ad hoc guards in `input.cpp` keep one open.
* Copies: `CANVAS_W/H` (4), `visibleArea()` wrapper (3), four-font load (3, plus `fonts.hudSmall`), toast (2, plus
  `StatusBox`'s own fade), local `GLUT_BUTTON_UP` (2), frame end (5), click press/hover/release (2).
* **New:** square-canvas ortho setup in 5 files (`map_view`, `level_gem`, `boss_bar`, `status_box`, `player_hud`)
  with different z ranges; the GL-state reset tail in 3 (`boss_bar`, `status_box`, `player_hud`); `BLOOD_TOP/BOTTOM`
  and the lock-gem palette copied (`boss_bar`, `player_hud`, `map_view`, `level_gem`, `tile_info`).

### Layering

After stage 1, `liblevel` and `librender` are clean. The rest:

* **Hub and cycle:** `state/game_state.h` includes world, entities and 5 ui screens; 24 files include it back.
  Every directory pair except a few forms a cycle through it.
* **world → ui:** `loot.cpp` includes `ui/inventory.h` for the ids; **new:** `dungeon_monsters.cpp` for the hitbox
  view. `quick_potion.cpp` (new) the same, so the pure `quickPotion()` needs GL to build.
* **world → graphics:** every `dungeon_*.cpp` includes `<GL/gl.h>` and `render_config`, `lighting`, `fire` or `ink`.
  Debug hitbox drawing sits in `Dungeon` (`dungeon_monsters.cpp:234-269`).
* **entities → graphics in headers:** `monster.h`, `player.h`, `item.h`, `trap.h` pull in `AnimatedModel`, `Texture`,
  `ParticleSystem`, `Sound`.
* **graphics → state:** `lighting.cpp`, `ink.cpp` read `Game().render`; `draw.cpp` includes input, test, state and 6
  ui headers. `hud.cpp` → `world/level.h`: **fixed** (moved to the viewer in stage 1).
* **ui / entities → test:** `inventory`, `menu`, `riddle`, `map_view` include `test/scenario.h` for
  `onFrameRendered`; `player.cpp` for `godMode()`. **New:** `player_hud.cpp` → `world/level.h` for
  `LOCK_COLOUR_COUNT`; `assets.h` → `world/level.h`, `world/decor.h`.
* `core/game.cpp` is `main` and wires everything. `core/timer` needing SDL: **fixed** (stage 1).

### Duplication

* **Game vs tools:** levelcheck `GRIP = 0.45f` copies `LADDER_GRIP_X` (anonymous namespace in `dungeon_base.cpp:15`);
  `GATE_STOP 0.4` vs `GATE_APPROACH 0.45`; `TILE_COUNT = 14` in `tile_info.h`; lock colours copied twice in
  `tile_info.cpp` (`LOCK_COLOURS`, `GATE_COLOURS`; the boss gem "Obsidian" is only in the editor readme and
  `mechanism.py`); item ids copied in `level_gen.cpp:19` and `tile_info.cpp:67`, with stale potion texts ("+25 HP",
  the potions now heal a percentage); the level_gen potion `WEIGHTS` sized by hand; GL init / GLUT main 3 times; the
  viewer's texture fallback and projections copy the game's.
* **Python:** `ascii2level.py` copies the level size, all tile / gate / monster / lock numbers and the checker's glyph
  table; the glyph round trip is lossy (`m`, `Q` have no import; `k` means both giant scarab and bad-colour key; every
  `O` is pair 1). `mechanism.py` lock names must match `LOCK_COLOUR_NAMES`; `make_icons.py` file names must match
  `tile_info.cpp`.
* **Movement rules:** the checker's `Walker` differs from the game in 14 ways, see [Walker](#walker-vs-game).
* **Inside the game:** decor seed hash (5), portal quad and scroll (2), `rock` lambda (2), view-window origin
  `mapX-4/-3`, `mapY-3` (5 files, 7 sites), blood / `TakeHit` and the `drawBlood` lambda (monster vs player), particle
  reset (4), shader compile and link (lighting vs ink), **new:** `0.1f * range` weapon reach (3), the model half-width
  formula (3), the locked-gate hint throttle (2 in `bumpGate`).
* **New:** the player's width is defined twice: movement uses `scale/60` and `scale/40`, combat the model-based
  `Player::HalfWidth`.

### Long functions (over ~75 lines)

| Function | Location | Lines |
|---|---|---:|
| `parseLine` (30-branch if-chain) | `scenario.cpp:401` | 174 |
| `Assets::Load` | `assets.cpp:198` | 168 |
| `checkLevel` | `level_check.cpp:359` | 166 |
| `runInstant` | `scenario.cpp:663` | 124 |
| `Dungeon::Draw` | `dungeon_render.cpp:151` | 124 |
| `PathScript` (class) | `levelcheck.cpp:31` | 123 |
| `Draw()` | `draw.cpp:200` | 101 |
| `SunBeam::draw` | `fire.cpp:218` | 95 |
| `DraftMap::Draw` | `map_view.cpp:270` | 86 |
| levelcheck `main` | `levelcheck.cpp:188` | 83 |
| `drawIcon` | `menu.cpp:169` | 81 |
| viewer `main`, `renderLevel`, `MainMenu::DrawOptions`, `Scenario::load`, `symbol` (map) | | 76-79 |

### Resources and memory

* `Texture` copyable, no destructor; `Font` copyable with a destructor (double delete risk); display lists, fire
  sprite, shader programs, ink FBO never freed. No VBOs.
* `font.cpp` `vsprintf`: **fixed** (`vsnprintf`).
* Fixed `char[]` buffers with `snprintf`; `SaveName{char name[25]}` with `strncpy`. **New bug:** `Game().Load()`
  reads `f >> saveName.name` into the 25-byte buffer with no width limit (`game_state.cpp:58`); fixed on its own, see
  [the plan](code-structure-review.draft.md#decided).

### Save format

* Ad-hoc `ofstream <<` in `Dungeon`, `Inventory`, `PlayerStats`; the slot list separately in `menu.cpp`.
* Only the inventory has a version tag (`INV2`); the legacy path guesses by content. `PlayerStats` treats stamina as
  optional; `Dungeon::LoadDump` guesses by read failure twice.
* **New:** quick-drink state and `Dungeon::levelKeys` are not saved (`levelKeys` is rebuilt from the keys held).

### Magic numbers

`glTranslatef(20,0,-25)`, `glTranslatef(40,0,0)`, `40*x-20`, `-30` instead of `TILE_SIZE` (`dungeon_render`,
`monster`); `draw.cpp:227-289` (camera, lights, offsets); `ink.cpp:192 720.f`; camera yaw `-110/70`;
`player->scale = 15`; `exit(666)`. Named since the first audit: `SCENE_NEAR/FAR`, `PLAYER_CLIMB_*`, `WINDUP_*`.

## Walker vs game

The checker's `Walker` (`level_check.cpp:37-138`) against `Dungeon::Move` and the mechanisms. Decided and done in
[stage 10](solved/movement-model.md): 1, 2, 3, 4, 6, 7, 8, 9, 14 fixed; 5, 10, 12, 13 kept with a reason.

1. Jumps only over a gap; the game can hop spikes and death cells that have a floor.
2. Never jumps from a ladder; the game does.
3. A dead `jump` branch in `Dungeon::Move` (`dungeon_base.cpp:138`): no caller passes `jump=true`.
4. Jump height documented as ~0.45 tiles, real peak 0.405; horizontal reach (~0.97 tiles) not modelled.
5. Jumps cost no stamina (the game: 20 per jump), so a jump chain may be unplayable.
6. Opens a keyed gate on touch; the game needs an approach of 0.45 tiles and `GATE_OPEN_MS` (the script waits a
   hard-coded 1400 ms).
7. A gate with value 2 in the file: the game opens it on load, the Walker treats it as closed.
8. A pulled lever (value 1) never opens anything in the game; the Walker still pulls it.
9. A key of colour 5 (boss lock) opens boss gates in the Walker, not in the game (the checker flags it anyway).
10. Reaching the boss's spawn cell counts as the kill.
11. Teleport: equivalent (the only shared movement constant is `TELEPORT_ARRIVAL_X`).
12. Traps: the path cell only; the game checks a 3 x 3 neighbourhood with a damage ramp.
13. Rocks: a cost only; the game has timing and position-dependent damage.
14. Whole cells; the game climbs from anywhere in the ladder cell and pulls to the grip.

## Tile descriptions

Where tile types are described today (input for [stage 3](code-structure-review.draft.md#stages)):

| Place | Per-type data |
|---|---|
| `src/world/level.h:14-96`, `level.cpp` | type ids, gate sub-types, monster ids, lock colours and names, `BOSS_LOCK`, `isSolidTile`, `isTeleporter`, `isBossMonster`, teleporter pairing |
| `tools/editor/tile_info.{h,cpp}` | name, icon file, description, attr/value choice lists, which fields a type uses, `TILE_COUNT` |
| `tools/editor/editor.cpp:493-503`, `icons/make_icons.py`, `readme.md:46-120` | map swatch, icon per type, docs tables |
| `src/world/level_check.{h,cpp}` | solid / standable / touch, hazard costs, goal, counts, lock / boss / gate / teleporter rules, threat, glyphs |
| `tools/level/levelcheck.cpp:40-86, 166-176` | scenario commands per move, report fields |
| `src/world/level_gen.cpp` | what it places |
| `tools/level/ascii2level.py:11-28` | glyph to (type, attr, value) |
| `dungeon_render.cpp`, `dungeon_io.cpp`, `dungeon_mechanisms.cpp`, `dungeon_monsters.cpp`, `dungeon_decor.cpp` | draw, interact, mechanisms, spawn, decor skip lists |
| `src/ui/map_view.cpp:97, 157-222` | draft map symbol (boss gate and teleporter fall back to graphite) |
| `assets.cpp:168-189`, `tools/blender/models/mechanism.py` | colour-named textures |
| `docs/levels.md`, `docs/testing.md` | rules, legend, warnings |

Worked examples (every place touched):

* **Teleporter:** `level.h`, `level.cpp`, `dungeon_io.cpp`, `dungeon_render.cpp`, `assets.{h,cpp}`,
  `mechanism_sounds.py`, `level_check.{h,cpp}` (5 functions), `levelcheck.cpp`, `tile_info.cpp`, editor readme,
  `ascii2level.py`, `docs/levels.md`, a test level and scenario. Missing: level_gen, an editor icon variant, a map
  symbol.
* **Boss gate:** `level.h`, `dungeon_mechanisms.cpp`, `dungeon_monsters.cpp`, `assets.{h,cpp}`, `mechanism.py`,
  `level_check.cpp` (6 sites), `level_check.h`, `tile_info.cpp`, editor readme, `ascii2level.py`, `docs/levels.md`.
  Missing: a map symbol, a gem name in `LOCK_GEM_NAMES`.
* **Boss monster:** `level.h`, `assets.cpp`, `monster.{h,cpp}`, `dungeon_monsters.cpp`, `scenario.cpp`,
  `scarab.py`, `level_check.cpp`, `levelcheck.cpp`, `tile_info.cpp`, editor readme, `ascii2level.py`,
  `docs/levels.md`, test levels.

## What we don't have

* A split between simulation and rendering; the sim cannot run without GL.
* A fixed timestep with dt; movement depends on key repeat.
* An event list: gameplay calls sound, `ShowStatus`, `AddXP` and the UI directly.
* A per-tile-type definition table.
* A screen stack or FSM, and a shared screen base / canvas helper.
* A player entity that owns its position and physics.
* One movement model shared by the game and the `Walker`.
* A gameplay RNG owned by the world.
* A shared view-window helper.
* Unit tests for pure logic.
* Save format versioning.
* A teleporter lookup index (`teleportPartner` and `openGates` scan the whole level; the checker calls it per
  expansion).
* Layered build: **done** in stage 1 for the two libraries; the rest of `src/` is still one tangle.
