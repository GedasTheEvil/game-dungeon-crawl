# Sprint drains stamina while standing still

Status: solved 2026-10-05 (play-tested). Bug.

Done: `Dungeon::Move()` returns whether the player moved; walk steps that moved call `PlayerStats::NoteWalked()`;
`UpdateStamina()` drains (and refuses) only with shift held and a real step this tick. `SprintMoveMultiplier()`
looks at shift + stamina left, so the first step is already a sprint step. Jump and fall moves do not count.
Scenario `tests/scenarios/sprint.txt` (new commands `sprint on|off`, `hold <dir> T`).

## Bug

Holding shift without moving drains stamina (5% of max per drain tick) and stops regeneration. You cannot stand
still fast: standing with shift held must cost nothing and regenerate like plain standing.

Cause: `PlayerStats::UpdateStamina()` (`src/entities/player_stats.cpp`, called each tick from `game_loop.cpp`)
only checks `sprint_requested`, set by shift press / release in `src/input/input.cpp`. It never asks whether
the player moved this tick.

## Fix

* Sprinting = shift held **and** the player actually moved this tick. Otherwise no drain, normal regeneration,
  and no stamina refusal flash.
* "Actually moved" means real displacement, not a walk key held: walking into a wall or a blocked cell with shift
  held does not drain either. `Dungeon::Move()` knows if the step happened.
* Keep `sprint_requested` as is (shift state), so starting to walk with shift already held sprints right away.
* Check other paths that can count as movement: the jump (has its own stamina cost), being pushed, sliding.

## Checks

* Scenario: hold shift standing still for some seconds, stamina stays full (or regenerates from below max).
* Scenario: hold shift and walk into a wall, no drain.
* Scenario: sprint along a corridor, drain as before.

Related: [sprint-motion-effect.md](../sprint-motion-effect.md) uses the same "actually moving" signal.
