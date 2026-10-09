# No attack while on a ladder

Status: draft 2026-10-09, refined 2026-10-09 (decided, not implemented). From the user: the player cannot attack while climbing a ladder.

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
* **Everyone:** the rule holds for monsters too, once they climb ([monster-climbers](monster-climbers.draft.md)).
* **Ladder tops:** accepted. A monster waiting at the top gets the first hits; the player climbs down or waits. No
  checker rule, no change to the monsters.

## Open (for the implementer)

* Check the campaign levels for a monster spawn next to a ladder top. Move it if the spot is a trap (no way to back
  off). `levelcheck` does not model attacks.
* Tests: a scenario in `tests/scenarios/` (attack on the ladder: `expect attacking == 0`; at its foot: 1).
