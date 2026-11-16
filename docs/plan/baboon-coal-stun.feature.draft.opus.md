# Baboon coal: a chance to stun the player for 3 s

Status: draft 2026-11-16, size M, needs nothing first.

## Idea

The baboon's thrown coal (`MonsterBaboon`, `RangedRules`, `src/level/monster_kinds.cpp`) gets a chance to stun the
player for 3 s. The player has no stun state today: stun is monster-only (`Monster::StunHit`, plan skill-stun), so
this adds one.

## Plan

* Player stun: a `GameClock` end time on the player (not saved, seconds). While stunned: no walk, jump, climb, attack
  or item use; gravity, damage, poison and burning go on. Stars over the head as the monsters' (`drawStunStars`) or a
  screen cue (implementer's choice), a HUD hint, and a sound.
* `RangedRules` gets a stun chance and duration (data, not a baboon special case). Baboon: 3000 ms, chance the
  implementer's balance call (suggestion 20%), rolled on `sim.random->gameplay` when the coal hits.
* No new stun while one runs (no refresh, no stacking).
* Boss (AGENTS.md, Boss abilities): Babi's coal (`MonsterBabi`, `kin` baboon) gets it stronger, e.g. a higher chance
  and 4 s.
* Interactions to decide: a stunned player on a ladder holds on; in water stays afloat as now; the Wedjat / resistance
  amulets do not block it unless the implementer finds a fitting rule.
* Monster description strings and the baboon's notebook / human notes mention the stun where they list its attacks.
* Unit test: a coal hit with the roll forced stuns 3 s (input ignored), a second hit during it does not extend it.

## Play-test

* `tests/play/baboon-coal-stun.txt`: a few baboons in range, high HP.
* Some coal hits stun you: you cannot move or attack for about 3 s, stars / cue show it.
* Babi's coal stuns more often and longer.
