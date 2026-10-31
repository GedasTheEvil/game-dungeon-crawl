# Missing idle clips

Status: implemented 2026-10-31, waiting for a play-test. Draft 2026-10-30, from the project audit of 2026-10-30.

Opus: Blender clips ([modeling.md](../modeling.md)).

## Problem

Without an `_idle` clip the move clip stands in (`src/entities/model_info.h`, fallback). Monsters with no idle
clip: rat, scarab, worm, plant, anubis, scorpion, egg_cluster. Standing still they walk on the spot (or the clip
pauses; check which). The giant kin on those models share the gap.

## Plan

* An idle clip per model: a breathing / twitch loop (rat sniffing, scarab antennae, scorpion tail sway, anubis
  weight shift, plant sway, worm slow coil, egg cluster pulse). Short, loops cleanly.
* Check each in the model viewer and in a screenshot scenario; no change to the boss models (out of scope for now).

## Implemented

* The fallback checked: without `_idle` the walk clip stood in and kept playing (`ModelInfo::Shown`), so a blocked
  walker walked on the spot.
* An `_idle` per model, built in its script (`-- --export-clip _idle` writes it alone): rat 32 (sniffing), scarab 32
  (antennae twitching apart, the elytra lifting a crack), worm 40 (slow coil, jaws tasting), plant 40 (dormant: bloom
  half closed, nodding), Anubis 36 (weight shift, feet planted, head turning), scorpion 40 (tail sway, claws opening),
  egg cluster 40 (dormant: one slow pulse). Notes: [remodeling.md](../remodeling.md#model-notes). The giant kin and the
  bosses on these files share them (the boss models themselves untouched).
* Engine: the plant and the egg cluster had their walk file as an idle loop already and never stood idle. They now
  start dormant on `_idle` and stay on it on the player's row until they have bitten or been hit (`Monster::Seek`),
  then show the walk file as their awake loop.
* Tests: `model_info_test` (every monster model has `_idle`), `clip_state_test` (tests/levels/idle_clips: seven
  monsters walled off on the player's row stand on their own idle clip). Screenshots: `tests/scenarios/missing_idle_clips.txt`
  (the dormant plant); review renders and model-viewer shots of each clip checked by eye (floor contact, loop seam,
  no foot slide).
* Play-test: `bin/game --play tests/play/missing-idle-clips.txt`.
