# Fire jet trap

Status: implemented 2026-10-22, waiting for a play-test (see [Implemented](#implemented-2026-10-22)); draft 2026-10-14. Needs first: [fire-damage](solved/fire-damage.md) (fire type, burning) and
[rhythm-traps](solved/rhythm-traps.md) (the timed cycle, its telegraph and level check). From the user.

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

## Open points (implementer's choice, user 2026-10-14)

* Jet length and wall vs. floor vent: one or both.
* Do both wards stack (multiply) or does the stronger win? Default: multiply.

## Decided

* Campaign placement: not here, in [denser-levels](denser-levels.draft.opus.md) if this lands first (vents
  only on levels where the Fire Warding amulet can drop); if denser-levels is already done, place a few vent runs
  in existing levels here.

## Tests

* Unit: a fire vent hit on an armoured player deals the same as on an unarmoured one; it sets burning.
* Unit: a mummy in the jet takes double damage.
* Scenario with a screenshot: the jet on, the player burning.

## Implemented (2026-10-22)

Play-test: `bin/game --play tests/play/fire-jet-trap.txt` (tests/levels/fire_vents).

* **Kind:** a tile type of its own, `FireVent` (22), glyph `+`, a wall type (fixture) beside `WallSpikes`, not a kind
  field in it: the wall spikes' checks (solid when out, stab, shove) stay theirs. The two share the cycle and its
  encoding (`spike_cycle.h`, attribute = speed, value = phase | live state), `isRhythmTrap`, the clock update
  (`Dungeon::updateRhythmTraps`) and the checker's crossing walk.
* **Open points:** one vent kind, low in the back wall; the jet fills its one cell from the floor to the ceiling.
  The wards multiply: the trap ward cuts the damage (`TrapDamage`), then the Fire Warding amulet's fire resistance.
* **Hit:** `FIRE_JET_MIX` (all fire, through armour) at the `TrapHurt` tick's pace, no dodge (sustained). After the
  user's first play-test (2026-10-22: under 10 damage for a jump, no threat at 1000 HP) it scales as strong poison:
  25% of max HP a second, at least 15 (`FIRE_JET_PERCENT_PER_SECOND`, `FIRE_JET_HP_PER_SECOND`), and burning as
  from a second's fire (5% of max HP a second for 3 s). Standing through a whole jet costs about 45% of max HP, a
  jump through about 30%. Monsters take the floor only, so no boss melts in it. The jet blocks nothing; a jump does not clear it. Status on entry: "The wall exhales. You smell
  your eyebrows."
* **Monsters:** cowards stop at a glowing or burning vent (`walkerBlockedAt`), the reckless walk through. All
  monsters in the cell are hit, flyers and leapers too (not the hidden ones): `Monster::StandInFire`, its fire
  resistance on top of `trapDamagePct` (a mummy: double). A trap's fire kill gives no XP: the new
  `Monster::burnByPlayer` (a weapon's or a sunbeam's fire stays the player's).
* **Look and sound:** `fire_vent` in `mechanism.py` (sandstone hood, bronze grille, embers, scorched floor); the jet
  is `Fire::draw` plumes rolling out and up (`drawFireJets`), embers in the mouth in the telegraph, a light on the
  beat (`addFireVentLights`). `fire_hiss.wav`, `fire_roar.wav` (`tools/audio/mechanism_sounds.py`), heard in view.
  Map: the mouth and a tongue of flame. Editor hints.
* **Checker:** `fireVents` / `pathFireVents`, path cost and difficulty as wall spikes; a run of rhythm traps (both
  kinds mixed) must be walkable both ways; a vent needs a floor under it and no water.
* **Tests:** `tests/unit/fire_vent_test.cpp` (checker, armour, burning, jump, damage by max HP, wards, the mummy, no XP);
  `tests/scenarios/fire_vents.txt` (screenshots: glow, jet, the player burning in it).

Not done:

* Campaign placement: left to denser-levels, as decided.
* Fire in water: a vent in half water is warned about, not modelled (see fire-in-water).
