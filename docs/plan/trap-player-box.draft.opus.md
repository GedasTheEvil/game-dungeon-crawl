# Traps hit the player's box

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

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
