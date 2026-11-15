# Field notes for the sphinx and name gates, swimming and the dark

Status: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-13, size S, needs nothing first. From the lore coverage audit of 2026-11-13.

## Idea

The journal writes a field note the first time the player meets most rules of the tomb (`FieldNote`,
`src/level/journal.h`; texts `[note.<slug>]` in `content/texts/journal.txt`; triggers `events.Note(FieldNote::...)` in
`src/sim/`). The traps without a note (floor spikes, death trap, rock and sand falls, ceiling traps) are
[journal-trap-notes](journal-trap-notes.feature.md). Still without one after it (`grep -n "^\[note\."
content/texts/journal.txt`, 76 note sections):

| Thing | First met | Note today |
|---|---|---|
| Riddle gate (the sphinx) | lvl1 | the riddle goes to the Riddles ribbon; nothing says what the gates are |
| Name gate | lvl5 | none; the Wisdom of Seshat note mentions it, expedition page 20 explains it on lvl38, 33 levels late |
| Deep water: swimming | zone 7, its mechanic (`ZoneMechanic::Water`) | only the Hapi skill and potion notes, written when learnt or found, not on the first swim |
| The dark: light keeps the spiders off | zone 3, its mechanic (`ZoneMechanic::Dark`) | only the hour-3 note's "I keep a torch close" |

Zones 3 and 7 are the only zones from the tomb on whose own mechanic has no note (sand, quicksand, mirrors, sandstorms,
fire jets, poison drips, dart plates all have one; zones 1 and 2 are in journal-trap-notes).

## Plan

* New `FieldNote` values at the end of the enum (notes are saved by number), their slugs in `AFTER_RING_SLUGS`
  (`src/level/journal_text.cpp`), `FIELD_NOTE_COUNT` up, texts in `journal.txt` in the notebook voice
  ([texts.md](../texts.md)). If journal-trap-notes is done first, append after its notes.
* Triggers, each the first time: the first riddle gate's scroll opened; the first sealed name gate touched; the first
  swim in deep water; the first camel spider (or tiny spider) turned away by light, in view.
* Each note says the rule as the game has it (keep in step with the code) and one line of lore. Samples, to check
  against the rules:
  * Riddle gates: "A sphinx at the gate, and a scroll with a question. The old texts call a sphinx shesep-ankh, a
    living image. This one is living enough to keep the door shut until I answer. After two wrong tries it lets slip a
    hint; walking away costs the reward, and the riddle stays in my notes."
  * Name gates: "A gate that wants a name. Somewhere on this level a king's name is carved in a cartouche: copy it,
    read it, say it. In Egypt a name was a part of you. Here it is a key."
  * Swimming: "Deep water. I swim, slowly, and it tires me twice as fast as walking. Out of breath in the water is a
    bad place to be out of breath. Rest on the stone." (check the stamina rule, and do not repeat the Hapi notes)
  * The dark: "The spiders here hate the light. A torch on the wall, a brazier: they will not come into its circle.
    So I go from flame to flame, like a moth with a plan."
* Unit test per trigger where a unit test reaches it (the existing note tests are the pattern); `texts_test.cpp` asks
  every key.

## Open questions

* The riddle gate note on lvl1 lands among the first-level hints; check it does not crowd them (the hints are on screen,
  the note is in the journal, so probably fine).

## Play-test

* `bin/game --play tests/play/field-notes-gates-and-water.txt` (level 1): walk to the riddle gate, interact, Esc, open J,
  Notes: "Riddle gates".
* `bin/game --level 5`: touch the name gate: "Name gates". `bin/game --level 28`: wade in the water: "Water".
  `bin/game --level 10`: a camel spider stops at a torch's light, in view: "The dark".
* Texts match the rules (see Implemented for what the code does).

## Implemented

* Notes `RiddleGates` (the scroll opened, `Dungeon::Interact` case `GateRiddle`), `NameGates` (a sealed name gate
  interacted with), `Wading` (a moving step in half water, `Dungeon::Move`), `TheDark` (a light shunner turned away by a
  lit cell between it and the player, in view, beside its `ShunLight` creature move in `updateWalker`); after the trap
  notes, `FIELD_NOTE_COUNT` 86; texts in `journal.txt`; tests `tests/unit/gate_water_notes_test.cpp` and
  `camel_spider_test.cpp`.
* Decided differently, from the code: there is no swimming (deep water is solid); the player wades in half water at
  half speed, no sprint, no jump, and nothing in the stamina rule differs in water, so the draft's "tires me twice as
  fast" is not true and the note says what the game does (half pace, no sprint or jump, fire weak, crocodiles lie low,
  Hapi). Title "Water", slug `wading`.
* Riddle note: the gate is spent when the scroll opens and never blocks the way, so the draft's "keep the door shut" is
  left out; reward (a share to the next level, at least 500), two misses for the hint, walking away, a tenth later
  are as the code has them. The name note adds Words of Thoth and that walking away leaves it sealed.
* The dark note: light is a torch, brazier, lamp or a mirror's sunbeam, the cell and the next one either way; a
  spider fears it only unhurt; the trigger needs a spider that flees (the shy walker, `runs`), in view.
* Open question: the riddle note on level 1 sits with the first-level hints; they are on screen, the note is in the
  journal, left as is.
