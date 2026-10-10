# Scarab dung ball

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user.

## Idea

The giant scarab rolls a dung ball at the player: a ranged attack that rolls along the row. Khepri, who rolls the
sun across the sky, rolls a bigger one.

* Giant scarab: when the player is on its row within range (default 6 tiles) and not in melee, it turns its back,
  pushes off a ball (wind-up ~0.8 s). The ball rolls along the floor (default 4 tiles/s), blunt damage on contact,
  then breaks. It falls into pits, stops at walls, rolls over traps. The player jumps over it.
* Cooldown: default 4 s.
* Khepri (the boss scarab): a bigger, faster ball, farther range, more damage (boss rule; its `kin` is the scarab,
  but the user wants Khepri to have it). Optional: a burning sun ball with [fire-damage](fire-damage.draft.md).
* The small scarab does not roll balls.
* Journal note: "It rolls its dinner at me. I am not sure whose dinner."
* Sound: a rolling rumble, a wet thud.

## Tests

* Unit / sim: ball rolls, hits, falls into a pit, is jumped over; cooldown holds; Khepri's ball is bigger and faster.
* Scenario with a screenshot.
