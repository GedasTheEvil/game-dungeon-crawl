# Unit tests for the monster rules

Status: draft 2026-10-07, refined 2026-10-09: shelved until an AI change. Step 4 of [sim-unit-tests](solved/sim-unit-tests.md), left open when steps 1-3 were done.
Covered by the wider [sim-library](sim-library.draft.md) (Dungeon, monsters and the player without GL).

## Idea

The decisions in `entities/monster_ai.cpp` (`Seek`, `canSpit`, `canDive`, `canCharge`, `UpdateCharge`, `Fly`)
testable in the unit tests, without the model and the particles: the monster's sim state apart from its drawing parts.

## Today

* `monster.h` includes particles, textures and the character model, so anything that includes it needs GL headers.
  `dungeon.h` includes `monster.h`, so no `dungeon_*.cpp` can join the level library yet.
* Steps 1-3 show the way: the decor scatter moved out as plain functions on a `Tile` grid (`decor_scatter.h`), with
  what the app owns (the boss table) passed in.

## Open

* The cut: a `MonsterSim` (position, timers, state) owned by `Monster` and drawn by it, or free functions on a small
  state struct, as the scatter.
* What `MonsterLinks` gives the rules (the map, the player's box, the random stream) without the assets.
* The biggest step of the four; worth it only with a concrete AI change coming (more bosses, monster balance).

## Decided (2026-10-09)

* Not on its own. The first step of the next AI change: the giant scarab's and giant rat's leap to close the gap in
  [monster-balance](monster-balance.draft.md). The cut (sim class or free functions) is chosen then.
