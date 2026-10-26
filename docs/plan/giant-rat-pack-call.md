# Giant rat: pack call

Status: draft 2026-10-19, ready (needs nothing first). From the user's review: each giant gets an ability of its
own. Its bite already gives disease (`meleeCurses`, `src/world/monster_kinds.cpp`), so the new ability is a call.

## Idea

Hurt, the giant rat squeals and calls the rats near it.

* **When:** once per giant rat, when its HP drops below 50%.
* **Call:** a loud squeal (the rat's sound pitched up, louder). Every rat and giant rat within 8 tiles, awake or not,
  wakes and runs to the player (aggro as if it had seen them). It calls no new rats: it only gathers the level's.
* No boss has the rat as kin: the boss rule does not apply.
* Journal note: "Hurt the big one and the small ones come. Rats are loyal. Who knew."

## Tests

* Unit / sim: below 50% HP the giant rat calls once; rats within 8 tiles aggro, rats farther away do not.
