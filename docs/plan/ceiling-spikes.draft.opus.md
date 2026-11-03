# Ceiling spikes: a spiked low ceiling hurts whoever jumps into it

Status: draft 2026-11-03. Needs first: [head-bump](head-bump.draft.sonet.md),
[monster-head-bump](monster-head-bump.draft.sonet.md). From the user.

A new trap: spikes on the underside of a low ceiling. Walking under it is safe; a jump into it hurts (from the user).
It makes "don't jump here" a rule the player reads off the ceiling, and a lure for jumping monsters.

## The trap

* Only on a shaped cell (`Tile::shape`) whose ceiling is low enough for a jump to reach it; from the floor a jump
  never reaches a ceiling a cell up (`movement.h`). The spikes sit on every underside of a solid sub-cell over open
  space in that cell.
* In the level format: a fixture (`Fixture`, the wall layer, like the poison drip's lump on the ceiling), with a free
  glyph. The editor places it; the level loader and the checker reject it on a cell with no reachable underside.
* A head bump under it (the player's or a monster's) is a spike hit instead of a plain bump: `SPIKE_ATTACK_MIX`
  through the armour, larger than the bump (pick it, about the larger of 15 HP and 10% of the max HP), plus the
  bump's daze roll. Any contact counts, no velocity threshold: the tips reach down.
* Walking, crouching ([crouch](crouch.draft.opus.md)) and standing under it are safe.
* Sound: a stab (reuse the wall spikes'), not the knock.
* Look: a row of bronze spikes on each underside, from the wall spikes' model (Blender 5, [modeling.md](../modeling.md),
  notes in [remodeling.md](../remodeling.md)), drawn with the sub-cell boxes. Visible from the ground, so the player
  can see it before a jump.
* Monsters: cowards (`Courage`) do not jump under it; the reckless and the ones in a pounce do (the lure). Bosses as
  their kin.
* Levels: none required. The [subcell-checker](subcell-checker.draft.opus.md) must warn if the only way on is a jump
  under spikes (forced damage). Add a few to the campaign only with [subcell-campaign](subcell-campaign.draft.opus.md).

## Tests

* Unit: a jump under spikes hurts by the spike rule through the armour, a walk under them does not; a pouncing monster
  takes the hit; a coward does not jump there; the loader rejects spikes with no reachable underside.
* Scenario on `tests/levels/subcells`: screenshots of the spikes from below and of a jump into them.
* Play script `tests/play/ceiling-spikes.txt`.
* Glossary: "ceiling spikes" (and extend "trap").
