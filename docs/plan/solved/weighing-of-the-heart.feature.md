# The weighing of the heart: the Hall of Judgment keeps its promise

Status: solved 2026-11-15 (play-tested by the user). Was: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-13, size L, needs nothing first. From the lore coverage audit of 2026-11-13.

## Idea

The last zone promises a judgment and never holds one.

* `docs/levels.md` (zone table) and [campaign-zones](campaign-zones.md) give zone 12 the mechanic "riddle-heavy
  finale, the weighing of the heart". The zone banner calls Anubis "who weighs the heart" (`src/level/zone_look.h`),
  the level titles are "The Hall of the Two Truths", "The Feather of Maat", "The Weighing of the Heart" (lvl52-54).
* Inscriptions 131 (The weighing of the heart, lvl52), 134 (Maat, the feather, lvl53), 130 (The negative confession,
  lvl51), 133 (The breath of the justified, lvl52) describe the scene. The hour-12 note says "something here will
  weigh my heart".
* Nothing in the code weighs anything: `grep -rni "weigh\|feather\|ammit" src` finds only render weights, gift weights
  and two comments (`monster_kinds.cpp:1042`, `zone_look.h:154`). The zone's mechanic is "Riddles" (`ZoneMechanic`,
  `campaign.h`): 3 riddle gates a level (`ZONE_GATES`), dealt from the same shuffled deck as everywhere, so the "Thoth asks
  his questions" of inscription 132 (lvl51) are palindromes, film trivia and Stargate questions (of 115 riddles in
  `content/riddles/`, about 5 are Egyptian at all; `grep -c "^Q:" content/riddles/*.txt`: 23+23+21+26+22 Q lines, a
  question can take two).
* Who is missing from the judgment: Osiris, who presides over it, appears in no zone-12 text but as "Osiris-justified"
  (id 133); Ammit is never named (one "eaten by the devourer", id 131); the 42 assessors only in a pencilled comment
  (id 130's "Forty-two sins denied"). [hippo-boss](hippo-boss.md) and [hippo](hippo.md) left an Ammit
  "for later".
* The epilogue (`content/texts/journal.txt`, `[diary.dawn]`) does not mention the judgment or Anubis at all.

## Plan

Three steps, each worth having alone; do them in order, stop after any.

1. **The assessors ask (riddles of the judgment).** A riddle file `content/riddles/judgment.txt`, theme "The Hall of
   the Two Truths", 12 to 16 riddles in the voice of the 42 assessors and Thoth, every answer something the game's own
   texts teach (inscriptions, field notes, monster notes): "What is set on the scale against the feather?" (heart),
   "Who writes the verdict?" (thoth), "Whose feather?" (maat), "Who steadies the scale?" (anubis), "Who eats the heart
   that sinks?" (ammit, devourer), "Say the confession: I have not ______" (stolen, killed, lied), "In whose name is the
   living one called justified?" (osiris). Format: [riddles.md](../../riddles.md). Then a file-level line, e.g.
   `zone: hall_of_judgment`, in the riddle loader (`src/ui/riddle.cpp`): a riddle with a zone is dealt only on that
   zone's levels, and those levels deal only such riddles while any are left (fallback: the normal deck). Unit test
   in `tests/unit/riddle_files_test.cpp`. 12 riddle gates in the zone today (lvl51-54, 3 each).
2. **The scales.** A placed prop on lvl54 between the Anubis boss's gate and the ankh: a balance with the feather on one
   pan (a new model, `tools/blender/models/decor.py`, [modeling.md](../../modeling.md)), Osiris enthroned behind it (the
   `osiris` prop exists, `DECOR_NAMES`), the `thoth_ibis_*` prop beside it. Interact (E, verb "Be weighed"): the verdict
   comes from the run (`RunStats`, `src/level/run_stats.h`) and is written into the journal as a field note
   (`FieldNote` at the end, `journal.h`; text in `journal.txt`). Never a game over: the tone of the game is light, the
   ankh stays behind. Suggested scoring (the implementer tunes it): light for reading (`inscriptions`), answering
   (`riddles`), the secrets (`secrets`); heavy for the coffins and jars robbed, ushabti smashed, `deaths`. Two or three
   verdict texts in the notebook voice:
   * light: "The pan with my heart rose. Thoth wrote something short. Anubis nodded, which I did not know jackals could
     do. Somewhere behind the throne something with a crocodile's mouth sighed and lay down again."
   * heavy: "The pan sank, just a little. I said the confession again, louder, and left out the part about the chests.
     It rose. Thoth wrote something long. I am told the gods are fair. I am glad they are also tired."
   The verdict sets which last sentence the epilogue gets (step 3).
3. **The epilogue answers.** `[diary.dawn]` names the scales: one sentence keyed by the verdict (a second text key,
   e.g. `diary.dawn.p3-light` / `p3-heavy`, picked in `src/ui/win_entry.cpp`). No verdict (scales skipped): today's
   text.

Optional, its own step (implementer's choice, or a later draft): Ammit as a creature: crocodile head, lion front,
hippo hind (a new model and clips), lying behind the scales, rising only on a heavy verdict as a short fight, or as
the Anubis boss's summon. A new monster needs its journal note (`monsters.txt`) and a glossary row. If she gets the
hippo's or crocodile's abilities, AGENTS.md "Boss abilities" applies in reverse: nothing to add to the base monsters.

Also, for the texts: an inscription on lvl53 or lvl54 that puts Osiris on his throne in the hall ("Osiris,
foremost of the Westerners, sits on the throne; the forty-two sit before him"), so the scene has its judge. Follow
[inscriptions.md](../../inscriptions.md) (new id, `G:` line, the sheet rebuilt if a new sign: `tools/textures/inscriptions.py`).
`bin/levelcheck content/levels/lvl5*` stays warning-free.

## Open questions

* Should the verdict change anything but text (a heavy heart: a blessing lost, Ammit's fight)? Default: text only.
* Score inputs: is opening a coffin "robbery" in this game's tone? The coffins note already jokes about it.
* The zone-riddle rule: a `zone:` line per file, or per riddle?

## Done

* Step 1: `content/riddles/judgment.txt`, 15 riddles of the assessors and Thoth, the source text above each. A file's
  `zone: SLUG` line (`src/level/riddle_file.*`, moved out of `ui/riddle.cpp` so the unit tests reach it): a zone
  level deals one of its zone's riddles not yet in the journal (`zoneRiddlesLeft`), then the common deck; the common
  deck never deals zone riddles. Decision: per file, and "left" means "not met yet" by the journal, so it holds across
  saves and a new game. 15 riddles for the zone's 12 gates.
* Step 2: the scales (`DECOR_SCALES`, `decor_scales.md3`, `build_scales` in `decor.py`): the balance with the heart
  (an ib jar) and the feather, Thoth with his pen beside it and Osiris enthroned behind it in the same prop (the
  `osiris` prop is a toppled statue, not a judge). Placed on lvl54 col 25, between the boss gate (col 23) and the
  ankh (col 26). Verb "Be weighed" (`Interaction::Weigh`), once: the verdict (`src/level/judgment.h`) is a field
  note, `HeartLight` or `HeartHeavy`. Light: inscriptions + 2 x riddles + 3 x secrets; heavy: 5 x deaths + 2 x
  ushabti smashed (a coffin opened is excavation, not robbery: the default answer to the open question). Text only.
  A map mark and an editor icon.
* Step 3: the epilogue answers with a page of its own after Dawn ("The scales", `DiaryPage::ScalesLight` /
  `ScalesHeavy`), before D. J.'s answer, the same way as plan expedition-ending; no verdict, no page.
* Inscription 155 "The throne of the judge" on lvl53 (col 16, row 3, near the exit): Osiris on the throne, the 42
  assessors, Thoth, Anubis, Maat, and Ammit named.
* Not done: Ammit as a creature (the optional step).

## Play-test

* `bin/game --play tests/play/weighing-of-the-heart.txt`: walk right onto the scales, E ("Be weighed"): the verdict
  on the status line and in the journal (J, Field notes); E again does nothing.
* Look at the scales: the balance with the red heart and the feather, Thoth beside it, Osiris enthroned behind; F1
  for the comic look.
* Take the ankh: after Dawn the page "The scales" answers the verdict.
* `bin/game --level 51`: the riddle gates ask the assessors' questions (theme "The Hall of the Two Truths").
