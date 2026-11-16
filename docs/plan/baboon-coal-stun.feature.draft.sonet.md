# Baboon coal: a chance to daze the player for 3 s

Status: draft 2026-11-16, size S, needs nothing first.

## Idea

The baboon's thrown coal (`MonsterBaboon`, `RangedRules`, `src/level/monster_kinds.cpp`) gets a chance to stun the
player for 3 s. The player's stun is the daze (glossary "daze": `Player::Daze`, "Your ears ring!"; half the walk
speed, no sprint, no jump, no grip on a ladder, the view sways, stars, the HUD tile). Today the giant bat's and
Shezmu's screech and a hard head bump (`Player::HeadBump`) daze; the rock trap that falls on the head stuns too.
Reuse it, no new state.

## Plan

* `RangedRules` gets a daze chance and duration (data, not a baboon special case). Baboon: 3000 ms, the chance the
  implementer's balance call (suggestion 20%), rolled on `sim.random->gameplay` when the coal hits (not on a block or a
  miss).
* Only when the player is not dazed already (user: "if it does not already"): a hit during a daze rolls nothing and does
  not extend it.
* Boss (AGENTS.md, Boss abilities): Babi's coal (`MonsterBabi`, `kin` baboon) gets it stronger, e.g. 30% and 4 s.
* The monster descriptions (baboon, Babi) and their notebook / human notes mention it where they list the coal.
* Unit test (`tests/unit/`): a coal hit with the roll forced dazes 3 s; a second hit during the daze leaves it as is.

## Play-test

* `tests/play/baboon-coal-stun.txt`: a few baboons in range, high HP.
* Some coal hits daze you ("Your ears ring!", stars, slow walk) for about 3 s.
* Babi's coal dazes more often and longer.
