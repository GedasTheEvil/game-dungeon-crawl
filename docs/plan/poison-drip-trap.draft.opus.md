# Poison drip trap

Status: draft 2026-10-14. Needs first: [rhythm-traps](solved/rhythm-traps.md) (the timed cycle, its telegraph
and level check). From the user. Sibling of [fire-jet-trap](fire-jet-trap.md).

## Idea

A timing trap and a floor trap at once. Venom has dripped from the ceiling for ages: a green puddle lies on the floor
from the start and never dries. Above it, a lump of venom swells and falls into the puddle on a cycle. The puddle is
always dangerous; the drop is dangerous on the beat.

Build it as a third kind of the rhythm trap (spikes, fire, drip) for the drop, plus a permanent floor-trap part for
the puddle, not a parallel system. The rhythm-trap spikes are already poisoned; the drip differs in shape: one
cell, from above, on a floor that is never safe.

* **Puddle (floor trap, always on):** stepping into it poisons (weak tier,
  [stronger-poisons](solved/stronger-poisons.md)), applied once per entry; re-entry refreshes as `Player::Poison`
  does. No damage. Present from level load, never fades.
* **Drop (timing trap):** on the cycle (`period`, `phase`), the lump falls onto the puddle cell. Whoever is there
  takes a small hit plus medium poison (stronger than the puddle's weak). Resistance rolls as for any poisoning. The trap
  ward (`trapCutPercent`) cuts the damage only, never the poison (as
  [death-spikes-poison](solved/death-spikes-poison.md)).
* **Telegraph:** the lump grows and wobbles, a wet creak, a last drop or two just before it falls.
* **Bypass:** jumping or levitation clears the puddle, like any floor trap, but not the drop: a jump timed into
  the falling lump is still a hit. So the safe pass is a jump between drops. Walking through always costs the weak
  poison; walking through on the beat costs the drop too. Sprint only shortens the time under the drop.
* **Monsters:** cowards treat the puddle as a trap tile (stop at its edge, walk-jumpers leap it, `Courage`);
  reckless ones walk in and get poisoned (`Monster::TakePoison`, byPlayer false: no XP for the kill). Flyers
  (weight 0) ignore the puddle, not the drop.
* **Draw:** a flat, glossy green puddle decal with a ring ripple from each small drip; the lump on the ceiling with
  a dark stain around it; a green splash on the big drop (`Fire::VENOM_RAMP` colours or a green blob). Map symbol
  like other traps.
* **Sound:** constant faint drip loop, wet creak on the telegraph, splat on the drop.
* Status line on the first drop hit, e.g. "Something on the ceiling has been saving that for you."

## Level format

A new glyph for the drip cell (pick a free one): puddle and lump on the same cell. Phase / period / speed the same
way rhythm traps take them. Main use: a row of drips across a corridor or a hall, staggered, so the player must jump
over each puddle between drops.

## Level check

* The puddle counts as a floor trap (like `spikes` / `pathSpikes`): jumpable, never a softlock.
* The drop uses the rhythm-trap crossing check (walk speed; a jump across must fit a safe window). Warn if no
  start time gets across without a drop hit. Count drips with the other rhythm traps.
* Extend the checker's antidote rule (`level_check.cpp`, today poisoner monsters only, `isPoisoner`) to drip
  cells: a level with drips needs an antidote.

## Open points (implementer's choice, user 2026-10-14)

* Drop damage size; maybe zero, poison only.

## Decided

* Puddle: one cell (user, 2026-10-14).
* Campaign placement: not here, in [denser-levels](denser-levels.draft.opus.md) if this lands first; if
  denser-levels is already done, place a few drip runs in existing levels here.

## Tests

* Unit: the puddle poisons on entry at any time; the drop hits only on the beat; a jump clears the puddle, not the
  drop.
* Unit: the checker counts the puddle as a floor trap and warns on an uncrossable drip run.
* Scenario with a screenshot: the lump falling into the puddle, the player poisoned.
