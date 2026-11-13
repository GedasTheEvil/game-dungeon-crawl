# The weighing of the heart: the Hall of Judgment keeps its promise

Status: draft 2026-11-13, size L, needs nothing first. From the lore coverage audit of 2026-11-13.

## Idea

The last zone promises a judgment and never holds one.

* `docs/levels.md` (zone table) and [campaign-zones](solved/campaign-zones.md) give zone 12 the mechanic "riddle-heavy
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
  (id 130's "Forty-two sins denied"). [hippo-boss](solved/hippo-boss.md) and [hippo](solved/hippo.md) left an Ammit
  "for later".
* The epilogue (`content/texts/journal.txt`, `[diary.dawn]`) does not mention the judgment or Anubis at all.

## Plan

Three steps, each worth having alone; do them in order, stop after any.

1. **The assessors ask (riddles of the judgment).** A riddle file `content/riddles/judgment.txt`, theme "The Hall of
   the Two Truths", 12 to 16 riddles in the voice of the 42 assessors and Thoth, every answer something the game's own
   texts teach (inscriptions, field notes, monster notes): "What is set on the scale against the feather?" (heart),
   "Who writes the verdict?" (thoth), "Whose feather?" (maat), "Who steadies the scale?" (anubis), "Who eats the heart
   that sinks?" (ammit, devourer), "Say the confession: I have not ______" (stolen, killed, lied), "In whose name is the
   living one called justified?" (osiris). Format: [riddles.md](../riddles.md). Then a file-level line, e.g.
   `zone: hall_of_judgment`, in the riddle loader (`src/ui/riddle.cpp`): a riddle with a zone is dealt only on that
   zone's levels, and those levels deal only such riddles while any are left (fallback: the normal deck). Unit test
   in `tests/unit/riddle_files_test.cpp`. 12 riddle gates in the zone today (lvl51-54, 3 each).
2. **The scales.** A placed prop on lvl54 between the Anubis boss's gate and the ankh: a balance with the feather on one
   pan (a new model, `tools/blender/models/decor.py`, [modeling.md](../modeling.md)), Osiris enthroned behind it (the
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
[inscriptions.md](../inscriptions.md) (new id, `G:` line, the sheet rebuilt if a new sign: `tools/textures/inscriptions.py`).
`bin/levelcheck content/levels/lvl5*` stays warning-free.

## Open questions

* Should the verdict change anything but text (a heavy heart: a blessing lost, Ammit's fight)? Default: text only.
* Score inputs: is opening a coffin "robbery" in this game's tone? The coffins note already jokes about it.
* The zone-riddle rule: a `zone:` line per file, or per riddle?

## Play-test

* `bin/game --level 51`: the riddle gates ask the assessors' questions, not palindromes.
* A play script that starts on lvl54 past Anubis, the ankh ahead: walk to the scales, press E, read the verdict in the
  journal (J), take the ankh, read the epilogue's last line.
