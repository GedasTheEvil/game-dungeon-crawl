# No attack while on a ladder

Status: draft 2026-10-09 (idea, not decided). From the user: the player cannot attack while climbing a ladder.

## Today

* `tryAttack` (`src/input/input.cpp`) starts a swing or a bow draw whenever the last one is over. It does not check
  the ladder, so the player can fight from the rungs.
* `Dungeon::PlayerOnLadder` (`src/world/dungeon_base.cpp`) tells when the player holds on to a ladder: on a ladder
  cell, within `LADDER_REACH` of the grip, and not standing on the floor at its foot.
* The player shows the climb clip there (`Player::showClimb`); a swing on the ladder mixes the attack into it.

## Idea

* `tryAttack` refuses while `PlayerOnLadder()`. Maybe it shows a status line ("Hands on the rungs"), like "The water
  is too deep to jump" (`Dungeon::JumpAllowed`).
* At the foot of a ladder (standing on the floor), or just beside it past `LADDER_REACH`: attacking works as today.
* A swing already under way when the player grabs the ladder: open (see below).

## Open

* Every weapon, or only melee? A bow needs both hands too, so most likely all.
* A swing that started before the climb: let it finish, or cut it off?
* Status line or silent? The key may be held down, so a line on every refused press could spam.
* Does this make any level harder, e.g. a monster that waits at the top of a ladder? Today the player can hit it from
  the rungs. Check the campaign levels with a monster next to a ladder top. `levelcheck` does not model attacks.
* Tests: a scenario in `tests/scenarios/` (attack on the ladder: `expect attacking == 0`; at its foot: 1).
