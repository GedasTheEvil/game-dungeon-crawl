# Water lotus

Status: draft 2026-10-10, ready (needs nothing first). From the user, after a lore review: a man-eating plant fits
a tomb better in the water. A new monster; the land plant stays (see [plant-poison](plant-poison.draft.md)).

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

Procedural Blender script per [docs/remodeling.md](../remodeling.md): flower on a pad, a fanged mouth in the
petals; clips: idle (floating, a slight sway), rise, bite, death (sinks). Drawn at the water surface.

## Tests

* Unit / sim: stays rooted, wakes in range, a bite drains stamina, only spawns in half water.
* Scenario with a screenshot: idle on the water, reared up.
* `./levelcheck levels/lvl*`: no warnings.
* Glossary row for the new terms (the kind, the stamina drain).
