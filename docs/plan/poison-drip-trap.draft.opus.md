# Poison drip trap

Status: draft 2026-10-14. Needs first: [rhythm-traps](rhythm-traps.draft.opus.md) (the timed cycle, its telegraph
and level check). From the user. Sibling of [fire-jet-trap](fire-jet-trap.draft.opus.md).

## Idea

A timed trap from above: a lump of venom swells on the ceiling, falls on a cycle onto one cell and leaves a green
puddle that stays a few seconds. Same clock as the rhythm traps (`period`, `phase`), as a third kind of the rhythm
trap (spikes, fire, drip), not a parallel system. The rhythm-trap spikes are already poisoned; the drip differs in
shape: one cell, and the danger lingers after the beat.

* **Telegraph:** the lump grows and wobbles on the ceiling, a wet creak, a last drop or two just before it falls.
* **Drop:** falls onto its cell. A hit on whoever stands there: small damage plus poison (tier per trap, default
  medium, [stronger-poisons](solved/stronger-poisons.md) tiers; resistance rolls as for any poisoning). The trap ward
  (`trapCutPercent`) cuts the damage only, never the poison (as [death-spikes-poison](solved/death-spikes-poison.md)).
* **Puddle:** after the drop, a puddle on the cell for `puddle` ms (default about half the period, per trap). Standing
  in it poisons (weak tier, applied once per entry, re-entry refreshes as `Player::Poison` does), no damage. It
  shrinks and fades before it goes.
* **Bypass:** sprint helps against the drop (one cell, short moment) but the puddle stays; jumping clears the
  puddle (it is on the floor), not the drop (a jump into the falling lump is still a hit). Levitation clears the
  puddle too. This is the intended difference from spikes and fire.
* **Monsters:** courage rules as for rhythm traps; cowards also stop at a puddle. Reckless ones get poisoned
  (`Monster::TakePoison`, byPlayer false: no XP for the kill). Flyers (weight 0) ignore the puddle, not the drop.
* **Draw:** the lump and the drop in venom green (`Fire::VENOM_RAMP` look, or a glossy green blob), a green splash on
  impact, a flat puddle decal. A dark stain on the ceiling marks the spot even between drops.
* **Sound:** wet creak on telegraph, splat on impact, a faint drip loop from afar.
* Status line on the first hit, e.g. "Something on the ceiling has been saving that for you."

## Level format

A new glyph for the drip cell (pick a free one). Phase / period / speed the same way rhythm traps take them; an
optional puddle time. Main use: a row of drips across a corridor or a hall, staggered, so the safe cells shift.

## Level check

The rhythm-trap crossing check, with the danger window of a drip cell = drop moment + puddle time (walk speed, no
jump). Warn if no start time crosses the run. Count drips with the other rhythm traps. Poisoners need an antidote
reachable like other poison sources (the checker's poisoner antidote rule).

## Open points

* Puddle time and whether puddles spread to a neighbour cell on a big drop (default: no).
* Drop damage size; maybe zero, poison only.
* Placement in the campaign.

## Tests

* Unit: the drip cell is dangerous at the drop and through the puddle time, safe after; a jump clears the puddle,
  not the drop.
* Unit: the checker counts the puddle time in the danger window.
* Scenario with a screenshot: the lump falling, a puddle, the player poisoned.
