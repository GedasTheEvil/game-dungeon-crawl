# Missing monster sounds

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: sound design with the synth scripts in `tools/audio/` (one per monster).

## Problems

`CharacterModel::loadSounds` (`src/entities/character_model.cpp`) skips missing files without a log line, so these
play nothing:

* Plant: no `plant_*.wav` at all; bite and death are silent.
* Sand worm surfacing: the burrow code plays the wake sound (`src/entities/monster_burrow.cpp`); no `worm_wake.wav`
  (Rerek has his own).
* Giant scorpion / scorpion queen grab tell: `monster_grab.cpp` plays the wake sound; no `scorpion_wake.wav`.
* Giant scarab / Khepri leap: `monster_leap.cpp` plays the jump sound; no `scarab_jump.wav`.

Stand-ins: the dodge sound is `items/sling_swing.wav` (`src/state/assets.cpp`); the dagger uses `sword_swing` +
`spear_hit`, axes and mace use `club_swing`, the javelin `spear_swing`.

## Plan

* Synthesize: `plant_att`, `plant_die` (and `plant_wake` if it has a wake), `worm_wake` (sand bursting, low
  rumble), `scorpion_wake` (chitin clatter, a hiss), `scarab_jump` (wing-case snap, buzz). Add them to the existing
  scripts or a new `plant_sounds.py`.
* A dodge whoosh of its own; own swings for the dagger (short, light) and the axes (heavier than the sword).
* Log one warning per missing *expected* clip: a clip is expected when the code can play it for that monster type
  (an ability uses it). Check the game log stays clean afterwards.
* Bosses keep their kin's voices here (bosses are out of scope for now).
* Optional cleanup: rename `potion_aphethamine.png` / `potion_stone_skin.png` to the Might / Armor names.
