# Fire jet trap

Status: draft 2026-10-14. Needs first: [fire-damage](fire-damage.draft.opus.md) (fire type, burning) and
[rhythm-traps](rhythm-traps.draft.opus.md) (the timed cycle, its telegraph and level check). From the user.

## Idea

A timed trap that breathes fire: a vent in the floor (or low in a wall) shoots a jet of flame on a cycle. Same clock
as the rhythm traps (`period`, `phase`, extended / retracted / telegraph), only the hazard is fire, not poisoned
spikes. Build it as a second kind of the rhythm trap (a `kind` field: spikes, fire), not a parallel system.

* **Telegraph:** the vent glows red and sparks, a hiss rising, just before the jet. Then a roar while it burns.
* **Jet:** a column of flame (floor vent) or a horizontal tongue across the corridor (wall vent), 1 to 3 cells
  long, implementer's choice per glyph or level key. Draw with `src/graphics/fire.cpp` (flame ramp). The jet is a
  light source: a dark corridor flickers on the beat, so the player sees the rhythm from afar.
* **Hit:** 100% fire (`DamageMix`), while in the zone and the jet is on (the `TrapHurt` tick, as for spikes). Fire
  ignores armour (fire-damage); the Fire Warding amulet and the trap ward (`trapCutPercent`) both cut it, one after
  the other. Every hit sets **burning**, so leaving the zone late still costs a few ticks; half water puts it out.
* **No jump, no sprint bypass:** as for rhythm traps. Jumping over a floor vent puts you into the column.
* **Monsters:** courage rules as for rhythm traps. Fire resistances apply: a reckless mummy walking through burns
  (weak, 200%), which is good: the player can lure mummies into it. Kills by the trap give no XP.
* **Sound:** hiss on telegraph, roar on the jet, audible from afar like the spikes' scrape.
* Status line on the first hit, e.g. "The floor exhales. You smell your eyebrows."

## Level format

A new glyph for the vent (pick a free one; a second for the wall vent if both exist). Phase / period / speed the
same way rhythm traps take them. Runs of vents with staggered phases are the main use.

## Level check

Reuse the rhythm-trap crossing check unchanged (the hazard kind does not matter for timing). Count fire vents with
the other rhythm traps. One extra: burning after the zone; the safe-window rule stays the same (burning is a cost,
not a block).

## Open points

* Jet length and wall vs. floor vent: one or both.
* Do both wards stack (multiply) or does the stronger win? Default: multiply.
* Placement in the campaign: which levels get vents (after fire-damage's Fire Warding amulet can drop).

## Tests

* Unit: a fire vent hit on an armoured player deals the same as on an unarmoured one; it sets burning.
* Unit: a mummy in the jet takes double damage.
* Scenario with a screenshot: the jet on, the player burning.
