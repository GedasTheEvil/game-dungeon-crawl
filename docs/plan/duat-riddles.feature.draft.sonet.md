# Riddles of the Duat: riddles the game's own texts answer

Status: draft 2026-11-13, size S, needs nothing first. From the lore coverage audit of 2026-11-13.

## Idea

The riddle gates are sphinxes in an Egyptian underworld, and almost none of their riddles is about it.

* `content/riddles/`: 5 files, 115 riddles (`grep -c "^Q:"` counts Q lines: gate 23, palindromes 23, pop 21, scifi 26,
  stargate 22; a riddle can have two). Egyptian at all: about 5, all in `gate.txt` (shesep-ankh / lion, two hounds,
  coffin, mummy, Nile). None asks about a god, a boss, an hour or a text the player read.
* A run meets 62 riddle gates (one a level in zones 1-11, three a level in zone 12: `ZONE_GATES`, `src/level/campaign.h`),
  dealt from one shuffled deck (`src/ui/riddle.cpp`), so there is room for more.
* Reading pays (inscription XP, boons), but nothing asks the player to remember what they read. A riddle whose
  answer is on a wall the player passed rewards the reader twice and ties the gates to the world.

## Plan

A new riddle file `content/riddles/duat.txt`, theme "Riddle of the Duat" (or "The Sphinx of the West"), 15 to 20
riddles. Format: [riddles.md](../riddles.md) (Q, A per accepted spelling, H). Rules:

* Every answer is taught by a text the game shows: an inscription (`content/inscriptions/*.txt`), a field note or
  monster note (`content/texts/journal.txt`, `monsters.txt`), the skill lore (`skills.txt`). Write the source in a `#`
  comment above each riddle (id or note slug), so a later text change can keep them in step.
* Answers a player can also know from general Egyptian lore; the hint points to where the game said it.
* Plain ASCII; accept the usual spellings (`serket`, `selket`, `serqet`).
* Not the judgment's questions (heart, feather, Maat, Thoth's verdict, Ammit): those belong to
  [weighing-of-the-heart](weighing-of-the-heart.feature.draft.opus.md)'s zone deck. If that plan is done first, don't
  repeat its riddles; if this one is first, keep those answers out anyway.

Sample riddles (check each against the cited text):

    # monsters.txt boss-scarab; zone_look.h Khepri
    Q: I roll the sun across the sky as a beetle rolls its ball,
    Q: and I made myself. Who am I?
    A: khepri
    A: khepera
    H: The lord of the first boss's lair.

    # inscription 18 Overthrowing Apep; monsters.txt apep
    Q: Every night I try to swallow the sun, and every night I fail.
    Q: Who am I?
    A: apep
    A: apophis
    H: The serpent of chaos.

    # inscription 99 The boatman's fee
    Q: The ferryman of the dead asks one fee, and it is not gold. What is it?
    A: a word
    A: word
    A: name
    A: his name
    H: The boatman's inscription in the cisterns.

    # note.amulet-traps
    Q: Torn out by Seth, healed by Thoth, worn against the spikes. What am I?
    A: eye of horus
    A: wedjat
    A: udjat
    H: An amulet for the floors with teeth.

    # note.shrine-of-ptah, skill djed-of-ptah
    Q: I am the backbone of Osiris and I do not bend. What am I?
    A: djed
    A: djed pillar
    H: Ptah's sceptre carries me.

Also good topics: Hapi (the Nile in flood), Serket (the scorpion goddess who makes the throat breathe), Sobek of
Shedet, Seth as the red hippopotamus, the ushabti's answer ("here I am"), the seven scorpions of Isis, Thoth's
baboons, the ankh in the serpent's name (Life of the Gods), the shen ring, sky iron (bia-n-pet).

Unit test: `tests/unit/riddle_files_test.cpp` already fails on a repeated question; `game.log` shows the new count.

## Play-test

* A play script with `riddles content/riddles/duat.txt` (as `tests/scenarios/solved/riddle.txt` uses it) before a
  riddle gate: each riddle reads well on the scroll (about 6 lines fit), the hints help.
