# HUD redesign: player health, stamina and quick slots

Status: idea, not started. Before [quick-potions.draft.md](quick-potions.draft.md), which puts its slots in this HUD.

## Why

The boss health bar (`src/ui/boss_bar.cpp`) and the other UI screens share one look ([../ui.md](../ui.md)): dark
stone panels, gold frames, gold diamonds, gradient fills, the status font. The player's own bars do not. They are
flat quads with a white line outline (`Hud::drawPlayerBars`, `src/graphics/hud.cpp`), bottom left, green health and
yellow stamina, no numbers. The held keys (`Hud::drawKeys`) are flat coloured shapes next to them. It looks like
a placeholder next to the level gem and the boss bar.

## Goal

One HUD panel, bottom left, in the UI look. It holds everything about the player at a glance:

```
 ◆━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━◆
 ┃ ♥ ███████████████████░░░░░   96 / 134     ┃
 ┃ ⚡ ██████████████░░░░░░░░░                 ┃
 ┃ [sword] [heal 3] [vigor 1]   ◆ ◆ ◆ ◆ keys ┃
 ┃   1-4     Q         R                     ┃
 ◆━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━◆
```

* **Health bar:** blood red gradient like the boss bar, gold frame, end diamonds. `HP / max` in the status font.
  A darker "lost" part that trails behind for ~0.5 s after a hit (the classic delayed damage bar), so a hit reads
  even in a fight.
* **Stamina bar:** thinner, amber / gold gradient, under the health bar. It flashes when a jump or sprint is refused
  for lack of stamina.
* **Low health:** under 25% the health bar pulses (like the inventory's selected slot halo).
* **Quick slots** (the icons [quick-potions.draft.md](quick-potions.draft.md) needs): the weapon in hand, the healing
  potion the hotkey would drink, the stamina potion the hotkey would drink. Each is a small stone tile
  (`ui::tile`, `TileStyle::Stone`) with the item icon, a count badge for potions (like the inventory slots) and the
  key cap under it. Greyed out when there is nothing to drink.
* **Keys:** the four lock gems in their colours (the gems of `LevelGem` / the inventory), dim sockets for the ones
  not found yet on this level, so the player sees how many locks there are... or only the found ones, as today (open
  question).
* **Level up / XP:** a thin XP line along the panel's bottom edge, the level gem stays top right.

## Code pointers

* Draw: `src/graphics/draw.cpp` (HUD block after the scene: `Hud::drawPlayerBars`, `Hud::drawKeys`, `LevelGem::draw`,
  `BossBar::draw`, `StatusBox::draw`).
* Style: `src/ui/ui_draw.h` (`panel`, `fillRect`, `strokeRect`, `diamond`, `tile`, `text`, colours), the canvas set up
  like `BossBar::draw` / `StatusBox::draw` (100 high, square pixels).
* A new `src/ui/player_hud.{h,cpp}` (`PlayerHud::draw(...)`) replaces `src/graphics/hud.cpp`. It takes plain values
  (ratios, counts, the icon choice), so a scenario can screenshot any state.
* Item icons: the inventory draws each slot's 3D model on a turntable (`Inventory::DrawSlotModel`). For the HUD,
  either render those models small (still, no turntable) or bake 2D icons once (a Blender render per item, like
  `tools/blender/render_sheet.py`) into a HUD atlas. 2D icons are cheaper and crisper at this size.

## Tests

* Scenario screenshots: full health, half, low (pulse), after a hit (delayed bar), empty stamina, keys held, each
  quick slot state (weapon, potion counts, none left). Also at 1280x720 and a small and a wide `resolution`.
* The existing HUD scenarios (`status_box.txt`, `level_gem.txt`, `stamina.txt`, `keys.txt`) still pass; check their
  screenshots for overlap with the new panel.

## Open questions

* Keys: show empty sockets for the lock colours the level has, or only the keys held?
* Numbers on the stamina bar too, or health only?
* Should the panel fade to half alpha when nothing changes for a while, or stay solid?
