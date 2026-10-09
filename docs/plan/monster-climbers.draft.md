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
* An AI change, so it carries steps 2-6 of [sim-library](sim-library.draft.md) (the monster split and the test
  harness), as decided in [sim-unit-tests-monster-rules](sim-unit-tests-monster-rules.draft.md).

## Decided (2026-10-09)

* **Climbers:** the rat, the giant rat, the scorpion, the giant scorpion, the mummy and the Anubis (hands or claws).
  Not worms, plants, cobras or the crocodile; bats fly already.
* **Bosses:** the bosses of climbers (the Scorpion Queen, the Anubis boss) can climb too, faster (the boss ability
  rule in `AGENTS.md`). Their arenas rarely give them a ladder, but the AI must not rule it out.
* **Checker score:** a climber's `threat` goes up by 1 (the player can no longer escape up a ladder). Bosses too.
  Re-run `./levelcheck levels/lvl*` and fix the levels it flags.

## Open

* A climber on the ladder with the player: it attacks the player on the rungs, who cannot attack back. Too harsh?
  Options: a climber cannot attack while climbing either, or it waits at the end of the ladder.
* Speed on the ladder: the player's climb speed, slower, or per monster.
* Tests: unit tests on the sim harness (a climber follows up and down; a non-climber stops at the foot).
