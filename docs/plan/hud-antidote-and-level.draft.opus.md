# HUD: antidote slot and character level

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: HUD layout ([hud-redesign](solved/hud-redesign.md)).

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
