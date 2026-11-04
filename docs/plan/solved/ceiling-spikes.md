# Ceiling spikes: the floor spike traps on a low ceiling, they hurt whoever jumps into them

Status: solved 2026-11-04 (play-tested by the user). Was: implemented 2026-11-04, waits for the play-test (play script removed). Was: draft 2026-11-03. Needs first: [head-bump](head-bump.md),
[monster-head-bump](monster-head-bump.md). From the user.

New traps: spikes on the underside of a low ceiling. Walking under it is safe; a jump into it hurts (from the user).
It makes "don't jump here" a rule the player reads off the ceiling, and a lure for jumping monsters.

## The trap

The floor's spike traps turned upside down (from the user): two kinds, as on the floor.

* **Ceiling spikes:** the floor spikes (`Spike`) on a ceiling.
* **Ceiling death trap:** the death trap (`Death`) on a ceiling, its tips poison strongly (`PoisonTier::Strong`).
* Only on a shaped cell (`Tile::shape`) whose ceiling is low enough for a jump to reach it; from the floor a jump
  never reaches a ceiling a cell up (`movement.h`). The spikes sit on every underside of a solid sub-cell over open
  space in that cell.
* In the level format: two fixtures (`Fixture`, the wall layer, like the poison drip's lump on the ceiling), each with
  a free glyph. The editor places them; the level loader and the checker reject one on a cell with no reachable
  underside.
* A head bump under one (the player's or a monster's) is a spike hit instead of a plain bump. Damage and poison as
  the floor kind: `SPIKE_ATTACK_MIX` through the armour, a whole crossing's damage (`TrapBell::crossHundredths` with
  `SPIKES_CROSS_HP` / `SPIKES_CROSS_PERCENT`, or the `DEATH_TRAP_` pair), the death trap's strong poison once per
  bump. A bump is one short touch, so it takes the whole crossing at once, not a bell over time. This is as deadly
  as walking over the floor kind (200 HP is more than a new player's 50): a ceiling trap is a "never jump here".
  A monster takes what it takes walking through the floor kind (`TrapHurt`), scaled the same way.
* No head bump damage or daze roll on top: the spikes replace the bump. Any contact counts, no velocity threshold:
  the tips reach down.
* Walking, crouching ([crouch](crouch.md)) and standing under it are safe.
* Sound: the floor spikes' stab, not the knock.
* Look: the floor spikes' and the death trap's models turned 180 degrees, on each underside, drawn with the sub-cell
  boxes. If a model does not fit a quarter-tile underside, scale it in its Blender 5 script
  ([modeling.md](../../modeling.md), notes in [remodeling.md](../../remodeling.md)). Visible from the ground, so the player
  can see it before a jump.
* Monsters: cowards (`Courage`) do not jump under it; the reckless and the ones in a pounce do (the lure). Bosses as
  their kin.
* Levels: none required. The [subcell-checker](subcell-checker.md) must warn if the only way on is a jump
  under spikes (forced damage). Add a few to the campaign only with [subcell-campaign](../subcell-campaign.md).

Implementer's choices:

* Two fixture types, `CeilingSpikes` (25, glyph `"`) and `CeilingDeath` (26, glyph `'`); attribute and value unused.
  The undersides: `ceilingUndersides` (`level.h`), the bottom row never (it hangs over the cell below). Reachable
  (`jumpReachesUnderside`, `movement.h`): the open run under an underside is lower than the jump's top head
  (`Jump::ARC.peak + PLAYER_BODY_HEIGHT`); in one cell that is any underside, so the rule only rejects a cell with
  none (no shape, a step only). The loader (`readLevel`) and the checker (an error) reject it; the editor shows the
  checker's error.
* Player: `Dungeon::ceilingTrapAt` asks `ceilingAt`'s three points for a spiked sub-cell; `ceilingSpikesHit` deals
  `TrapBell::crossHundredths` through `TrapDamage` (the trap ward helps) and `TakeHit(..., ignoreArmor)`, as the floor
  crossing; the death trap's strong poison and its venom line. Status "Spikes in the ceiling!", the grunt.
* Sound: the floor spikes have no sound of their own (a walk over them is silent); the stab is the wall spikes'
  `WorldSound::SpikeThunk`.
* Monster: `Monster::CeilingSpikes`: `TrapHurt::rampTotal` of `TrapCrossHits` (its box, `TRAP_BOX_FORGIVE` in, across
  a tile at its walk speed, at `TRAP_HURT_INTERVAL_MS`), cut by `trapDamagePct`; strong poison for the death trap; no
  stun; remembered as a bump (`bumpMemoryMs`), so no second pounce under it at once. The ceiling is `leapRoom`'s
  (`Dungeon::leapCeiling`, `leapCeilingTrap`).
* Cowards: `Dungeon::leapUnderSpikes` sweeps the sub-cell columns from the take-off to the landing for a spiked
  underside first in a leap's reach over the head; a coward does not hop up, leap a gap, leap away or leap on its
  path there (`shunsLeap`). A pounce ignores it (the lure). Bosses as their kin: no boss rule.
* Look: the floor trap's model a quarter tile wide (`Trap::Show(size)`, no Blender change: the engine scales a model
  by its largest side anyway), `CEILING_TRAP_ROWS` (4) along the depth on each underside, the tips stretched
  (`CEILING_SPIKES_STRETCH` 1.5, the death trap's `CEILING_DEATH_STRETCH` 2.5, so it reads as the bigger one).
* Checker: a jump onto a quarter step whose arc reaches a spiked underside (the sub-cell over `STEP_ROOM`, at the
  take-off or the step) is `PathMove::SpikedStep`, `COST_CEILING_SPIKES` 30 on top; on the path it warns ("the only
  way to the exit jumps into ceiling spikes"), `LevelReport::pathCeilingSpikes`. The other jumps need a cell of head
  room, so they never touch one.
* Test level: `tests/levels/ceiling_spikes` (its own, not `tests/levels/subcells`: its tests stay as they are).
  Unit tests `tests/unit/ceiling_spikes_test.cpp`, scenario `tests/scenarios/solved/ceiling_spikes.txt`, play script
  `tests/play/ceiling-spikes.txt` (removed once solved).

## Tests

* Unit: a jump under ceiling spikes costs a floor crossing through the armour, under a ceiling death trap also the
  strong poison; a walk under them does not; a pouncing monster
  takes the hit; a coward does not jump there; the loader rejects spikes with no reachable underside.
* Scenario on `tests/levels/subcells`: screenshots of the spikes from below and of a jump into them.
* Play script `tests/play/ceiling-spikes.txt` (removed once solved).
* Glossary: "ceiling spikes", "ceiling death trap" (and extend "trap").
