# Crouch: crawl spaces and ducking attacks

Status: solved 2026-11-04 (play-tested by the user). Was: implemented 2026-11-04, waiting for the play-test (play script removed). Was: draft
2026-11-02, needed [subcell-physics](subcell-physics.md) first. From the user.

Implementer's choices:

* The key: the down key (`climb_down`, S / arrow down), no key of its own (from the user: the controls are many
  already, and there is no crouch on a ladder). Held off a ladder it crouches; in a ladder's cell (also standing at a
  ladder's top) it climbs down as before. Climbing down to the floor with it held ends in a crouch.
* Height `CROUCH_HEIGHT_FACTOR` 0.55 (not 0.6): the crouched hitbox's top must stay under `SHOT_CHEST` (0.6 of the
  standing height), else a shot aimed at the standing chest still grazes it (`static_assert` in `movement.h`). Body
  0.206 tiles, under a quarter crawl space. The pose's top is 0.56 of the idle's.
* Shots are aimed at the player's chest as they leave (`aimShot`, the crouched chest if already down): a duck dodges
  the shots already on the way, not the next ones. Darts and other hits read the same crouched hitbox
  (`Player::Height`, `Player::Covers`).
* Attack heights in `MeleeReach::height`: High for the `HIGH_BLOW` kinds (Anubis, the Anubis boss, mummy, mummy
  priest, ushabti), Low (`LOW_BITE`) for the scarabs, worms and Rerek, rats, crocodiles and Sobek, scorpions and the
  queen, the egg cluster, the lotus, the camel spiders and the Mother of Webs; Body for the rest (bats, cobras and
  Apep, hippos and Seth, baboons and Babi, the plant, the mimics). Low and Body hit a crouched player the same (the
  standing box is tested); a High swing misses (the cone not tested, a touch inside the body still hits). A boss's
  band must be its kin's (`validKinds`).
* A crouch starts on the floor only (not in a jump or a fall, not in a ladder's cell); the player stands up once
  nothing solid is over the crouched body up to the standing head (`Dungeon::roomToStand`: a whole cell over the
  centre, sub-cells across the body).
* A refused attack says "Stand up to strike". The stand-to-crouch clip takes `CROUCH_TRANSITION_MS` (200 ms) either
  way; its frame follows `Player::crouchPhase`.
* Balance: `bin/balance table` gives the same numbers as before (the sim's player never crouches).

A new action: the player crouches to pass under a low ceiling (a crawl space) and to duck high attacks.

## Action

* A crouch key, rebindable on the options screen; pick a free default (C or Left Ctrl). Hold to crouch.
* Crouched: about 0.6 of the height (0.6 x 0.375 = 0.225 tiles, under a one-quarter crawl space),
  walk at a third of the walk speed (from the user), no sprint, no jump, no climbing. Under a ceiling too low to
  stand, the player stays crouched when the key is let go.
* No stamina cost for crouching (from the user).
* No attacks while crouched, melee or ranged (from the user). Interact works crouched.
* Archeologist clips in Blender 5 ([modeling.md](../../modeling.md)): crouch idle, crouch walk, and stand to crouch and
  back. Notes in [remodeling.md](../../remodeling.md).

## Ducking (from the user: missiles and high swings)

* Crouched, the player's hitbox is lower. Missiles (arrows, bolts, thrown stones, spit) fly at a height; one that
  passes over the crouched hitbox misses.
* Melee attacks get a height band in `MonsterKind` (low, body, high): a high swing misses a crouched player, a low
  one (a rat's bite, a scarab, a crocodile's snap) hits all the same. Set the band per kind; bosses as their kin.
* Balance: a duck must not make a fight free. Crouched, the player cannot strike back, moves at a third of the speed
  and cannot jump; a low attack still hits. Check the balance sim (docs/development.md) with a player that never
  ducks, so its numbers stay as today.

## Tests

* Unit: crouch under a low ceiling, cannot stand up there, stands up after; a high missile and a high swing miss
  when crouched, a low bite hits; crouched walk is a third of the walk speed; no attack and no stamina drain while
  crouched.
* Scenario with screenshots of the crouch poses and a crawl space on `tests/levels/subcells`.
* Play script `tests/play/crouch.txt` (removed once solved).
* Glossary: "crouch", "crawl space" if not added yet, "attack height".
