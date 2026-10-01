# Monster strength

Status: draft 2026-10-01.

## Idea

Some monsters are too weak for their place in the campaign, others too strong. Review every monster type's threat
and readjust its stats so each one is a real fight at the levels it appears on.

Example: the **giant scarab** (levels 5-10) is slow (speed 2) and easy to kite: the player backs off, shoots it with
the bow and it never reaches them. Easy to tank in melee too. Decided 2026-10-01 (numbers refined when the work
starts):

* Faster: it walks faster (3?), so the player cannot outwalk it.
* Leaps more: it uses its leap (`Locomotion::WalkJump`) to close the gap on the player whenever it can, not only to
  cross pits and traps.
* More HP or damage only if speed and the leap are not enough.

Do it after the walk speed change ([fixed-timestep.draft.md](fixed-timestep.draft.md), 1.0 tiles/s): monster speeds
are only meaningful against the final player speed.

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
