# Stage 4: split simulation from rendering

Status: implemented 2026-09-30, not yet reviewed by the user. Stage 4 of the
[code structure review](code-structure-review.draft.md).

## Why

`Draw` changed game state, so what happened depended on what was drawn and how often:

* `Dungeon::Draw` spawned the monsters in view (and with that started the boss fight and its minions), turned the
  portal plasma, the riddle mark and the treasure items, and set the shared club model's scale to the chest's 10 for
  good: once a club chest had been drawn, the club in the player's hand was drawn at the chest's size.
* `Monster::Draw` and `Player::Draw` switched the death / revival pose, ran the blood particles and advanced the
  clip frame. A dead monster that is not drawn never finishes its death clip, and the mimic's chest waits for it.
* The HUD's lost-health trail was file-static state updated inside `PlayerHud::draw`, never reset.
* `Update()` (the game tick) and the attack's hit resolution lived in `graphics/draw.cpp`, declared in `input.h`.

## Done

* `state/game_loop.{h,cpp}`: `Update()` and `updateAttack` moved out of `graphics/draw.cpp`; `graphics/draw.h`
  declares `Draw()`. `input.h` lost the three declarations (one, `Load`, had no definition).
* `Dungeon::Update` ends with `spawnInView` (the same 10 x 6 window the draw loop walks) and `updateAnimations`
  (portal scroll, riddle mark, `treasureSpin`). `DrawTreasureTile` borrows the item's angle and size and puts them
  back (`CHEST_CLUB_SCALE`).
* `Monster::Animate` / `Player::Animate`, called at the end of `Update()`, do the pose, blood and frame work;
  `Draw` only draws. `Player::shownFrame` (the weapon's fist) is set for the frame that Draw will show.
  `Dungeon::inView` is the one culling test for both. `PlayerHud::tick` (from `Update()`) and `PlayerHud::reset`
  (new game, loaded game).
* Order within a scenario tick is unchanged (commands, `Update`, `Draw`), so the random sequence is too: every
  scenario passes with its seeded expectations.

Screenshots against the build before: 59 of 290 differ, all where an animation shows. The clip frames are one tick
ahead (they now advance before the frame is drawn, not after), the treasure and riddle marks turn at one step per
tick instead of one per drawn chest, and the club in the player's hand has its own size again after a club chest
(`props/012_held_club`).

## Left for later

* Only the monsters in view animate: their blood still draws from the shared `rand()`, so animating the others
  would change every seeded roll after it. Stage 9 gives the particles their own random stream; then all active
  monsters should animate, so an unseen monster finishes dying.
* The UI screens (inventory turntable, menu) still advance their own animations in their `Draw`; they are
  presentation, not game state.
* `drawWeapon` still borrows the held weapon's shared `rotA` (and puts it back).
