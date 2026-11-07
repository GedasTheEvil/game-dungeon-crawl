# Human-like monster notes: what the archaeologist felt, not how the monster works

Status: implemented 2026-11-06, waiting for the user's play-test. Was: draft 2026-11-06, size M, needs nothing first.

## Idea

The journal description of a monster (`note` in `content/texts/monsters.txt`, written after the first kill) should
read like a person wrote it after a fright, not like a rules card. Drop how it hurts and how it works ("its sting
poisons", "jumps over pits", "hard to hit in the air"); the SAW lines (`content/texts/moves.txt`), the hit and the
resistance rows of the journal page already say that. Instead: what the archaeologist saw, compared with what came
before, how it felt, a short reaction.

The user's example, the giant scarab:

> This bug is a lot bigger than the ones I saw previously. It even tried to throw a ball of dung at me. Ew!

## Plan

* Rewrite every `note:` in `content/texts/monsters.txt` (36 monsters) in that voice: first person, plain words, 1 to 3
  short sentences, a reaction ("Ew!", "I ran.", "Never again."). Keep the existing lore jokes that already work
  (Odysseus and the lotus, Shai-Hulud, the ushabti's "here I am", Babi the heart-eater) where they fit the new voice.
* Compare with kin met earlier where there is one: giant rat vs rat, giant bat vs bat, giant scarab vs scarab, giant
  scorpion vs scorpion, giant cobra vs cobra, tiny spider vs camel spider, cursed mimic and mimic jar vs mimic, each
  boss vs its kin. The note is written after the first kill, but the kin may not have been met yet (levels, `--level`):
  word it so it still reads right ("bigger than any beetle I know", not "than the ones on level 2").
* A behaviour may show up as something seen, not a rule: "it tried to throw a ball of dung at me" is fine,
  "rolls dung balls that hurt" is not. No numbers, no damage words, no poison strength.
* Bosses keep their god or myth in one line, then the personal reaction.
* No code change expected (`monsterNote` reads the file). Check the journal page still fits (line count, wrap) with
  the longest notes: `docs/ui.md` (journal), a screenshot scenario of a few creature pages.
* Leave `MonsterKind::description` (`src/level/monster_kinds.cpp`) alone: it is the editor and tool label, not shown
  to the player.

## Play-test

* `bin/game --play tests/play/human-monster-notes.txt` (a save with every creature written down), press J, open the
  Creatures ribbon and leaf through.
* Each creature page reads like a diary line with a reaction, no rule talk, no SAW lines; the giant scarab's matches the example's
  tone.
* The text fits the page, no cut-off lines.

## Play-test feedback

### Round 1 (2026-11-07)

- [x] the SAW lines on a creature page are still there, remove them, no need for how the bite feels. Fixed: the SAW block is gone (`attackSentence`, `creatureMoveNote`, `content/texts/moves.txt` deleted); moves are still tracked (tests, save) but no longer mark a page new.
