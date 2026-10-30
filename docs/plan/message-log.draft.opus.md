# Message queue and log

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: HUD change, a new journal or screen section.

## Problem

The status box holds one message for 3 s (`GameState` status message, `src/state/game_state.cpp`;
`src/ui/status_box.cpp`). A new message replaces the old one. A kill posts "Gained N XP", then "Now you are level N",
then maybe a tomb-curse line (`src/world/dungeon_monsters.cpp`, kill handling), each replacing the last: level-ups and
curse warnings get lost. There is no log to read them back.

## Plan

* Queue: messages stack (up to 3 visible, newest at the bottom), each fades on its own timer. Merge repeats
  ("Gained 30 XP" twice → "Gained 60 XP"). Important lines (level up, curse, poison, heka earned) stay longer and
  have a colour.
* Log: the last ~50 messages of this level, readable on a page (a journal tab or a section of the inventory; pick
  what fits [ui.md](../ui.md)). Not saved.
* Unit tests for the queue (stacking, merge, expiry).
