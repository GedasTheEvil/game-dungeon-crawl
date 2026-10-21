# Kiting monsters: keep the distance, shoot, back off

Status: draft 2026-10-21, from the user (for the baboon of the Lake of Fire). Implemented 2026-10-21, see
[Done](#done). No kind kites yet, so there is nothing to play-test until the [baboon](solved/baboon.md); the unit
tests make the Anubis a kiter. Built on the ranged attack (`RangedRules`, `Monster::AttackRanged`) and the coward's
flight ([coward-flee-ranged](solved/coward-flee-ranged.md): `Dungeon::walkerReaches`, `Dungeon::fleeFrom`).

## Idea

A new AI for monsters that would rather shoot than bite. Today a ranged attacker (cobra, Anubis guard, mummy priest)
shoots when the player is on its row and in range, and otherwise walks up and bites. A kiter keeps the player at its
range: when the player comes too close, it backs off, but only as far as it still reaches them with a shot. It does not
run so far that it loses them. Melee is its fallback, not its plan.

## Behaviour

* **A distance band** per kind: `near` (too close, back off) and `far` (its range, or a little less). Inside the band
  it stands and shoots when the shot is ready. Beyond `far` it comes closer, up to the band.
* **Threatened:** the player is nearer than `near` and coming at it (closing in over the last moments, not just
  standing there). Then it backs off along its row, away from the player, until it is back in the band. A
  hysteresis (back off at `near`, stop a little past it) keeps it from jittering.
* **Never loses the player:** it does not back off past `far`, does not leave the player's floor unless the way back
  goes on (a ladder for a climber), and stops at a trap, a gap or deep water like a coward. Retreating is no escape:
  it stays in the fight.
* **Cornered:** a wall, a drop, a trap or another monster behind it, or the player in bite reach: it fights in melee
  (its normal bite or blow). Once there is room again, it backs off again.
* **Speed:** it backs off a bit faster than the player walks, slower than a sprint. Sprinting catches it, a corner
  traps it. That is the counterplay: cornering it, or cover against its shots.
* **Out of sight** (another floor, round a corner): it seeks as today, up to its band.

Implementer's choice: share the "where to back off" search with `fleeFrom` (it already finds the cells along the
row), and whether a climber also backs off up a ladder.

## Who

* The baboon and its boss Babi ([baboon](solved/baboon.md), [babi-boss](babi-boss.draft.opus.md)): the first
  kiters. Babi's band is wider and he backs off faster, within his arena.
* Not today's ranged attackers by default. [monster-balance](monster-balance.draft.opus.md)'s sim pass may switch some
  on (a cobra or the mummy priest that kites), with the user's yes.

## Checker

A kiter is harder to pin down than a walker of the same stats: a `threat` factor for kiters
(`monsterThreat`, `src/world/level_check.cpp`), for example x1.15, implementer's choice.

## Tests

On the sim harness ([sim-library](solved/sim-library.md)):

* The player walks at a kiter: it backs off and stays in the band, and shoots.
* The player stands still in the band: it does not move.
* A wall behind it: it bites the player in reach.
* It never backs off past its range, and does not leave the floor (a drop shaft behind it: it stops and fights).
* The player sprints at it: they catch it.

## Done

* `KiteRules` (`monster_kinds.h`): `near`, `far`, `backOffSpeed` (speed points: 19 the player's walk, 57 the sprint).
  `MonsterKind::kite` turns it on. No kind has one yet: the baboon and Babi set theirs.
* `Dungeon::kiteAway` / `Dungeon::kite` (`dungeon_walkers.cpp`), called from `updateWalker` after the climber's path
  and the light shunning, before the coward's flight. The shot in `UpdateMonsters` waits while it backs off.
* Threatened: `Monster::WatchPlayer` notes a step of the player towards it (`KITE_CLOSING_MS`). Change from the draft:
  while the player keeps coming, it backs off all the way to `far`, not only `KITE_HYSTERESIS` past `near`. With the
  short stop, a long ranged clip (the Anubis's, about 2 s) let a walking player reach it during the shot. Once the
  player stops, it stops `KITE_HYSTERESIS` past `near`. It never steps past `far`.
* Cornered: a wall, a drop, a trap (also for a reckless kiter), a lit cell for a light shunner, or another monster on
  its row behind it. Then it stands in its band, shoots when it can and bites the player in reach (the walker's rules).
* No back-off up or down a ladder (implementer's choice): off its row it follows the player as before.
* The back-off shares `fleeDir` with the coward's flight (it faces away while it backs off); `AttackRanged` clears it, so
  it turns to the player for the shot.
* Checker: `KITER_THREAT_FACTOR` 1.15 in `monsterThreat`.
* Tests: `tests/unit/kite_test.cpp` on `tests/levels/kite_{open,wall,drop}`: band and shot, standing still, the walk at
  it (backs off, never caught, never past `far`), a wall and a drop shaft behind it (it bites, stays on its floor), the
  sprint catches it.
* Fixed on the way: `tools/level/ascii2level.py` wrote level files without the zone line the loader now wants (plan
  zone-looks); it writes `zone none`.
* Changed with the [baboon](solved/baboon.md), the first real kiter: a kiter breaks off the rest of its ranged clip
  once the shot has left, to back off (a walking player reached it in the clip's tail). Cornered means no room behind
  (`Dungeon::kiteRoom`) for a step; cornered nearer than `near`, with no room to get back to it, it closes in and bites
  (before, it sat in reach of the player's weapon, out of its own bite reach). A journal move, `CreatureMove::Kite`.
