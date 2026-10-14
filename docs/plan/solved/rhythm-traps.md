# Rhythm traps: timed wall spikes

Status: solved 2026-10-14, play-tested by the user:
`bin/game --play tests/play/rhythm-traps.txt`. From the user. See [Implemented](#implemented-2026-10-14): the user
changed the rule before it was built (out, the spikes are a wall).

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
* Dodge ([skill-dodge](skill-dodge.md)): may avoid a single event (one stab or one tick), never sustained
  time in the zone. Levitation and speed do not bypass it. Default as stated; exact rule is the implementer's choice.
* Boss ability rule: no monster gains an ability here, so no boss change.

## Tests

* Unit: the cycle phase at time t (extended / retracted / telegraph), two traps with different phases give the
  expected overlap of safe windows.
* Unit: the checker warns on an uncrossable combination, passes a crossable one.
* Scenario: walk into the zone while extended: hit and poison; wait for the retract and cross: no hit; sprint
  and jump into it while extended: still hit.

## Implemented (2026-10-14)

User decisions (2026-10-14), over the idea above:

* **Out, the spikes are a wall**, like a closed gate (`isSolidTile`): no walking, sprinting or jumping through. No
  continuous damage in a danger zone: the only hit is on whoever stands in the cell as they come out, one stab
  (`SPIKES_STAB_DAMAGE` 10, `SPIKE_ATTACK_MIX`, cut by the trap ward) and weak poison, then a shove out of the cell
  (the near side if open). Monsters the same (`TrapHit`, no XP for the kill).
* **Dodge**: the Dodge skill may dodge the stab (no damage, no poison); the shove happens anyway.

What went in:

* Tile `WallSpikes` (16), glyph `|`: attribute = speed (`SpikeSpeed`: normal 4 s, fast 3.2 s, slow 4.8 s; out 1.2,
  0.8, 1.6 s), value = phase in eighths; the live state sits in the value's upper bits (`spikeState`). Cycle in
  `src/world/spike_cycle.h`, numbers in `gameplay_config.h` (a `static_assert` keeps each safe window at 1.5x the walk
  through a cell). Telegraph: the last 0.6 s before they come out, tips twitch out of the slots, a scrape.
* Game: `Dungeon::updateWallSpikes` / `stabWallSpikes` (`dungeon_spikes.cpp`). Cowards treat a telegraphing cell as a
  trap (`walkerBlockedAt`); out, everyone is blocked.
* Look and sound: `spike_slots` (slotted sandstone panel, green stains) and `wall_spikes` (bronze, green tips) in
  `tools/blender/models/mechanism.py`; `spike_scrape.wav`, `spike_thunk.wav` (`tools/audio/mechanism_sounds.py`),
  heard only in view. Map: a 3 x 3 grid of slots. Editor hints, `tools/editor/readme.md`.
* Checker: counts (`wallSpikes`, `pathWallSpikes`), path cost, a wall above and a floor under, and
  `wallSpikesCrossable`: a run of them in a row must be walkable both ways at walk speed without stopping (else a
  softlock one way). Result: at normal speed a run of 3 never is (3.5 s through, 2.8 s in), 2 at the same phase or at
  alternate phases (0 and 4) are. Rhythm puzzles: single traps or pairs, with free cells between to wait in.
* Tests: `tests/unit/wall_spikes_test.cpp` (cycle, checker, the trap in the world, the trap ward, a mummy stabbed and
  shoved); `tests/scenarios/wall_spikes.txt` (screenshots through one cycle).

Not done:

* No campaign level has them yet ([denser-levels](../denser-levels.draft.opus.md) or a level pass can place them).
* No journal field note for them (the journal texts were being rewritten in parallel, plan simple-damage-words).
* Per-trap poison tier: all weak.
* Flyers are not stabbed (they are blocked while the spikes are out).
