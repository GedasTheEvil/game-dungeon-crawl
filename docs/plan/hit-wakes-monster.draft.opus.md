# A hit wakes the monster: it comes for the player, or runs

Status: draft 2026-10-30 (user). Needs nothing first.

## Problem

A monster hit from afar (an arrow, any ranged hit) acts as if nothing happened. Seen with a bat: shot on its roost,
it stays hanging there. It should fly at the player: the hit woke it.

`Monster::takeHit` already wakes a lurker and sets `alerted`, but the AI does not act on `alerted` alone. The bat
hunts only when `Dungeon::flightGoal` says it `sees` the player: `BAT_SIGHT` (1.75 tiles) for a bat that is not
`Monster::Hunts()` (a minion, an alerted boss). An arrow flies farther than that, so the bat goes back to idle.

## Rule

A monster hit by the player (any weapon, any range) has noticed them: from then on it behaves as if it had seen them
(the minion's / alerted boss's hunt, `Hunts()`, or an own "provoked" flag; the implementer's call). For every kind:

* It can reach the player: it comes, by its normal ways (fly, walk, climb, jump, burrow, swim, its ranged attack from
  where it stands).
* It cannot reach the player: it runs away from them (like `Dungeon::fleeFrom`, plan coward-flee-ranged), not stand
  and take arrows. Today only cowardly walkers flee (`Monster::flees`); widen that to every kind that can move away.
  A kind that cannot move at all (rooted: the plant, ...) stays.

Go through every locomotion and wake rule: fliers (`BAT_SIGHT`), walkers, ambushers and lurkers (mummy, cobra,
submerged, floating, figurine, sand worm, rerek wake ranges), burrowers, swimmers, the bosses (`BOSS_WAKE_RANGE`).
Each one's "sees the player" test gets a "or it was hit" branch.

Open points (implementer's choice):
* Does it forget? E.g. a provoked monster that has not seen the player for N seconds goes back to its roost / post.
  Simplest: never forgets on this level.
* A trap's hit (falling rock, spikes) is not the player's: no wake from it, or wake without a target. Only player hits
  are the rule here.
* A pack call (`answersCall`) already alerts the pack; whether those answering also hunt from afar like this.

## Tests

Unit tests (`tests/unit/`): a roosting bat hit from beyond `BAT_SIGHT` leaves its roost toward the player; a walker on
another row with no way up runs away when hit; a rooted one stays. A scenario only if a screenshot is wanted.

Play-test: `bin/game --level N` with a roosting bat (lvl8 has several), shoot one from afar.
