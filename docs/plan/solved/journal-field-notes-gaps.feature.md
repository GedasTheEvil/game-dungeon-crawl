# Journal: missing field notes

Status: solved 2026-11-05 (play-tested by the user). Was: implemented 2026-11-04, waiting for the user's play-test. Was: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

## Problems

* No field notes for wall spikes (rhythm traps), poison drips or fire jets. `content/texts/journal.txt` has only
  `note.dart-traps` and `note.trapped-chests` for traps ([rhythm-traps](rhythm-traps.md) left it).
* No field notes for the skills Dodge, Power hit, Flaming arrows and Words of Thoth; Stun, Sharp eye, Serket's
  favour, Favour of Hapi and Blessing of Isis have one (`FIELD_NOTE_SLUGS`, `src/world/journal_text.cpp`).
* Two move texts the code never shows (`content/texts/moves.txt`): `[move.curse] touch:` (`curseMoveNote` builds keys
  from curse names only) and `[move.summon] wall` (the only wall summoner has `smallMinion`, so "wall-servants" is
  always picked).

## Plan

* Write the seven notes in the notebook's voice ([texts.md](../../texts.md)), unlocked the same way as the existing ones
  (first time seen / learned). Add the slugs to `FIELD_NOTE_SLUGS`.
* Delete the two dead move texts (or wire them, if a case really needs them).
* Unit test: every field-note slug has a text.

## Play-test

* `bin/game --play tests/play/journal-field-notes-gaps.txt`: learn Dodge, Power hit, Flaming arrows and Words of Thoth on the Heka page (K); each writes its field note.
* Walk on until the wall spikes come out in view: their note is written.
* Read the Notes in the journal (J): the 5 new notes, in the notebook voice.
* Optional: tests/levels/fire_vents and tests/levels/poison_drips write the fire-jet and poison-drip notes.
