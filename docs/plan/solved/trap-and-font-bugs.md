# Trap and font bugs

Status: found in the [code structure audit](code-structure-review-audit.md) (2026-09-30). Fixed 2026-09-30 (see
[Implementation](#implementation)); verified in play 2026-09-30. Small and
independent of the [code structure review](code-structure-review.md); can be fixed any time.

## 1. Trap damage depends on drawing, and one hurt timer is shared per trap kind

* `Dungeon::DrawTrapTile` (`src/world/dungeon_render.cpp:117`) takes the single shared `Trap` of that kind
  (`Game().assets.traps.spikes` / `deathTrap`), points it at the tile (`dungeonCamX/Y`, `setCords`) and calls `Show()`.
* `Trap::Show()` draws the model and then calls `Hurt()` (`src/entities/trap.cpp:31`). So:
  * Damage only happens when the trap is drawn. A trap outside the drawn window, or any frame where `Draw` is
    skipped (UI screens, scenario frames), deals no damage. Damage speed follows the frame rate, not the game tick.
  * `Hurt_timer` is a member of the shared `Trap`, so every tile of one kind uses the same timer. Two spike tiles
    drawn in one frame: the first one to call `Hurt()` consumes the timer. Which tile hurts depends on draw order.
  * `dungeonCamX/Y` are raw pointers, uninitialised in the constructor (`trap.cpp:13-18`).
* `gHitStreak` / `gLastHitMs` (`trap.cpp:9-10`) are shared on purpose (the streak belongs to the player), so they
  can stay.

**Fix idea:** move trap damage into `Dungeon::Update()`. Check the player's cell (and the neighbours the hitbox
reaches) for trap tiles and apply the damage there, with one hurt timer per player (or per trap cell). `Trap` then
only draws. Initialise or remove the camera pointers.

**Test:** a scenario where the player stands on spikes: damage ticks at the expected rate, also with two adjacent
spike tiles, and also while a UI screen is open (if the world should pause then, no damage).

## 2. Font buffer overflow

* `Font::print` (`src/graphics/font.cpp:56-65`) formats into `char text[256]` with `vsprintf`, which has no bound.
  A formatted string of 256+ chars (long level name, riddle text, status message) writes past the stack buffer.
* **Fix:** `vsnprintf(text, sizeof(text), fmt, ap)`, or format into a `std::string` sized by a first `vsnprintf`
  call if long strings must print in full.
* Check other `vsprintf` / `sprintf` calls in `src/` and `tools/` in the same pass (`grep -rn 'v\?sprintf('`).

## Implementation

Done 2026-09-30:

* **Traps.** `Trap` only draws (no timer, no camera pointers, no tile coordinates). `Dungeon::updateTraps`, called
  from `Dungeon::Update`, checks the trap tiles round the player's cell (3 x 3: the death trap's hitbox reaches into
  the next cells) with the same hitbox as before (`TRAP_HITBOX_X/Y_SCALE` x the trap's scale, round the player's point).
  One hurt timer and damage streak for the player (`Dungeon::trapHurt`), shared by every trap: two traps never hurt
  twice in one interval. Paused with the world (no damage on UI screens).
* **Font.** `Font::print` uses `vsnprintf` (cut at 255 chars). It was the only `vsprintf` / `sprintf` in `src/` and
  `tools/`.
* **Test.** `tests/scenarios/spikes.txt` (`tests/levels/spikes_one`, `spikes_two`): the hitbox edge, the damage ramp,
  no damage with the inventory open, two tiles side by side. The old code passes it too: in the scenarios every
  tick draws and a trap in reach is always in view, so the old bugs only showed at a low frame rate or with draw
  order. The test pins the behaviour.

Not done: the traps still use the player's point, not the player's hitbox (`Player::HalfWidth`,
[monster hitboxes](monster-hitboxes.md)). A box would widen every trap by 0.06 tiles; left out so as not to change
the jump timings over spikes.
