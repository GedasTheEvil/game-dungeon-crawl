# The screech hurts, and the daze shows

Status: draft 2026-10-30 (user). Needs nothing first.

## Problem

The screech (plan giant-bat-screech, `ScreechRules`) only dazes: 1.5 s (Shezmu 2 s) at half walk, no sprint, no
jump, no grip on a ladder, the view rolls and sways. On flat floor that reads as a camera shake and nothing else; the
user thought it did nothing. No damage, and only the "Your ears ring!" status line says the player is dazed.

## Changes

1. **Damage.** The screech's blast (the tell's end, the player still in the cone) deals a little damage: a new
   `ScreechRules::damage`, through the armour like a hit (`Player::TakeHit`, a sound mix fitting a scream). Giant
   bat about 8 (its bite is 28), Shezmu more (about 12; Boss abilities, AGENTS.md). Balance is the implementer's
   call, against monster-balance's numbers.
2. **The daze shows.** While `Player::Dazed()`:
   * Stars (or little birds / Egyptian-flavoured sparks, e.g. small gold ankhs or stars of Nut) circle the player's
     head, the cartoon "stunned" look. Drawn in the world, at the head, through the daze; fading out with the view
     over `DAZE_CLEAR_MS`.
   * A HUD tile like the curse tiles (`drawCurses`, purple frame; or its own frame colour, it is not a curse) with a
     daze icon and the seconds left, in the row with poison / resistance / curses. A new icon in the HUD atlas
     (`PlayerHud::Icon`).
   Both, unless one alone reads clearly enough in a play-test (implementer's choice; the stars are the must).
3. **Optional, more bite** (implementer's choice, balance): daze 2 s for the giant bat (Shezmu 2.5 s); monster bites
   on a dazed player +25%. Leave out if the damage alone makes the screech count.

Journal: the giant bat's / Shezmu's screech text (`content/texts/moves.txt`, `screech`) says it hurts.
Glossary: update the `screech` and `daze` rows.

## Tests

Unit: a screech blast on a player in the cone takes health (armour applied); out of the cone, none. Scenario
(`tests/scenarios/`): `Daze` the player (scenario command exists, `src/test/scenario.cpp`), screenshot the stars and
the HUD tile.

Play-test: `bin/game --level 8` (giant bats), let one screech at you.
