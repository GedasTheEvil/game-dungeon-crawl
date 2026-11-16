# Split GameState into smaller parts

Status: draft 2026-11-16, size L, needs nothing first.

## Problem

The architecture review (2026-11-16, smell 13): `GameState` (`state/game_state.h`) includes 34 project headers (10
from `ui/`) and is included by 24 files. It holds the sim (dungeon, player, bag), the drawing (assets, views), the UI
screens, the settings, saves, the sound mapping (`soundOf`, `characterSound`) and the loading screen (`DrawLoad`).
Screens change the sim through `Game()` directly (riddle, inventory, win entry, HUD); the input module mixes key
handling with gameplay commands (`PlayerActionController`, `playerControl()` in `input/input.cpp`, called by the game
loop).

## Plan

* One piece at a time:
  * done 2026-11-16: the sound mapping next to its data (`SoundBank::Of`, `CharacterModel::SoundOf`);
  * done 2026-11-16: `DrawLoad` to `Assets::DrawLoadingScreen` (`graphics/assets.cpp`);
  * done 2026-11-16: `PlayerActionController`, `playerControl()`, the held walk, charge and look steps from input
    to `state/gameplay_commands.*`;
  * done 2026-11-16: saves to a `SaveService` (`state/saves.*`, `Game().saves`): write, read, load into the game,
    autosave, quick save, latest save, the death count; the files' paths and the newest save in `SaveFolder`
    (`level/save_slots.*`, unit-tested), the autosave's death rewrite in `SaveSlots::SetFileDeaths`;
  * done 2026-11-16: the gameplay actions out of the inventory screen to `ItemCommands` (`state/item_commands.*`,
    `Game().itemCommands`): quick drink and antidote, the weapon keys, insanity's club, the scenario's equip, wear and
    add item; the screen's buttons use its `Use` and `Actor` (plan
    [bag-out-of-inventory](solved/bag-out-of-inventory.tooling.md)). A weapon key no longer leaves its line in the
    closed inventory's toast.
* Screens take explicit references (as `Inventory` takes the bag and `ItemCommands`) instead of `Game()`.
* Done 2026-11-16: `UIContext` in `state/ui_context.h`, forward-declared, so `game_state.h` includes no `ui/` header
  (34 project includes before, 25 now).
* Each step: no behaviour change; unit tests and the archived UI scenarios give the same results as before.
