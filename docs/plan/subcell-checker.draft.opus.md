# Sub-cells: the level checker

Status: draft 2026-11-02. Needs first: [crouch](crouch.draft.opus.md) and
[subcell-monsters](subcell-monsters.md). From the user.

The checker's walker (`src/world/level_check.cpp`) moves by whole cells; it learns the sub-cell moves.

* Edges: up a quarter step by a jump, down by walking off, through a crawl space crouched (slower, a higher cost), a
  one-cell gap jumped from a step's height as from the floor (the reach from `Jump::ARC`).
* Monster reach: a large monster cannot follow through a crawl space; a spawn is cut off where its kind cannot pass.
* New warnings:
  * a rise of 2 or more quarters with no ladder on the only way (unreachable);
  * a ladder, gate, door or treasure in a cell without full head room;
  * a placed decor or decal that does not fit the shape;
  * a step narrower than the player's body (standing on it is not possible);
  * a shape in a `Wall` or `DeepWater` cell;
  * a crawl space that ends at a drop or a trap the player cannot see crouched (open point: drop this one if noisy).
* Docs: the sub-cell rules in [levels.md](../levels.md), next to the water and sand rules.
* Every campaign level still passes with no warnings (none has shapes yet); `tests/levels/subcells` passes.
* Unit tests for each edge and warning.
