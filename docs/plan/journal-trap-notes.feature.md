# Journal: field notes for floor spikes, death traps, rock falls and ceiling traps

Status: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-13, size S, needs nothing first.

From a journal coverage sweep on 2026-11-13. Every monster kind (36 `[monster.*]` in `content/texts/monsters.txt`), item
kind (47 `[item.*]`), skill (17), curse (8 `curse-*` notes), amulet type, ring gem and zone hour has its text, and
`tests/unit/texts_test.cpp` guards the keys. Traps are the gap: [journal-field-notes-gaps](solved/journal-field-notes-gaps.feature.md)
added wall spikes, fire jets and poison drips, and `dart-traps` and `trapped-chests` exist, but these traps have no
field note (`FIELD_NOTE_SLUGS`, `src/level/journal_text.cpp`; the trap tiles are in `src/level/tile_defs.cpp`):

* floor spikes (`Spike`) and the death trap (`Death`), including the pit with a death trap in it;
* rock falls and sand falls (`RockFall`, attribute 1 is sand): only `note.amulet-blunt` mentions "falling rocks";
* ceiling spikes and the ceiling death trap (`CeilingSpikes`, `CeilingDeath`): a jump into them hurts, a walk under
  them does not ([ceiling-spikes](solved/ceiling-spikes.md)), and nothing tells the player so.

## Plan

* Write the notes in the notebook's voice (`docs/texts.md`; `content/texts/journal.txt` header says keep them in step
  with the rules): one note for floor spikes and the death trap, one for rock and sand falls (the grit that trickles
  just before the rock drops, a jump does not save you, the loose ceiling is walkable), one for ceiling traps.
* Add `FieldNote` entries at the END of the enum (saved by number, `src/level/journal.h`), their slugs to
  `AFTER_RING_SLUGS`, raise `FIELD_NOTE_COUNT`, and unlock each the first time the player is hurt by or sees the trap
  (as `WallSpikes` / `DartTraps` do; see where they are written).
* Optional, the implementer's call: skip any of the three the user finds too obvious; floor spikes are self-evident.

## Play-test

* `bin/game --play tests/play/journal-trap-notes.txt`: a god-mode hall with floor spikes, a death trap, a rock fall, a
  sand fall and ceiling spikes (the last one a jump away from the exit).
* Walk east over the spikes and the death trap, under the rock and sand falls; at the ceiling spikes walk under them
  (nothing), then jump into them. Open J, Notes: "Floor spikes", "Loose ceilings", "Ceiling spikes".
* The texts match the rules (see the notes' wording); the notes do not appear before the trap is met.

## Implemented

* Notes `FloorSpikes` (a spike or death trap touched, `updateTraps`), `RockFalls` (the player sets off a rock or sand
  fall, `startRockFall`), `CeilingTraps` (a jump hits ceiling spikes, `ceilingSpikesHit`); enum end, `AFTER_RING_SLUGS`,
  `FIELD_NOTE_COUNT` 82; texts in `journal.txt`; unit tests `tests/unit/trap_notes_test.cpp`, level `tests/levels/trap_notes`.
* Decided: all three notes written (floor spikes too, with the death trap and the pit); unlocked on being hurt or
  touched, not on merely seeing the trap (a trap in view is not a rule learnt yet).
