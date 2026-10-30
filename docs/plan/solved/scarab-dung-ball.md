# Scarab dung ball

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user.

## Idea

The giant scarab rolls a dung ball at the player: a ranged attack that rolls along the row. Khepri, who rolls the
sun across the sky, rolls a bigger one.

* Giant scarab: when the player is on its row 4 to 6 tiles away (default) and not in melee, it turns its back,
  pushes off a ball (wind-up ~0.8 s). The ball rolls along the floor (default 4 tiles/s), blunt damage on contact,
  then breaks. It falls into pits, stops at walls, rolls over traps. The player jumps over it.
* Cooldown: default 4 s.
* Closer than 4 tiles the leap of [monster-balance](monster-balance.md) step 2 (2-3 tiles) wins: the
  ball for range, the leap to close in. Whichever plan lands second keeps the bands apart.
* Khepri (the boss scarab): a bigger, faster ball, farther range, more damage (boss rule; its `kin` is the scarab,
  but the user wants Khepri to have it). Optional: a burning sun ball with [fire-damage](fire-damage.md).
* The small scarab does not roll balls.
* Journal note: "It rolls its dinner at me. I am not sure whose dinner."
* Sound: a rolling rumble, a wet thud.

## Tests

* Unit / sim: ball rolls, hits, falls into a pit, is jumped over; cooldown holds; Khepri's ball is bigger and faster.
* Scenario with a screenshot.

## Implemented (2026-10-22)

Point by point:

* Giant scarab: with the player on its row, out of bite reach and no wall between, it turns its back
  (`Monster::StartRoll`, `facing = -dir`), stands on its head and shapes the ball (0.8 s, the ball grows at the edge of
  its box), kicks it off. The ball (`Dungeon::DungBall`, `world/dungeon_balls.cpp`) rolls along the floor at the
  player, blunt damage on contact (dodge counts), then breaks. It falls into a pit (breaks at its bottom), sinks in
  half water, breaks on a wall (or wall spikes out, a closed gate), rolls over traps, and falls apart after
  `DUNG_BALL_MAX_ROLL` (4) tiles. A jump clears it (its top is 2 x radius off the floor).
* **Range cut** (as the baboon's): the draft's 4-6 tiles kicks it off the screen, the view shows about 1.8 tiles
  either side of the player, and the roller with its back turned must stand in it with its head. Giant scarab: 0.6-1
  tile between the boxes, 2.2 tiles/s, radius 0.1, 10 damage. The windup is the tell; a jump at the kick clears it.
* Cooldown 4 s (Khepri 3.5 s).
* The leap band of [monster-balance](monster-balance.md) step 2 is not in yet: that draft now says to keep
  its leap out of the ball's band.
* Khepri: bigger (radius 0.13), faster (2.8 tiles/s), from a little farther (1.1), more damage (28), and it burns
  (60 blunt : 40 fire, the player burns for 2 s; flames on the ball once half shaped and on its way, `Fire::SUN_BALL`,
  its light). Her bigger body leaves no more room in the view for a farther range.
* The small scarab rolls none.
* Journal (`CreatureMove::RollBall`): "It rolls its dinner at me. I am not sure whose dinner. A jump clears it."; for
  Khepri "She turns her back on me and rolls a burning sun along the floor, as she rolls the sun across the sky. A
  jump clears it."
* Sound: the kick and the rolling rumble (`scarab_att_ranged.wav`), a wet thud as it breaks
  (`scarab_ball_thud.wav`, `WorldSound::DungThud`), `tools/audio/scarab_sounds.py`; a splash when it sinks.
* Model: the roll clip `scarab_att_ranged.md3` (15 frames, `ROLLER_CLIPS`; `scarab.py --export-clip _att_ranged`), the
  ball `media/models/items/dung_ball.md3` and its texture (`items.py`'s `dung_ball`).
* Threat (`MonsterKind::threat`) left as it was: balance is monster-balance's.

Tests: `tests/unit/dung_ball_test.cpp` (the rules, the windup and the hit, the jump, over the spikes, the end of the
roll, the cooldown, the pit, the water, Khepri's burning ball; levels `tests/levels/dung_ball*`), scenario
`tests/scenarios/solved/dung_ball.txt` (the windup, the kick, the hit, the boxes, Khepri's; screenshots).

Play-test: `bin/game --play tests/play/scarab-dung-ball.txt` (the giant scarab); `bin/game --level 5` (Khepri).
