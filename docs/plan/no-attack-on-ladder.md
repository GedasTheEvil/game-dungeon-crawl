# No attack while on a ladder

Status: implemented 2026-10-09, not play-tested yet (see [Implemented](#implemented-2026-10-09)). From the user: the
player cannot attack while climbing a ladder.

## Today

* `tryAttack` (`src/input/input.cpp`) starts a swing or a bow draw whenever the last one is over. It does not check
  the ladder, so the player can fight from the rungs.
* `Dungeon::PlayerOnLadder` (`src/world/dungeon_base.cpp`) tells when the player holds on to a ladder: on a ladder
  cell, within `LADDER_REACH` of the grip, and not standing on the floor at its foot.
* The player shows the climb clip there (`Player::showClimb`); a swing on the ladder mixes the attack into it.

## Idea

* `tryAttack` refuses while `PlayerOnLadder()` and shows a status line ("Hands on the rungs"), like "The water
  is too deep to jump" (`Dungeon::JumpAllowed`).
* At the foot of a ladder (standing on the floor), or just beside it past `LADDER_REACH`: attacking works as today.

## Decided (2026-10-09)

* **Weapons:** all of them: melee, the bow, throwing weapons. Both hands are on the rungs.
* **Mid-swing:** a swing or draw already under way when the player grabs the ladder finishes (and can hit). Only new
  attacks are refused.
* **Feedback:** the status line "Hands on the rungs", once per press: a held key does not repeat it.
* **Everyone:** the rule holds for monsters too, once they climb ([monster-climbers](monster-climbers.md)).
* **Ladder tops:** accepted. A monster waiting at the top gets the first hits; the player climbs down or waits. No
  checker rule, no change to the monsters.

## Open (for the implementer)

* Check the campaign levels for a monster spawn next to a ladder top. Move it if the spot is a trap (no way to back
  off). `levelcheck` does not model attacks.
* Tests: a scenario in `tests/scenarios/` (attack on the ladder: `expect attacking == 0`; at its foot: 1).

## Implemented (2026-10-09)

* `Dungeon::AttackAllowed(tell)` (`src/world/dungeon_base.cpp`): false while `PlayerOnLadder()`; with `tell` it says
  "Hands on the rungs". `tryAttack` (`src/input/input.cpp`) asks it before a new swing or draw; a swing under way
  finishes (`updateAttack` does not ask). Once per press: the key's release (`releaseBinding`) or a new mouse click
  lets the next refusal tell again, the key repeat does not.
* Ladder tops of the campaign (`levelcheck --map`, monsters within 3 cells of a ladder's top cell on its row): lvl4
  (a plant, rooted), lvl6 (a rat), lvl12 (a plant; a mummy 2 cells off, it wakes as the player arrives), lvl13 (a
  giant bat; a mummy 2 cells off), lvl26 (a mummy 3 cells off). None is a trap: the ladder leads back down, the
  mummies are slow, the plants rooted. No level changed.
* Tests: `tests/unit/world_rules_test.cpp` (no attack mid-ladder, the status line, an attack at its foot, a swing
  carried onto the rungs lands); `tests/scenarios/ladder.txt` checks the key path (`attack` on the rungs:
  `attacking == 0`; at the foot: 1).
