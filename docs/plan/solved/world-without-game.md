# World without `Game()` (stages 6 and 7)

Status: draft 2026-10-01. Done 2026-10-01 (see [Done](#done)). Stages 6 and 7 of
[code-structure-review.md](code-structure-review.md).

## Why

`src/world` (the `Dungeon`) and `src/entities` (monsters, the player) call `Game()` in about 115 places: the player,
the assets, the journal, the random streams, the level number, the win flag, the inventory, the riddle screen, the
status line and the sounds. So the simulation cannot run, or be read, without the whole app: every function may
touch any part of the game. Decided in the review: `Game()` goes out of the world and the entities only; it stays the
app root for `main`, input and the screens. Audio and the log may stay global services.

## Design

* **What the world owns:** the level number and the win flag move from `GameState` into `Dungeon`
  (`LevelNumber()`, `Won()`). They are world state; the app reads them.
* **What the world is given** (`SimLinks`, `src/world/sim_links.h`): the player, the journal, the item bag (the
  inventory's model, not its screen), the random streams, the assets and the event list. `GameState` links them
  into the `Dungeon` once, after loading. The monsters get the links from the `Dungeon` as a parameter.
* **What the world tells the app** (stage 7, `WorldEvents`, `src/world/world_events.h`): a list of events the
  app drains: play a sound, show a status line, write a field note, ask the riddle. The app applies them at fixed
  points: after the input actions and at the end of `Update()`. The world no longer reaches into the UI or the sound
  bank.
* The player and its stats write their events (status, field notes) into a `WorldEvents&` parameter; they hold no
  link.
* A check keeps it so: `make layers` fails on `Game()` or `game_state.h` in `src/world` and `src/entities`.

## Steps

1. Level number and win flag into `Dungeon`.
2. `WorldEvents`, drained by the app; sounds, status lines, field notes, the riddle through it.
3. `SimLinks` for the rest of the `Dungeon` and the monsters; the player's and its stats' events by parameter.
4. The layer check.

Each step keeps `make`, `make tidy`, the unit and scenario tests and `./levelcheck levels/lvl*` green.

## Not in this plan

* Unit tests of the `Dungeon` itself: its draw code lives in the same class and needs GL and the asset bank. Taking
  the renderer out of `Dungeon` would be its own stage.
* `Scenario::godMode()` in `Player::TakeHit` (a test switch, not `Game()`).

## Done

* Steps 1-4 as planned. `make layers` runs `tools/check_sim.sh` over `src/world/dungeon*`, `sim_links.h` and
  `src/entities/`.
* Kill rewards (journal kill, XP, the weapon drop) and the journal's damage-type note moved from `Monster` into
  `Dungeon::playerHit` / `rewardKill`. `Monster::takeHit` returns whether that hit killed it; a trap's kill is never
  rewarded because the trap code does not call `rewardKill`.
* A monster keeps its `MonsterLinks` (player, journal, events, level) from its spawn; a level load clears the
  monsters, so the level number cannot go stale.
* The player and its stats take a `WorldEvents&` (`TakeHit`, `AddXP`, `UpdateStamina`, `RefuseStamina`); their draw
  code takes the `TextureRegistry`.
* `ItemBag::Find` adds a found item and writes the weapons note, for the world and the inventory screen alike.
* Drain points: `GameState::ApplyWorldEvents` after each input action, at the end of `Update()` (every screen), and
  after each scenario command (an `expect` on the next line sees the result).
* `GameRandom` moved to `src/world/rng.h`. Unit tests: `tests/unit/world_events_test.cpp`.

