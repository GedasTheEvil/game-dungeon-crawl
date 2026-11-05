# Regeneration amulet: more sources

Status: solved 2026-11-05 (play-tested by the user). Was: implemented 2026-11-04, waiting for the user's play-test. Was: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

## Problem

The Regeneration amulet has no lesser or minor tier. Chest and cursed-mimic rolls skip it (`src/world/loot.cpp`), and
no level places it (amulet ids 32, 33 appear 0 times). Only a boss drop at 1 in 16 per boss gives it: about a 50%
chance per run, the last chance on lvl54.

## Plan

* Place one Regeneration amulet in a chest in the second half of the campaign (around lvl30-40), in a guarded or
  secret spot. Level must pass `bin/levelcheck`.
* Keep it out of random chest rolls (it is strong); keep the boss drop.
* Optional: a lesser tier in random rolls, at half the effect, if the balance sim (`tools/balance`) is fine with it.

## Play-test

* `bin/game --level 37`: the chest at col 9, row 2 (a monster beside it) holds an amulet of regeneration.
* Wear it: health comes back slowly.
