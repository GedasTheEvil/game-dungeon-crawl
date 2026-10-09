# Monsters that climb ladders

Status: draft 2026-10-09, refined 2026-10-09 (decided, not implemented). From the user: climbers, monsters that use ladders as well.

## Today

* No monster uses a ladder. The AI (`src/entities/monster_ai.cpp`) walks a row, jumps (giant rat, giant scarab) or
  flies (bats). A ladder is the player's way up and down, and so their way out of a fight.
* The player cannot attack from the rungs ([no-attack-on-ladder](no-attack-on-ladder.draft.md)): a monster waiting at
  a ladder top gets the first hits.

## Idea

* A `MonsterKind` flag: a climber follows the player up or down a ladder (`Dungeon::PlayerOnLadder`, `LADDER_REACH`),
  with a climb clip on its model (Blender, [../remodeling.md](../remodeling.md)).
* An AI change, so it carries steps 2-6 of [sim-library](sim-library.draft.md) (the monster split and the test
  harness), as decided there.

## Decided (2026-10-09)

* **Climbers:** the rat, the giant rat, the scorpion, the giant scorpion, the mummy and the Anubis (hands or claws).
  Not worms, plants, cobras or the crocodile; bats fly already.
* **Bosses:** the bosses of climbers (the Scorpion Queen, the Anubis boss) can climb too, faster (the boss ability
  rule in `AGENTS.md`). Their arenas rarely give them a ladder, but the AI must not rule it out.
* **Checker score:** a climber's `threat` goes up by 5% (x1.05), bosses too (the player can no longer escape up a
  ladder). The same share for every climber: the rat 0.7 → 0.735, the giant rat 3 → 3.15, the Anubis 8 → 8.4, the
  Anubis boss 15 → 15.75.
  Re-run `./levelcheck levels/lvl*` and fix the levels it flags.
* **No attacks on the rungs, for everyone:** a climber cannot attack while it climbs, as the player cannot
  ([no-attack-on-ladder](no-attack-on-ladder.draft.md)). Depends on that draft: it ships first, or with this one. A
  climber on the ladder only follows; it attacks once off the rungs.
* **Climb speed:** 80% of the monster's own walk `speed`. A boss walks faster, so it climbs faster too. Comes after
  [anubis-speed](anubis-speed.draft.md) (the Anubis guard walks faster, so it climbs faster).

* **Path-finding:** a climber that has seen the player (woken, chasing as in `Seek` today) keeps a path to the
  player's cell across floors: walkable rows, ladders and, for jumpers (the giant rat), planned jumps over gaps.
  Before it has seen the player it idles as today. Non-climbers keep today's AI.
* **Range:** capped. Starting values, tuned in play: a path of at most 20 cells; out of view for 5 s, it gives up
  and stays where it is.
* **Mid-climb:** if the player steps off and walks away, the climber finishes the climb to the end it was heading
  for, steps off, and chases on that floor (a new path).
* **Clips:** a new climb clip per climber model, in Blender. The bosses share their kin's model and clip.

## Open (for the implementer)

* The path search: on the map grid (`Level`), so it can live in the lib and be unit tested on its own. Replan when
  the player's cell changes, not every tick.
* Tests: unit tests on the sim harness (a climber follows up and down; a non-climber stops at the foot; a giant rat
  plans a jump; it gives up past the cap).
