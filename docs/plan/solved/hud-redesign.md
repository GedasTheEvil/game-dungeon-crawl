# HUD redesign: player health, stamina and quick slots

Status: implemented 2026-09-30 (see [Implementation](#implementation)); verified in play 2026-09-30. Together with
[quick-potions.md](quick-potions.md), which puts its slots in this HUD.

## Why

The boss health bar (`src/ui/boss_bar.cpp`) and the other UI screens share one look ([../../ui.md](../../ui.md)): dark
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
 ┃   1-4     H         0                     ┃
 ◆━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━◆
```

* **Health bar:** blood red gradient like the boss bar, gold frame, end diamonds. `HP / max` in the status font.
  A darker "lost" part that trails behind for ~0.5 s after a hit (the classic delayed damage bar), so a hit reads
  even in a fight.
* **Stamina bar:** thinner, amber / gold gradient, under the health bar. It flashes when a jump or sprint is refused
  for lack of stamina.
* **Low health:** under 25% the health bar pulses (like the inventory's selected slot halo).
* **Quick slots** (the icons [quick-potions.md](quick-potions.md) needs): the weapon in hand, the healing
  potion the hotkey would drink, the stamina potion the hotkey would drink. Each is a small stone tile
  (`ui::tile`, `TileStyle::Stone`) with the item icon, a count badge for potions (like the inventory slots) and the
  key to press as a key cap under it (`1`-`4`, `H`, `0`, drawn like the options controls table's key caps). No potion
  of that kind left: the slot is empty (no icon, no count), the key cap stays.
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

## Decided

Defaults picked during implementation (2026-09-30); change them after playing if they feel wrong:

* Keys: a dim socket per key the level has (its Key tiles, plus the held ones after a load), the held ones set with
  their gem. A colour opened only by a lever has no socket.
* Numbers on the health bar only.
* The panel stays solid, no fade.

## Implementation

* `src/ui/player_hud.{h,cpp}`: `PlayerHud::draw(View, ...)`. The layout is in panel units on a canvas
  `100 / SCALE` high (`SCALE` 0.85), `PANEL` bottom left. `View` holds plain values; `playerHudView()` in
  `src/graphics/draw.cpp` fills it from the game.
* Health: blood gradient (the boss bar's colours), gold frame and end diamonds, `HP / max` in `fonts.hudBody`.
  The lost part (`DamageTrail`) holds 500 ms after the last hit, then drains. Under 25% an additive red ring pulses.
* Stamina: amber gradient. `PlayerStats::RefuseStamina()` (a jump without enough stamina, sprint held at 0)
  flashes it red for 600 ms.
* Quick slots: stone tiles with the item model drawn small (the inventory's models, not baked icons; weapons lie
  at -40 degrees), a count badge on the potions, key caps `1-4`, `H`, `0` like the options table. A quick drink
  flashes its slot gold for 500 ms.
* 2D icons instead of the models: [hud-icons.md](hud-icons.md).
* Key sockets: `Dungeon::LevelKeys()`. XP: a thin gold line along the panel's bottom edge.
* The boss bar moves up above the panel when they would overlap (narrow windows such as 4:3).
* `src/graphics/hud.cpp` keeps only `Hud::drawBar`, for the model viewer.
* Tests: `tests/scenarios/player_hud.txt` (full, slots, hit trail, half, low, refused jump, key sockets),
  `player_hud_sizes.txt` (4:3 with the boss bar), `player_hud_small.txt` (640 x 360).
