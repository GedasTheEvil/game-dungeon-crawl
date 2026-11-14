# Lore corrections leftovers: Babi the heart-eater, Apep swallowing the sun

Status: implemented 2026-11-14, waiting for the user's play-test. Was: draft 2026-11-13, size S, needs nothing first.

From the lore audit of 2026-11-13. [lore-corrections](solved/lore-corrections.bug.md) fixed these facts in some texts
and missed other copies. Text only; keep the voice of each text.

## Repro

1. `bin/game --level 45`, kill Babi, open the journal (J), Creatures: Babi's note.
2. Answer or read the Stargate riddles (`content/riddles/stargate.txt`, theme "Chevron Seven Locked").

## Expected

The same facts as [lore-corrections](solved/lore-corrections.bug.md) decided, everywhere:

* Babi eats entrails, not hearts. Book of the Dead spell 125: Babi "who lives on the entrails of the elders";
  the eater of hearts at the weighing is Ammit (Am-mut, the Devourer). The zone banner already says "Babi, the
  eater of entrails" (`src/level/zone_look.h:136`), `rebuild.txt` id 122 "bull of the female baboons".
* Apep would swallow the sun and fails every night (the monster note already says so). His enemy in the Duat is Ra
  (whose boat he attacks; Seth spears him from the prow, Isis and Serket bind him in the seventh hour). Sobek is
  not Apep's foe in the Amduat or the Book of Gates. Sure.

## Actual

* `content/texts/monsters.txt:127`: "Babi, the heart-eater, the loudest of the baboons. I keep my heart, thank you."
* `src/level/monster_kinds.cpp:1325` (comment): "Babi (plan babi-boss), the bull of the baboons, who eats the hearts
  of the dead by the lake of fire".
* `content/riddles/stargate.txt:10-11`: "Which System Lord is named after the serpent / that swallows the sun every
  night?"
* `content/riddles/stargate.txt:13`, its hint: "Sobek's old enemy in the Duat." (In Stargate SG-1 Apophis's rival
  was Ra, later Sokar; "Sobek" fits neither the myth nor the show.)

## Fix

* monsters.txt babi: e.g. "Babi, the bull of the baboons, who lives on the entrails of the dead. The loudest of
  them. I keep my insides, thank you."
* monster_kinds.cpp comment: "who lives on the entrails of the dead".
* stargate.txt: "named after the serpent / who tries to swallow the sun every night?"; hint "Ra's old enemy in the
  Duat." (true of the myth and of the show).
* `grep -rn -i "heart-eater\|eats the hearts\|swallows the sun" src content readme.md docs/*.md` afterwards: none
  left (solved plans are history).

## Fixed when

* A test that names the plan fails before the fix and passes after it: e.g. in `tests/unit/texts_test.cpp`, the babi
  note has no "heart"; the riddle loader test: no stargate riddle says "swallows the sun" or "Sobek".

## Play-test

* `bin/game --level 45`: kill Babi, read his journal note: entrails, not hearts.
