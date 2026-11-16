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
  * the sound mapping to `audio/`;
  * `DrawLoad` to `graphics/`;
  * saves to a `SaveService` (`state/saves.cpp`);
  * `PlayerActionController`, `playerControl()` and the held-walk and charge steps from input to
    `state/gameplay_commands.cpp`;
  * the gameplay actions still in the inventory screen (quick drink, equip, wear, add item; plan
    [bag-out-of-inventory](solved/bag-out-of-inventory.tooling.md)).
* Screens take explicit references (as `Inventory::Actor()` does) instead of `Game()`.
* Forward-declare `UIContext` so `game_state.h` no longer includes `ui/`.
* Each step: no behaviour change; unit tests and the archived UI scenarios give the same results as before.
