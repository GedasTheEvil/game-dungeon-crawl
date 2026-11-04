# Ceiling spikes: the floor spike traps on a low ceiling, they hurt whoever jumps into them

Status: draft 2026-11-03. Needs first: [head-bump](solved/head-bump.md),
[monster-head-bump](monster-head-bump.draft.sonet.md). From the user.

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
* Walking, crouching ([crouch](solved/crouch.md)) and standing under it are safe.
* Sound: the floor spikes' stab, not the knock.
* Look: the floor spikes' and the death trap's models turned 180 degrees, on each underside, drawn with the sub-cell
  boxes. If a model does not fit a quarter-tile underside, scale it in its Blender 5 script
  ([modeling.md](../modeling.md), notes in [remodeling.md](../remodeling.md)). Visible from the ground, so the player
  can see it before a jump.
* Monsters: cowards (`Courage`) do not jump under it; the reckless and the ones in a pounce do (the lure). Bosses as
  their kin.
* Levels: none required. The [subcell-checker](subcell-checker.draft.opus.md) must warn if the only way on is a jump
  under spikes (forced damage). Add a few to the campaign only with [subcell-campaign](subcell-campaign.draft.opus.md).

## Tests

* Unit: a jump under ceiling spikes costs a floor crossing through the armour, under a ceiling death trap also the
  strong poison; a walk under them does not; a pouncing monster
  takes the hit; a coward does not jump there; the loader rejects spikes with no reachable underside.
* Scenario on `tests/levels/subcells`: screenshots of the spikes from below and of a jump into them.
* Play script `tests/play/ceiling-spikes.txt`.
* Glossary: "ceiling spikes", "ceiling death trap" (and extend "trap").
