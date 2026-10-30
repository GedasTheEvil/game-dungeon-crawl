# Traps hit the player's box

Status: implemented 2026-10-30, waiting for a play-test: `bin/game --play tests/play/trap-player-box.txt`. From the
project audit of 2026-10-30.

## Problem

Spike and death traps test the player's centre point: `Dungeon::updateTraps` calls `inTrap(mapX, mapY)`
(`src/world/dungeon_base.cpp`). Only the fire jet uses `HalfWidth`. Left out on purpose in
[trap-and-font-bugs](solved/trap-and-font-bugs.md), [monster-hitboxes](solved/monster-hitboxes.md) and
[monster-balance](solved/monster-balance.md). A player can stand half on spikes unharmed; visually it reads as a bug.

## Plan

* Spikes, pits and death traps test the player's box (feet span, `HalfWidth`), with a small forgiveness margin
  (e.g. 25% of the width) so grazing an edge doesn't kill.
* Same rule for monsters that walk into traps (reckless walkers, [trap-walking-monsters](solved/trap-walking-monsters.md)).
* Re-check the levels: a jump that only worked through the point test must still be possible (`bin/levelcheck`,
  the climb / jump reach model). Unit tests for the edge cases.

## Done

* `Dungeon::inTrap` takes a half width: the player's box, less `TRAP_BOX_FORGIVE` (25%) of it on each side. The
  crossing's bell (TrapBell) spans the widened hitbox, so a whole crossing still deals the same.
* Reckless walkers touch traps with their box too; the other walkers keep the centre test, or a death trap (whose
  hitbox reaches 0.3 tiles into the next cells) would stab them standing beside it.
* The fire jet already used the full box; unchanged.
* Levels: `bin/levelcheck content/levels/lvl*` clean (the checker models traps per cell). Unit tests:
  `tests/unit/trap_box_test.cpp`; the edge step in `trap_bell_test.cpp` moved 0.04 left to keep its depth.
