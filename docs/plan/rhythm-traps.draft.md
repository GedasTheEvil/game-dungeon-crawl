# Rhythm traps: timed wall spikes

Status: draft 2026-10-10. Needs nothing first. From the user.

## Idea

A trap on a cycle: poisoned spikes shoot out of a wall, stay out, retract, wait, repeat. Always on the clock, never
triggered. Sprinting and jumping do not beat it: the player must time the pass and cross while it is safe.

* **Cycle:** `period` = out time + retracted time (ms), plus a `phase` offset. Phases: extended (hurts), retracted
  (safe), and a short telegraph just before extending (spikes twitch, a click or scrape) so the player can read it.
* **Placement:** spikes come out of a wall beside the corridor (a side wall, or a floor strip); a run of cells in
  front of the wall is the danger zone while extended. Sideways spikes cannot be jumped: the jump gives no
  clearance, and sprint only cuts the time in the zone, not the hit. While extended, the zone does
  continuous damage to whoever stands in it (the `TrapHurt` tick, growing the longer it stays).
* **Hit:** continuous damage while in the zone and extended, plus weak poison ([stronger-poisons](stronger-poisons.md) tiers; the
  tier is per trap, default weak). Resistance and `trapCutPercent` (trap ward) apply as for spikes
  (`SPIKE_ATTACK_MIX`, `TrapHurt` / `Dungeon::updateTraps` are the model).
* **Combining:** every trap has its own `phase` and `period` (a "speed": fast, normal, slow). Two or three in a row
  with different phases make a rhythm puzzle: the safe windows must overlap enough to cross (the level checker
  verifies a safe path exists, see Level check).
* **Monsters:** same rule as traps now: reckless take the hit, coward stop at the edge (`Courage`). Cowards wait for
  the safe window, if cheap to do; else treat it as a trap tile.
* **Draw:** a wall-mounted spike model (reuse the spike model, animated out of a slot in the wall); a faint
  glow/stain on the poisoned tips (green) so it reads as poison. Map symbol like spikes.
* **Sound:** scrape on telegraph, thunk on extend. The cycle is audible from afar.

## Level format

A new glyph (pick a free one) for the trap tile. Phase and period: per tile, from a level-file key or by glyph
variants (fast / slow), implementer's choice. A run of tiles with auto-staggered phases is a handy default
(`phase = index * step`).

## Level check

(`src/world/level_check.h`) A rhythm trap is passable when its window is safe, so it never softlocks, but a
combination must be crossable with the player's walk speed: the checker simulates the cycle against walk speed
(not sprint) and warns if no start time crosses the run. Count them like `spikes` / `pathSpikes`.

## Open points

* Walk speed vs. safe window: how long the minimum safe window is (suggest at least 1.5x the time to cross
  the zone at walk speed).
* Dodge ([skill-dodge](skill-dodge.draft.md)): may avoid a single event (one stab or one tick), never sustained
  time in the zone. Levitation and speed do not bypass it. Default as stated; exact rule is the implementer's choice.
* Boss ability rule: no monster gains an ability here, so no boss change.

## Tests

* Unit: the cycle phase at time t (extended / retracted / telegraph), two traps with different phases give the
  expected overlap of safe windows.
* Unit: the checker warns on an uncrossable combination, passes a crossable one.
* Scenario: walk into the zone while extended: hit and poison; wait for the retract and cross: no hit; sprint
  and jump into it while extended: still hit.
