# Lore additions

Status: implemented 2026-11-05, waiting for the user's play-test. Was: draft 2026-10-30, size M, needs nothing first. From the project audit of 2026-10-30.

Authentic details that would deepen the texts. Each is optional; pick the ones that fit the text and level spots.

* **12th hour**: Ra is reborn by passing through the serpent "Life of the Gods" (ꜥnḫ-nṯrw) tail to mouth and comes out
  as Khepri at dawn. Ties the ankh goal to the last hour; use in the hour-12 note and the epilogue
  ([win-ending-flow](solved/win-ending-flow.md)).
* **Sphinx**: the Egyptian *shesep-ankh*, "living image": another ankh tie for the riddle gates (a riddle-gate text).
* **Mehen**, the coiled serpent guarding Ra's boat in hour 7: not every serpent is an enemy (inscription id 91 hints at
  it already).
* **The boatman** (inscription id 99): Book of the Dead spell 99, the ferryman Mahaf / Aqen asks the names of the
  boat's parts, so "pay with a word" is authentic. Name him, cite the spell.
* **The seven stars** (id 127): Meskhetyu, the Big Dipper, the "foreleg of Seth".
* **Isis's seven scorpions** (Tefen, Befen, Mestet, Mestetef, Petet, Tjetet, Matet): names for giant scorpions in
  texts. Naming the scorpion queen Tefen (instead of Serket's daughter, odd when Serket's favour follows) touches a
  boss: leave until bosses are back in scope.
* **The workmen's absence list** (id 56): ostracon BM 5634 has "stung by a scorpion"; add the line.
* **Tayt**, the weaving goddess who made mummy linen: a nod near the Mother of Webs.

## Implemented

* **12th hour**: the hour-12 field note names the serpent, Life of the Gods (ankh-netjeru), tail to mouth, and the ankh
  in its name. The epilogue (`diary.dawn`) already told it.
* **Sphinx**: a new riddle in `content/riddles/gate.txt`: "My makers called me shesep-ankh, the living image: the face
  of a king on the body of a ______" (lion). Not "sphinx": the scroll's heading, "Riddle of the Sphinx", would give it
  away.
* **Mehen** (id 91, lvl26): "In the seventh hour coiled Mehen guards Ra's boat from Apep." The seventh hour's field note
  already named him.
* **The boatman** (id 99, lvl31): Mahaf asked to wake Aqen, the mast and the sail asking their names; the comment cites
  Book of the Dead, spell 99.
* **The seven stars** (id 127, lvl48): "Meskhetyu, the foreleg of Seth, never sets in the north."
* **The absence list** (id 56, lvl3): "Absent: Pendua, stung by a scorpion." (The name is from the village, not the
  ostracon line.)
* **Tayt** (id 70, "The weaver's complaint", lvl12, the level before the Mother of Webs): she wove the linen that wraps
  the dead, yet the spider weaves finer.
* Not done: Tefen for the scorpion queen (the draft leaves it until bosses are back in scope).
* Checked: the scenario `tests/scenarios/lore_additions.txt` screenshots the journal pages of 91, 99 and 127 (they fit,
  no orphan words).

## Play-test

`bin/game --play tests/play/lore-additions.txt` (lvl31, at the boatman's inscription, Words of Thoth learnt):

* Read it (E), open the journal (J), Inscriptions: Mahaf, Aqen, the mast and the sail; "Book of the Dead, spell 99".
* Other texts, by level: `bin/game --level 26` (Mehen), `--level 48` (Meskhetyu), `--level 3` (the scorpion sting),
  `--level 12` (Tayt); the sphinx riddle comes from the shuffled deck of any riddle gate.
