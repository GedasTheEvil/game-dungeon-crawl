# Message queue and log

Status: solved 2026-10-31 (play-tested by the user). Was: implemented 2026-10-30, waiting for a play-test: `bin/game --play tests/play/message-log.txt`. From the project audit of 2026-10-30.

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
  what fits [ui.md](../../ui.md)). Not saved.
* Unit tests for the queue (stacking, merge, expiry).

## Done

* `MessageLog` (`src/ui/message_log.h`): up to 3 lines stacked, oldest at the top, each on its own timer. A line told
  again while it shows merges and moves to the bottom: XP gains add up, others count "(x2)". `StatusTone` on the
  world's `Status` event: good news (level up, heka taken, a curse lifted) in green, dangers (curse, poison) in red,
  6 s instead of 3.
* The tomb curse no longer repeats the curse's line inside the execration's: both now show, stacked.
* Log: the journal's section L, this level's last 50 lines, newest first; cleared on a new level, a new game and a
  load. Not saved.
* Tests: `tests/unit/message_log_test.cpp` (stacking, expiry, merges, log size, level change); screenshot scenario
  `tests/scenarios/solved/message-log.txt`.
* Play-test fix 2026-10-31: the level banner (shown again after a load) sat at a fixed height and the stacked lines
  ran over it. It now goes under the stack (`StatusBox::draw` returns where the stack ends) and only moves down while
  it shows. Scenario `tests/scenarios/solved/message-log-banner.txt`.
