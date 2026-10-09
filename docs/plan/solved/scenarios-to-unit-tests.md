# Scenarios to unit tests

Status: implemented 2026-10-09 (tooling only, so solved without a play test; see [Progress](#progress)). From the
user: scenarios that don't need a screenshot move to unit tests. Expected result: a faster test run, fewer resources.

## Today

* 106 scenarios in `tests/scenarios/` ([../../testing.md](../../testing.md)). Each starts the game under Xvfb, so
  `make test` is the slow part (~7-8 min for 97 at the time of [sim-unit-tests](sim-unit-tests.md)) and
  loads the machine (`JOBS`, `RESERVE_CORES`, `MIN_FREE_MB`, `MIN_SWAP_FREE_MB`).
* 19 doctest files in `tests/unit/` link `liblevel.a` and `libbase.a`: no window, no GL, fast.
* 10 scenarios take no screenshot. Many others take one or two only as a debugging aid, and check the behaviour with
  `expect` lines (e.g. `weapon_hotkeys.txt`: 2 screenshots, 22 expects; `amulets.txt`: 3 and 27).

## Idea

* Sort each scenario into one of three groups:
  1. **Behaviour only:** no screenshot, or screenshots nobody needs to look at; all checks are `expect` lines on
     rules in the library (stats, items, damage, loot, water, progression). Move to a unit test, delete the scenario.
  2. **Behaviour over code still in the game binary:** monster AI, missiles, mechanisms, boss. Blocked until the rules
     leave the GL files ([sim-library](sim-library.md)). Stay scenarios for now.
  3. **Visual or whole-game:** HUD, screens, model / animation, filtering, aspect, smoke, input path end to end. Stay
     scenarios.
* A moved test checks the same thing as the scenario's `expect` lines, through the library's public interface.
* Some scenarios mix both: split them (the rule to a unit test, the look stays a trimmed scenario).

## Measuring without a screenshot (survey 2026-10-09)

What the screenshots of the 96 scenarios confirm, by eye:

| Class | Count | Scenarios |
|---|---:|---|
| Debug aid only (the `expect` lines check it) | 24 | amulets, weapon_hotkeys, anubis_coffins_load, anubis_boss, monster_poison, damage_types, keys, level_exit_jump, teleport, chest_pickup, reckless_rock, reckless_anubis, reckless_spikes, giant_rat_speed, quick_potions, venom_amulet, dart_trap, boss, lvl5_boss, monster_idle_bars, riddle_scaled, smoke, generated, decor_depth |
| Position / geometry | 17 | monster_hitboxes, weapon_reach, melee_weapons, coward_rock, monster_walls, monster_hazards, monster_follow, water, water_arrow, rock_fall, fall_trap, giant_rat_jump, giant_scarab_jump, gate_facing, render_window, aspect_wide, aspect_tall |
| Animation / pose | 18 | weapons_held (88 shots), bow, ranged_weapons, bats, cobra, mimic, mummy, scorpions, scorpion_queen, sobek, apep, crocodile, vampire, rats, scarabs, monster_anim, ladder, mechanisms |
| Light / colour / particles | 6 | lighting, blood_at_start, monster_blood, summon_effects, sprint_motion, toon |
| HUD / UI | 26 | menu, options, credits, inventory*, player_hud*, journal*, journal_page_turn, status_box, level_gem, level_up, draft_map, screen_tabs, riddle, ... |
| Truly visual | 5 | filtering, surfaces, statues, props, toon_weapon_scale |

The main blocker is that `Dungeon`, `Monster`, `Player` and `character_model` are not in `liblevel`. `dungeon.h`
includes `monster.h`, which pulls in particles, textures and the model. Most of the debug-only class checks Dungeon or
monster rules. The plan to solve it: [sim-library](sim-library.md).

Techniques, by value:

1. **Weapon swing pose (S).** `swingPose(WeaponMotion)` (`src/graphics/draw.cpp:62-87`) is pure apart from `Game()`
   and the clock. Move it to `items.cpp` as `swingPose(m, elapsedMs)`, then assert tilt, thrust and draw at the
   windup, hit, swing and recovery marks for every weapon. Replaces most of weapons_held (88 shots) and parts of bow
   and ranged_weapons.
2. **Water sink (S).** `Dungeon::waterSink` (`src/world/dungeon_base.cpp:211`) needs only the map. Make it a free
   function on `Level`, then assert the depth at the bank edge, mid-ramp and the centre, and 0 on dry floor. Replaces
   water, the crocodile's bank shots and the water_arrow geometry.
3. **Flames and lights over all levels (S-M).** `Dungeon::flamesAt` and `TORCH_FIRE` / `BRAZIER_FIRE`
   (`src/world/dungeon_render_decor.cpp:15, 118`) sit in a render file. Move them out with a plain light id instead of
   `Lighting::LightDef`. Then run a property test over `levels/lvl*`: every torch, brazier and lamp cell gives a flame
   inside its cell, and no torch stands in deep water
   ([no-torches-under-water](../no-torches-under-water.draft.md)). The scatter is already in the lib (`decor_test`).
4. **Hitbox and reach matrix (M).** `Monster::HalfWidth` / `Player::HalfWidth` (`monster.cpp:158`, `player.cpp:58`)
   are model bbox × scale × `Ink::figureScale()`. Toon mode changes the hitboxes, which is worth a check of its own.
   MD3 parsing is GL-free until the display list compiles (`animated_model.cpp:241`). Either move the parser to a
   lib, or store the half-widths in `MonsterKind` with a test against the md3 bbox. Then assert from
   `MONSTER_BITE_REACH` and `Item::Reach()` that every weapon reaches a biting monster of every size. Replaces
   monster_hitboxes, weapon_reach and melee_weapons.
5. **UI layout as data (M).** `ui::visibleArea` / `toCanvas` (`ui_draw.cpp:8`) are pure but live in `librender`.
   `playerHudView()` has no GL. Other UI rules are trapped inside `draw()`: the status box fade, the gem per level
   (`level_gem.cpp:12`), the HUD trail (`player_hud.cpp:317`), the inventory slot and button rects. Pull out
   `layout(resX, resY)` → rects and `alpha(ageMs)`. Assert that at 4:3, 16:9 and 21:9 everything fits the canvas
   without overlap and that click points hit their targets. The page curl maths (`page_curl.h:19-21`) is already pure;
   move it out of its GL file.
6. **Golden dumps (S-M).** A per-level text dump of the scatter and the flames, diffed in a unit test. Replaces
   decor_depth and the placement part of surfaces and props. Particles: `PARTICLE_COUNT` is fixed, so a test checks
   the emitter triggers, which are already `WorldEvents` in the lib.
7. **Fixed-step sim harness (L).** Most of the parts already exist: the virtual clock (`core/timer.h`),
   `UPDATE_TICK_MS`, `GameRandom` with separate gameplay and effects streams (`world/rng.h:31`), `SimLinks`, and
   `Dungeon::Dump` / `LoadDump`. What is missing is Dungeon and the monsters in a lib (the sim split). With that, a
   test loads a level, steps N ticks and checks positions: the rat stops at the rock, the wall and the hazards, the
   jump arc, the rock-fall graze against a crush. Covers the debug-only class and most of the geometry class.
8. **Animation clip state (L, after 7).** `CharacterModel::Enter` / `Advance` / `Progress`
   (`character_model.cpp:83-115`) and `AnimPlayback` (`animated_model.h:22`) need only each clip's frame count and
   loop flag. Split out a GL-free playback core, then assert (state, frame) per tick: idle, rise, attack, die, and a
   non-looping clip holding its last frame. Replaces the animation class.
9. **Draw-list recorder (L, low priority).** A shim over the draw calls (`glTranslatef`, `Fire::draw`,
   `Lighting::add`, `Model::Show`) records (kind, x, y, z, state, frame) for golden diffs. It is heavy, because the
   draw code calls GL directly everywhere. The pure-function splits above come first.

The truly visual class (5) and a smoke screenshot per screen stay scenarios.

## Decided (2026-10-09)

* **Scope:** techniques 1-8. The draw-list recorder (9) is dropped.
* **Order, by size and by what blocks them:**
  1. Without the sim library: 1 swing pose, 2 water sink, 3 flames and lights, 6 golden dumps.
  2. 5 UI layout as data.
  3. After [sim-library](sim-library.md): 4 hitbox and reach matrix (on `ModelInfo`, its step 1), 7 the sim
     harness, 8 clip state.
* One technique per commit. The game plays the same after each one.
* **Moved scenarios:** delete a scenario once all its checks are unit tests. If something visual is left, trim it to
  that, with one smoke screenshot.
* **Measure, no target:** note the `make test` time and the scenario count before and after each batch here.
* Tooling only (no change to the game): once done and checked, straight to `solved/`.

## Progress

* Start (2026-10-09, after [sim-library](sim-library.md)): 106 scenarios, 602 screenshots, `make test` 6:22.
* Batch 1 (techniques 1, 2, 3, 6):
  * 1 swing pose: `swingPose(motion, elapsedMs)` in `world/items.cpp`, `tests/unit/swing_test.cpp` (rest, windup,
    strike, recovery, the bow's draw, the thrust, for every weapon). `weapons_held` keeps the rest and strike shots
    (104 to 52); each shot removed is a `wait 1` (a `screenshot` takes a tick), so the others stay byte-identical.
  * 2 water sink: `Dungeon::WaterSink` public, tested on `tests/levels/water` in `tests/unit/sim_test.cpp` with the
    wading speed and the spikes under water. `water` stays: its sprint and jump checks go through the input.
  * 3 flames: `flamesAt` on the `DecorLayout` (done with sim-library step 5); `decor_test.cpp` checks every fire of
    every campaign level burns inside its cell. The deep-water torch check waits for
    [no-torches-under-water](../no-torches-under-water.draft.md).
  * 6 golden dumps: `tests/unit/golden_test.cpp`, `tests/unit/golden/decor_lvlNN.txt` (surfaces, props, decals,
    torches, ladders, fires); `GOLDEN_UPDATE=1` writes them anew. `decor_depth` deleted: its tiers are checked
    through the dungeon in `sim_test.cpp`.
  * After: 105 scenarios, 546 screenshots, `make test` 6:12.
* Technique 5, UI layout as data: `src/ui/ui_layout.{h,cpp}` (the canvas, `Rect`, `visibleArea`, `toCanvas` and
  `toWindow`, the screen tabs, the status box's lines, box and fade, the level gem, the HUD's damage trail) and
  `src/ui/inventory_layout.{h,cpp}` (panels, slots, tabs, sort and use buttons, `hitAt`) in the level library; the
  page turn's maths in `src/ui/page_turn.{h,cpp}`. `tests/unit/ui_layout_test.cpp`: at 4:3, 16:9, 21:9 and a tall
  window everything fits, nothing overlaps, a click on a part's centre hits it; the fades and the trail. `status_box`
  drops its last (empty) shot. The other HUD and screen scenarios stay: they show the look. After: 105 scenarios, 545
  screenshots.
* Technique 4, hitbox and reach matrix: `tests/unit/reach_test.cpp` on the sim harness: every size of walker (scarab,
  rat, worm, giant scarab, boss scarab, giant rat) stops where it bites and every melee weapon hits it there, in
  normal and toon mode; toon mode widens the boxes; an arrow hits a giant rat behind spikes. `monster_hitboxes`
  deleted; `melee_weapons` merged into `weapon_reach` (the look of each reach against its weapon). After: 103
  scenarios, 538 screenshots, `make test` 6:16.
* Technique 7, the sim harness: `SimWorld` presses the keys as the scenario commands do; `Dungeon::StartJump` and
  `Dungeon::Interact` (pick up, lever, gate) moved out of `input.cpp` into the library for it.
  `tests/unit/monster_rules_test.cpp` and `tests/unit/world_rules_test.cpp` take over the checks of 22 scenarios,
  deleted: reckless_rock, reckless_spikes, reckless_anubis, giant_rat_speed, monster_hazards, giant_rat_jump,
  giant_scarab_jump, monster_poison, damage_types, venom_amulet, monster_idle_bars, teleport, level_exit_jump,
  dart_trap, rock_fall, fall_trap, water_arrow, boss, lvl5_boss, anubis_boss, anubis_coffins_load, generated; and
  coward_rock and monster_walls (in `sim_test.cpp` since sim-library step 6). The numbers came out the same as in
  the scenarios. Stay scenarios: keys, amulets, weapon_hotkeys, quick_potions (the keys and the inventory), water
  (sprint and jump keys), smoke, riddle_scaled, chest_pickup, monster_follow (the drawing). After: 79 scenarios, 473
  screenshots, `make test` 5:01.
  `chest_pickup` followed (the chest's item and bonus roll).
* Technique 8, clip state: `tests/unit/clip_state_test.cpp`: monsters on one model walk out of step (`monster_anim`
  deleted), the mummy's rise plays once from its start, a die clip holds its last frame, the cobra goes coiled, rise,
  spit, the bat roosts until it swoops, the player's jump, climb (one cycle a tile) and die clips. The other
  animation scenarios stay: they show the poses. After: 77 scenarios, 468 screenshots (`make test` 8:38 under an
  outside load, no core idle; 5:01 for 79 the run before).
* Done: 106 scenarios and 602 screenshots to 77 and 468; the unit tests from 145 test cases to 208, in about 6 s.
  `make test` took 6:22 at the start and 5:01 after technique 7 (79 scenarios), with about a quarter less CPU time
  (35.5 to 26.2 minutes of user time).
