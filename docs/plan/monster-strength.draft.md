# Monster strength

Status: draft 2026-10-01.

## Idea

Some monsters are too weak for their place in the campaign, others too strong. Review every monster type's threat
and readjust its stats so each one is a real fight at the levels it appears on.

Example: the **giant scarab** (levels 5-10) is slow (speed 2) and easy to kite: the player backs off, shoots it with
the bow and it never reaches them. Easy to tank in melee too. Options, not decided:

* Walk faster (3?), so the player cannot outwalk it.
* Use its leap (`Locomotion::WalkJump`) to close the gap, not only to cross pits and traps.
* More HP or damage, if speed alone is not enough.

## What

* Go through every type in `MONSTER_DEFS` (`src/state/assets.cpp`): speed, HP, damage, attack interval, XP.
  Compare against the player's level and gear at the levels it spawns on (`level_gen.cpp` picks, `docs/levels.md`).
* For each, check three player tactics: tank it in melee, kite it with the bow, run past it.
  A monster that loses to all three at its own levels is too weak.
* XP follows the new threat.
* Verify with scenarios (`tests/scenarios/`) and a playthrough.

## Related

* [weapon-ranges-and-balance.draft.md](weapon-ranges-and-balance.draft.md): its HP retune is open; do both together.
* [trap-walking-monsters.draft.md](trap-walking-monsters.draft.md), [boss-rooms.draft.md](boss-rooms.draft.md).
