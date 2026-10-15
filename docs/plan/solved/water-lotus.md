# Water lotus

Status: solved 2026-10-15 (play-tested), implemented 2026-10-14. From the user, after a lore review: a man-eating plant fits
a tomb better in the water. A new monster; the land plant stays (see [plant-poison](plant-poison.md)).

## Idea

A **blue lotus** (Nymphaea caerulea, Nefertem's flower) rooted in half water. Idle, it is a flower floating on the
water, like decor. When the player comes near, it rears up and bites. The blue lotus was a sedative: its bite drains
stamina, so the wading player gets slower still and cannot sprint away.

* Locomotion: stationary (rooted) with a lurk, like the mimic's ambush: a floating flower until the player comes near.
* Placement: only in half water cells. The checker warns otherwise, as it does for the crocodile
  (`level_check.cpp`, "is not in or next to water"; this one needs the cell itself to be half water).
* Bite: damage plus a stamina drain (default: 25% of max stamina per bite; tune in play).
* Stats: between the plant and the crocodile (HP, damage, XP, `threat`). Poison immune, like the plant.
* Levels: the flooded levels, from the first one with half water. `generated` range accordingly; a few placed by
  hand in levels 1-5 only if they have half water.
* Glyph: pick a free one.
* No boss has it as kin; no boss change.

## Journal

Note in the archaeologist's voice, humour welcome (lotus-eaters, Odyssey): e.g. "Pretty, blue, and it wants my leg.
One bite and I could sleep for a week."

## Model

Procedural Blender script per [docs/remodeling.md](../../remodeling.md): flower on a pad, a fanged mouth in the
petals; clips: idle (floating, a slight sway and a twitch now and then: the only tell against the decor lotus pads, user decision), rise, bite, death (sinks). Drawn at the water surface.

## Tests

* Unit / sim: stays rooted, wakes in range, a bite drains stamina, only spawns in half water.
* Scenario with a screenshot: idle on the water, reared up.
* `bin/levelcheck content/levels/lvl*`: no warnings.
* Glossary row for the new terms (the kind, the stamina drain).

## Related

* [water-decorations](water-decorations.md): decor lotus pads to hide this one among.

## Implemented (2026-10-14)

* `MonsterLotus` (24, glyph `d`, editor label `lotus`, game name "Blue lotus") in `KINDS` (`src/world/monster_kinds.cpp`):
  50 HP, bite 7 every 0.9 s (fangs: 70% pierce, 30% slash), 1300 XP, threat 2, poison immune; weak to slashing, resists
  blows and fire (wet), tough against points. `staminaDrainPct = 25`: each bite that lands (not dodged) takes 25% of max
  stamina, in god mode too (`Monster::bite`); stamina comes back at 5% a second, so a lotus biting every 0.9 s keeps the
  player drained.
* `Locomotion::Floating`: rooted (as the plant, bites at `ROOTED_BITE_REACH`), lurks until the player comes within
  `FLOATING_WAKE_RANGE` (1.2 cells) of its centre on its row, then plays its rise clip (`riseMs` 600). Biting, it turns its
  head to the player. Drawn at the water surface: `Monster::floatLift` lifts it from the basin floor to the surface.
  Clips: `FLOATING_CLIPS` (`src/entities/model_info.h`).
* Journal: two new moves, `CreatureMove::Bloom` ("Floats among the lotus flowers, rears up when I come near.") and
  `CreatureMove::Sedate` ("Its bite makes me drowsy, too tired to run or jump."); the note in the draft's words plus the
  Odyssey joke.
* Checker: "the lotus at ... is not in half water" (`checkWater`, `src/world/level_check.cpp`).
* Model: `tools/blender/models/lotus.py` (per-frame rebuilt geometry, no rig), sounds `tools/audio/lotus_sounds.py`; see
  [remodeling.md](../../remodeling.md). Idle it matches the decor `water_lotus`; the only tell is a twitch of the jaws once in
  its 8 s idle loop. Drawn at 0.7 of the modelled size (`scale` 12), so its reared head meets the wading player's face.
* Levels (30 in all): one in 7, 8, 9, 19, 20, 27, 28 and 30, two in 16, 17, 18, 25 and 29, three in 21-24. In the
  longer water stretches, at least 3 cells from other monsters where the pool has room (levels 7 and 9: 2 cells from their
  crocodile, the pools are small). Level 25 got two, not three: with three it ranked above level 26. Levels 1-5 have no
  pool (lvl1's one half water cell is a lone puddle), so none there. `generated` stays empty: levelgen makes no water.
* Tests: `tests/unit/sim_test.cpp` ("the blue lotus floats until ..."), scenario `tests/scenarios/solved/lotus.txt` (floating
  beside a decor lotus, rise, bite, toon, side view, death), test level `tests/levels/lotus`.

Play-test: `bin/game --play tests/play/water-lotus.txt`, or `bin/game --level 8` (the lotus at the east end of the
flooded hall).
