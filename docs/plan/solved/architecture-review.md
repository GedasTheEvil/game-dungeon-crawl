# Architecture review

Status: done 2026-10-07 (review only, no code change). Report below; the refactors got their own drafts.

## Idea

The game has grown a lot (bosses, weapons, attack timers, amulets, journal, riddles, many UI screens). Look over the
current architecture as a whole: does the split still make sense, or does it need a refactor?

## Snapshot (2026-10-07)

| Dir | Files | Lines |
|---|---|---|
| `src/world/` | 44 | ~6000 |
| `src/ui/` | 30 | ~5700 |
| `src/graphics/` | 25 | ~2400 |
| `src/entities/` | 14 | ~1900 |
| `src/state/` | 12 | ~1600 |
| `src/test/` | 2 | ~1200 |
| `src/input/` | 5 | ~960 |
| `src/core/` | 8 | ~700 |

Largest files: `test/scenario.cpp` (~1180), `ui/journal_view.cpp` (~1070), `ui/menu.cpp` (~950),
`ui/inventory.cpp` (~900), `state/assets.cpp` (~650).

## Questions to answer

* Module boundaries: is each dir one concern? `world/` is the biggest: dungeon, items, loot, monsters, level gen and
  the checker all live there. Split it?
* Dependencies: which way do they point? Does `world/` know about `ui/` or `graphics/`? Draw the include graph.
* Large files: split `journal_view`, `menu`, `inventory`, `scenario` by sub-screen or command?
* Data in code vs data files: split off to [data-tables.draft.md](../data-tables.draft.md).
* Per-kind `switch`es spread over many files: adding one monster or item kind touches how many places?
* Game state: one owner, or spread over globals/singletons?
* Tests: what is unit-testable without a window? What only runs as a scenario?

## Output

A short report: what holds up, what hurts, a ranked list of refactors (cost vs gain). Each worthwhile refactor gets
its own draft plan. No refactor without a clear gain for upcoming work.

## Report (2026-10-07)

Earlier stages already did the heavy lifting ([code-structure-review](code-structure-review.md),
[layered-build](layered-build.md), [world-without-game](world-without-game.md), [sim-render-split](sim-render-split.md),
[dungeon-split](dungeon-split.md)). No big refactor is due; a few targeted ones pay off for the queued content.

### What holds up

* Layers are enforced, not just hoped for: `liblevel.a` (no GL, SDL, `Game()`), `librender.a` (no SDL, `Game()`),
  `check_sim.sh` keeps `Game()`, `game_state.h`, `ui/` and `input/` out of `src/world/dungeon*` and `src/entities/`
  (`make layers`). `Game()` count: ui 140, input 103, test 81, graphics 58, state 44, core 20, world 0, entities 0.
* Dependencies point the right way for the sim: world and entities know graphics and `state/assets.h`, never ui or
  input. Dir-level include graph: ui -> graphics/state/world/core/input/entities; state -> everything below;
  world <-> entities (Dungeon owns monsters, entities use world rules). No cycle through ui.
* Game state: one owner, `GameState` behind `Game()`. Other mutable globals are GL/audio/process singletons
  (Lighting, Ink, MotionFx, Fire sprite, Sound, Logger, the virtual clock, the scenario runner, three input statics):
  acceptable, as the review decided.
* Rules already unit-tested (18 files, ~80 cases): damage, poison, items, item bag, loot, progression, journal, level,
  level checker, walker, movement, tiles, settings, bindings, view window, world events, docs sync.

### What hurts

1. **Monster and item data scatter** (the main cost of new content). A plain monster variant: 4 places in 3 files,
   6 with poison; one with new behaviour: the cobra touched 18 files. Monster rows are split over `MONSTER_DEFS`,
   `RESISTANCE_DEFS`, `WADING_DEFS`, `POISON_DEFS`, `SPIT_DEFS`, `ATTACK_MIX_DEFS` (`state/assets.cpp`), `KINDS`
   (`world/monster_kinds.cpp`) and `PICKS` (`world/level_gen.cpp`); `isPoisoner()` repeats `POISON_DEFS` in code;
   Sobek's charge (`assets.cpp:479`) and the egg cluster (`dungeon_boss.cpp:180`) are hard-coded type checks. A
   weapon: 9-11 places; a potion: ~11, with per-kind `switch`es (`potionGain`, `potionNote`, `weaponGrowthPercent`,
   `missileOf`) and implicit orders (save slots, HUD icon index, the mimic loot range ending at `Antidote`, hand-typed
   kind counts). Balance stats sit in `assets.cpp` (the file with the most churn: 57 commits in 60 days) next to model
   loading, so they are not unit-testable. Covered by [data-tables](../data-tables.draft.md) (its scatter list is now
   corrected).
2. **Potion effects live in the inventory screen.** `Inventory::DrinkPotion` / `QuickDrink` (`ui/inventory.cpp:201-315`)
   apply gains to `PlayerStats`, play sounds and build the toast. Amulets and the resistance potion would land there
   too. Draft: [item-effects-out-of-ui](item-effects-out-of-ui.md).
3. **Drawing is still mixed into the sim files.** `draw*` functions in `dungeon_decor.cpp` (scatter rules + GL),
   `dungeon_mechanisms.cpp`, `dungeon_boss.cpp`, `dungeon_arrows.cpp`, `dungeon_monsters.cpp`; entities include GL.
   Known leftover ("its own stage", world-without-game). Effect: monster AI, player stats, decor scatter and mechanism
   rules only run as scenarios (97 scenarios, ~7-8 min). Draft: [sim-unit-tests](../sim-unit-tests.draft.md).
4. **Scenario commands in three places.** Each new command touches `parseLine`, `runInstant` and often `fieldValue`
   in `test/scenario.cpp` (1207 lines, 44 commits in 60 days); the parser has no `Game()` but is not tested. Draft:
   [scenario-split](../scenario-split.draft.md).

### Not worth it now

* Splitting `src/world/` into dirs: the libraries and `make layers` already draw the line; moving files is churn.
* `graphics/draw.cpp` is the frame composer (includes ui, game state, scenario): an app-layer file in `graphics/`.
  Moving it to `state/` is cosmetic; do it in passing if the file is touched for other reasons.
* `ui/menu.cpp`: the options + controls sub-screen (~350 lines) is a clean seam, but menu work is not queued. Split it
  when [inventory-keys](../inventory-keys.draft.md) or a new options page touches it.
* `ui/journal_view.cpp`: the note text tables (`FIELD_NOTES`, `moveNote`) could move next to `world/journal`; small
  gain, do it with the next journal change.
* `level_check.cpp`, `input/input.cpp`: one concern each, a split would only move lines.
* Test hooks in game code (`Scenario::godMode()` in `Player::TakeHit`, `onFrameRendered` in `draw.cpp`): small, known.

### Ranked refactors

| # | Refactor | Cost | Gain for queued work |
|---|---|---|---|
| 1 | [item-effects-out-of-ui](item-effects-out-of-ui.md) | small | amulets, resistance potion land in the rules (done: in `PlayerStats`) |
| 2 | [data-tables](../data-tables.draft.md): one row per monster / item kind | medium | every new monster, boss, weapon, potion, amulet; monster balance tuning |
| 3 | [sim-unit-tests](../sim-unit-tests.draft.md): clock into the level lib, then rules out of the GL files | small first step, medium after | player stats, AI, scatter tested without a window; faster than scenarios |
| 4 | [scenario-split](../scenario-split.draft.md) | small-medium | every feature adds scenario commands |

Order: 1 before amulets. 2 when the user confirms (its timing rule), best before more bosses. 3's first step (the clock,
`PlayerStats` tests) is cheap and can go any time.
