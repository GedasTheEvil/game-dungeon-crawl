# Poison drip trap

Status: implemented 2026-10-25, waiting for the user's play-test: `bin/game --play tests/play/poison-drip-trap.txt`. Draft 2026-10-14. Needs first: [rhythm-traps](rhythm-traps.md) (the timed cycle, its telegraph
and level check). From the user. Sibling of [fire-jet-trap](fire-jet-trap.md).

## Idea

A timing trap and a floor trap at once. Venom has dripped from the ceiling for ages: a green puddle lies on the floor
from the start and never dries. Above it, a lump of venom swells and falls into the puddle on a cycle. The puddle is
always dangerous; the drop is dangerous on the beat.

Build it as a third kind of the rhythm trap (spikes, fire, drip) for the drop, plus a permanent floor-trap part for
the puddle, not a parallel system. The rhythm-trap spikes are already poisoned; the drip differs in shape: one
cell, from above, on a floor that is never safe.

* **Puddle (floor trap, always on):** stepping into it poisons (weak tier,
  [stronger-poisons](stronger-poisons.md)), applied once per entry; re-entry refreshes as `Player::Poison`
  does. No damage. Present from level load, never fades.
* **Drop (timing trap):** on the cycle (`period`, `phase`), the lump falls onto the puddle cell. Whoever is there
  takes a small hit plus medium poison (stronger than the puddle's weak). Resistance rolls as for any poisoning. The trap
  ward (`trapCutPercent`) cuts the damage only, never the poison (as
  [death-spikes-poison](death-spikes-poison.md)).
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

## Decided

* Puddle: one cell (user, 2026-10-14).
* Drop: a small hit plus medium poison, as above, not poison only (user, 2026-10-22). The size is tuned with the
  other traps.
* Campaign placement: not here, in [denser-levels](../denser-levels.draft.opus.md) if this lands first; if
  denser-levels is already done, place a few drip runs in existing levels here.

## Tests

* Unit: the puddle poisons on entry at any time; the drop hits only on the beat; a jump clears the puddle, not the
  drop.
* Unit: the checker counts the puddle as a floor trap and warns on an uncrossable drip run.
* Scenario with a screenshot: the lump falling into the puddle, the player poisoned.

## Implemented (2026-10-25)

Play-test: `bin/game --play tests/play/poison-drip-trap.txt` (tests/levels/poison_drips).

* **Kind:** a wall type of its own, `PoisonDrip` (23), glyph `,`, on the wall spikes' cycle and encoding (attribute =
  speed, value = phase | live state; `isRhythmTrap`, `Dungeon::updateRhythmTraps`). In the wall layer though it hangs
  from the ceiling: that layer carries the cycle. The drop's time out is the spikes' (1.2 s of 4 s normal): the lump
  falls in 0.18 s, then the venom pours after it, thinning, till it ends (`dripFalling`). The telegraph is the
  spikes' last 0.6 s.
* **Puddle:** weak poison once per entry, no damage, when the body's middle is within `DRIP_PUDDLE_HALF_WIDTH`
  (0.35) of the cell's and the player is on the ground (not jumping or falling). No levitation exists in the game yet:
  only the jump clears it.
* **Drop:** whoever's body overlaps the cell while it falls, a jump too: `DRIP_DROP_DAMAGE` (8, blunt
  `DRIP_DROP_MIX`, so armour helps; the trap ward cuts it) and medium poison, once per entry. The poison comes even
  when the ward takes the whole hit. Status on each drop hit: "Something on the ceiling has been saving that for
  you." No dodge.
* **Monsters:** cowards stop at the puddle (`walkerBlockedAt`: always a trap), walk-jumpers leap it, the reckless walk
  in. `Monster::StandInDrip` once a tick: the puddle (on the ground, not flyers or leapers in the air) and the drop
  (all but the hidden), each once per entry, the drop as a trap's hit (`TrapHit`, `trapDamagePct`); poison not the
  player's: no XP.
* **Look and sound:** drawn in GL, no model (`dungeon_render_drips.cpp`): a dark stain on the ceiling, the lump
  swelling in it on a neck, wobbling and sagging in the telegraph, falling, a green splash (`Fire::VENOM_RAMP`) and a
  ring, the stream after it; a glossy puddle (a sheen spot) with ripples from three small drops between drops and two
  in the telegraph; a faint green light off it. `drip_plips.wav` (the small drops, at the start of the safe part),
  `drip_creak.wav` (the telegraph, the last two plips), `drip_splat.wav` (tools/audio/mechanism_sounds.py), heard in
  view. Map: the puddle, the drop and its fall. Editor hints.
* **Checker:** the puddle is a floor trap (`COST_DRIP_PUDDLE` on the path, jumped over like spikes); the cell is a
  rhythm trap in the run crossing check (mixed with the other kinds), path cost and difficulty as wall spikes;
  `poisonDrips` / `pathPoisonDrips`; warnings: no floor under it, no ceiling over it, in water, an object in the
  puddle (but a key or a page), drips without an antidote in reach.
* **Tests:** `tests/unit/poison_drip_test.cpp` (checker, puddle on entry at any time and once per entry, the drop on
  the beat only, a jump clears the puddle not the drop, the trap ward, monsters, no XP);
  `tests/scenarios/solved/poison_drips.txt` (screenshots: the cycle, the player poisoned in it).

Not done:

* Campaign placement: left to denser-levels, as decided.
* No boss ability: no monster gains one here.
