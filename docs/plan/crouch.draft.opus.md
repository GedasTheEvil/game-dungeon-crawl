# Crouch: crawl spaces and ducking attacks

Status: draft 2026-11-02. Needs first: [subcell-physics](solved/subcell-physics.md). From the user.

A new action: the player crouches to pass under a low ceiling (a crawl space) and to duck high attacks.

## Action

* A crouch key, rebindable on the options screen; pick a free default (C or Left Ctrl). Hold to crouch.
* Crouched: about 0.6 of the height (0.6 x 0.375 = 0.225 tiles, under a one-quarter crawl space),
  walk at a third of the walk speed (from the user), no sprint, no jump, no climbing. Under a ceiling too low to
  stand, the player stays crouched when the key is let go.
* No stamina cost for crouching (from the user).
* No attacks while crouched, melee or ranged (from the user). Interact works crouched.
* Archeologist clips in Blender 5 ([modeling.md](../modeling.md)): crouch idle, crouch walk, and stand to crouch and
  back. Notes in [remodeling.md](../remodeling.md).

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
* Play script `tests/play/crouch.txt`.
* Glossary: "crouch", "crawl space" if not added yet, "attack height".
