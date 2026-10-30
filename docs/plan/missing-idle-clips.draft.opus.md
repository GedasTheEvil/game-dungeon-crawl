# Missing idle clips

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: Blender clips ([modeling.md](../modeling.md)).

## Problem

Without an `_idle` clip the move clip stands in (`src/entities/model_info.h`, fallback). Monsters with no idle
clip: rat, scarab, worm, plant, anubis, scorpion, egg_cluster. Standing still they walk on the spot (or the clip
pauses; check which). The giant kin on those models share the gap.

## Plan

* An idle clip per model: a breathing / twitch loop (rat sniffing, scarab antennae, scorpion tail sway, anubis
  weight shift, plant sway, worm slow coil, egg cluster pulse). Short, loops cleanly.
* Check each in the model viewer and in a screenshot scenario; no change to the boss models (out of scope for now).
