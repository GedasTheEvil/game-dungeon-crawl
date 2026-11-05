# HUD: antidote slot and character level

Status: solved 2026-11-05 (play-tested by the user). Was: implemented 2026-11-05, waiting for the user's play-test. Was: draft 2026-10-30, size M, needs nothing first. From the project audit of 2026-10-30.

Opus: HUD layout ([hud-redesign](hud-redesign.md)).

## Problems

* The antidote has no HUD slot. The `=` key has no key cap and no count. `PlayerHud` has `slots[4]`: weapon,
  healing, stamina, amulet (`src/ui/player_hud.h`). A poisoned player can't see whether they carry an antidote.
* XP is only a thin line (`XP_LINE`, `src/ui/player_hud.cpp`). The character level number shows only in the
  inventory.

## Plan

* A fifth quick slot for the antidote, with its key cap and count, in the same style. It blinks (like the stamina
  refusal) while the player is poisoned and has one. Check the layout at the smallest window size.
* The character level number next to (or on) the XP line; the XP figure on hover or in the inventory as now.
* Screenshot scenario for the new HUD.

## Done

* A fifth quick slot, between the stamina potion and the amulet: the antidote's flask with its count and the
  QuickAntidote key cap (`5` / `=`), empty when none is left. While poisoned with one held, a green ring blinks round
  it; drinking one flashes it gold like the other potions (`Inventory::AntidoteMs`).
* The panel is one slot step wider (`WIDER` in `player_hud.cpp`): the bars, the health numbers, the key sockets, the
  rings and the stamina-skill sign move right with it. The boss bar reads the panel's size, so it keeps clear.
* The character level's number in a small badge at the right end of the XP line. The XP figure stays in the inventory.
* Scenarios `hud_antidote_and_level` (+ `_320x200`, `_800x600`: the smallest window and 4:3).

## Play-test

`bin/game --play tests/play/hud-antidote-and-level.txt`

* The HUD has five slots: club, healing, stamina, antidote (3) with key cap 5, no amulet. The level badge reads 2.
* The antidote slot blinks green while poisoned. Press 5 (or =): the poison ends, the count drops to 2, the blink stops.
