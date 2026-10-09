# Monsters that climb ladders

Status: draft 2026-10-09 (idea, not decided). From the user: climbers, monsters that use ladders as well.

## Today

* No monster uses a ladder. The AI (`src/entities/monster_ai.cpp`) walks a row, jumps (giant rat, giant scarab) or
  flies (bats). A ladder is the player's way up and down, and so their way out of a fight.
* The player cannot attack from the rungs ([no-attack-on-ladder](no-attack-on-ladder.draft.md)): a monster waiting at
  a ladder top gets the first hits.

## Idea

* A `MonsterKind` flag: a climber follows the player up or down a ladder (`Dungeon::PlayerOnLadder`, `LADDER_REACH`),
  with a climb clip on its model (Blender, [../remodeling.md](../remodeling.md)).
* The boss of a climbing base monster climbs too, faster (the boss ability rule in `AGENTS.md`).
* An AI change, so it carries steps 2-6 of [sim-library](sim-library.draft.md) (the monster split and the test
  harness), as decided in [sim-unit-tests-monster-rules](sim-unit-tests-monster-rules.draft.md).

## Open

* Which monsters climb: e.g. rats, scorpions, the mummy and the Anubis (hands or claws); not worms, plants, cobras or
  the crocodile. Bats fly already.
* A climber on the ladder with the player: it attacks the player on the rungs, who cannot attack back. Too harsh?
  Options: a climber cannot attack while climbing either, or it waits at the end of the ladder.
* Speed on the ladder: the player's climb speed, slower, or per monster.
* `levelcheck`: does a climber change any level's score (the player can no longer escape up a ladder)?
* Tests: unit tests on the sim harness (a climber follows up and down; a non-climber stops at the foot).
