# The way back from the name cartouche to its name gate

Status: draft 2026-10-30 (user). Needs nothing first.

## Problem

lvl8: the name gate (`!`, col 35 row 21 in `bin/levelcheck --map`) is passed on the way down; the name cartouche
(`:`, col 14 row 1) is next to the exit at the bottom. Between them the player drops down, so once the name is read
the gate can no longer be reached: the secret room is lost.

## Level check

In `checkNameGates` (`src/world/level_check.cpp`), next to the existing rules (plan secret-name-gates): with the name
gate sealed, the cell in front of the name gate must be reachable from the name cartouche. Walk from the cartouche
cell (a settled walker state there, like `startState`; the cartouche is only reached in a sealed walk, so take the
state(s) that walk had at its cell) with `Walker(grid, true, true)`. Warning, e.g. "the name gate at ... cannot be
reached back from the name cartouche at ...".

Unit test in `tests/unit/` (the level check tests): a tiny level with a drop between the gate and the cartouche
warns; with a ladder back up it does not.

## Fix the levels

Run `bin/levelcheck content/levels/lvl*` (25 levels have a name gate). Every flagged level, lvl8 at least: swap the
name gate and the cartouche (user). The gate is a `Door` tile, the cartouche a wall fixture: the gate goes to a floor
cell where the cartouche was (or the nearest fitting one), the cartouche onto the wall at the gate's old place.
Keep the other name gate rules: the cartouche found with the gate sealed, not next to it. Where a swap alone does not
pass, move one of them as little as needed. Edit the level files by hand (never regenerate, AGENTS.md).

## Generator

`level_gen.cpp` (named wing, `cartoucheCol`): pick the cartouche spot only where the gate's wing is reachable back
from it, so a new generation passes the check too. Optional if the generator has no cheap reachability at that point:
then the levelgen output simply gets the new warning and the spot is re-rolled.

## Done when

`make check` passes, `bin/levelcheck content/levels/lvl*` has no warnings. Play-test: `bin/game --level 8`, read the
cartouche, walk back to the name gate and open it.
