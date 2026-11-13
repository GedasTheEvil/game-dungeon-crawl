# The attack, the walk step and the tick order in one place for the game and SimWorld

Status: draft 2026-11-13, size M, needs nothing first.

## Problem

The unit tests and the balance sim (`tools/balance/balance.cpp:18`) drive the player through `SimWorld`
(`tests/unit/sim_world.h`, used by 79 test files: `grep -rln sim_world.h tests/unit tools | wc -l`). For the player's
side it does not call the game's code: it carries hand copies, each with a comment naming the original.

| SimWorld (`tests/unit/sim_world.h`) | The game's original |
|---|---|
| `Tick` (76): the update order | `tick` (`src/state/game_loop.cpp:80`) |
| `Attack` (187), `HoldAttack` (204), `swing` (323) | `tryAttack`, `tryCharge`, `releaseCharge`, `attackReady`, `swing` (`src/input/input.cpp:172-251`) |
| `updateAttack` (332) | `updateAttack` (`src/state/game_loop.cpp:23`) |
| `walk` (303), `moveMultiplier` (299) | `walk` (`input.cpp:158`), `execute` (`input.cpp:43`) |
| `bagUser` (312) | `bagUser` (`src/ui/inventory.cpp:75`) |
| `Wear` (169), `WearRing` (177) | `Inventory::Wear`, `WearRing` (`src/ui/inventory.cpp:656`, `669`) |

They have drifted already:

* **Palsy:** the game refuses a ranged attack while the Palsy curse runs (`input.cpp:176`: "Your hands shake too much
  to aim"). `SimWorld::Attack` has no such check: a test or a balance fight with Palsy shoots. The rule has no test.
* **The attack timer:** the game asks `attackTimer.TimePassed(true)` (`input.cpp:229`, does not restart the timer);
  `SimWorld::Attack` asks `TimePassed()` (`sim_world.h:188`), which restarts it, so a shot refused by
  `ShotRefused` costs the test a full attack interval the game does not charge.
* **The shot's height:** the game shoots from the drawn fist (`Game().playerView.Fist(...)[1]`,
  `game_loop.cpp:43`, from `graphics/player_view`); SimWorld from `0.65f * player.Height()` (`sim_world.h:342`).
  The game's sim result depends on a graphics class.

Every new rule on the attack or the walk has to be written twice, and a test that passes proves the copy.

## Plan

Move the player's controls into the sim library, as plan scenarios-to-unit-tests did for `Dungeon::StartJump` and
`Dungeon::Interact`; the game and SimWorld then call the same code.

1. `src/sim/player_control.{h,cpp}` (no `Game()`, no sound): given the `Dungeon`, the `Player`, the `ItemBag` and the
   `WorldEvents`:
   * `Attack(facing)` and the power-hit pair (`StartCharge`, `ReleaseCharge`): today's `tryAttack` / `tryCharge` /
     `releaseCharge` / `attackReady` / `swing`, with the Palsy and `ShotRefused` checks and `TimePassed(true)`.
     They return what began (`None`, `Swing`, `ChargedSwing`, `Charge`) so the input plays the sound.
   * `UpdateAttack(facing, handHeight)`: today's `updateAttack`; returns whether a hit landed or a shot left (the
     strike sound). `handHeight` in tiles is passed in: the game gives the fist (`PlayerView::Fist`), SimWorld its
     0.65 of the height. Keep that one difference, written down in the header.
   * `Step(GameplayAction)` / `Walk(dx, dy)`: the sprint multiplier and `NoteWalked`.
   * `bagUser(player)` next to `BagUser` (`src/level/item_bag.h`), used by `Inventory` and SimWorld.
2. `input.cpp` keeps the key mapping, the `held` turn, `insane()` and the sounds; it calls `PlayerControl`.
   `game_loop.cpp` calls `UpdateAttack`.
3. `SimWorld` calls the same functions; its copies go. Add a test: with Palsy, `world.Attack()` with a bow equipped is
   refused and spends no shot.
4. Optional, if it stays small: one `PlayerControl::AfterUpdate(events, safe)` for the stamina, poison, regeneration
   and animation calls that `tick` and `SimWorld::Tick` both make in the same order.

## Risk

Low to medium: the game's attack must behave the same. Scenarios with attacks (`make test`) and `make unit` before
and after; `bin/balance table` output before and after should differ only where the timer and Palsy fixes reach.

## Verify

* `make check`; `make test` (scenario screenshots of fights unchanged).
* `grep -n "input.cpp\|game_loop.cpp" tests/unit/sim_world.h`: no "copy of" comments left for the attack and walk.

## Open questions

* `Wear` / `WearRing` / `bagUser` overlap [item-actions-out-of-screens](item-actions-out-of-screens.tooling.draft.opus.md):
  whichever lands first takes them; the other drops them.
