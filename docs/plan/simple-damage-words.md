# Simple damage words

Status: implemented 2026-10-14, waits for the play-test (`bin/game --play tests/play/simple-damage-words.txt`). From the user. Two parts: simple damage words, then a
plain-English rewrite of the journal.

## Why

The user's children play-test the game. English is not their first language, and words like "blunt", "pierce" and
"bludgeon" mean nothing to them. The game stays in English, but the words the player reads should be the plain
words a child learns first.

## Change

Only text the player reads changes. The code (`DamageType::Blunt`, ...), the scenario commands, the level files and
the glossary's code terms stay as they are.

| now | player reads |
|---|---|
| blunt | smash |
| slash | cut |
| pierce | stab |
| fire | fire (no change) |

"Smash", not "punch" (the user's first idea): a punch is a fist, but here a club, a mace or a sling stone deals it.
"Smash" is a word children know. The user can still pick "punch".

Where:

* `DAMAGE_TYPE_NAMES` (`src/world/damage.h`): inventory weapon stats, journal monster pages, the "Journal: ..."
  status line. Simplest: a second array of player words, or rename the strings in place if nothing parses them
  (check the scenario/test code first).
* `attackSentence` (`damage.h`): one word family per type. MAIN: "smashes me hard", "cuts me deep", "stabs me to the
  bone", "sets me on fire". VERB: "smashes", "cuts", "stabs", "burns". HINT: "smashing pain", "cutting pain",
  "stabbing pain", "burning pain".
* Journal topics (`src/ui/journal_view.cpp`): titles "Smash", "Cut", "Stab". Rewrite the texts in short, plain
  sentences, without "bludgeons", "crush rather than cut". Example: "A club smashes. So do the mace, sling stones
  and the throwing stick."
* Amulets (`src/world/items.cpp`): "Blunt Warding" → "Smash Guard", "Slash Warding" → "Cut Guard", "Pierce
  Warding" → "Stab Guard"; the short labels and "%d%%" lines the same way. "Warding" is a hard word too: "Trap
  Warding" → "Trap Guard" and the same for every other "... Warding".
* The Stun skill card (`src/world/skills.cpp`): "Blunt hits may stun a foe" → "Smash hits may stun a foe".
* Grep for any other text the player reads: `grep -rniE 'blunt|slash|pierc|bludgeon|warding' src`.
* Glossary (`docs/glossary.md`): add the player words to the "damage type" row.
* Update the unit tests and scenarios that check these strings.

## Also (same pass, small)

* "draught" → "potion" in every name and journal text ("Health draughts" → "Health potions"). "Potion" is a
  word every child who plays games knows.

## Plain-English journal (user, 2026-10-14: same pass)

Rewrite every journal text in plain English: the topics (`src/ui/journal_view.cpp`) and the creature notes
(`.note` in `src/world/monster_kinds.cpp`).

Rules:

* Reader: a 12 to 14 year old whose first language is not English (user, 2026-10-14). English at school since the
  2nd grade: solid basics, heading towards advanced, not there yet (about B1-B2). So: normal, natural English, not
  simplified. Cut only what such a reader trips over: rare or bookish words ("bludgeon", "draught", "warding"),
  idioms and figures of speech ("like a vice", "takes a bite out of it"), long sentences with several clauses. Do
  not rewrite a text that is already clear: most need a word or two changed, not a new text.
* Keep the first-person voice ("I", "me") and the Egyptian names (Montu, Ptah, Hathor, khopesh, ...): they are the
  game's flavour, and a name explains itself. Explain a name once, plainly, where it is not clear ("the khopesh, a
  curved sword").
* Keep every fact: keys (H, U, Shift, ...), numbers, which item or creature does what. A rewrite that drops or
  changes a fact is wrong. Compare each text old to new before committing.
* A text may get longer when plain words need it, but it must still fit its page: check the journal's topic and
  creature pages with a screenshot scenario (`tests/scenarios/`, [testing.md](../testing.md)).
* Example. Old: "Jumping and running (Shift) tire me out: the yellow bar. Each jump takes a bite out of it, running
  drains it all the time." New: "Jumping and running (Shift) tire me out: the yellow bar. Each jump uses up some of
  it, and running drains it all the time." (Only the idiom goes.)

Not in this plan: the riddles (their wording is the puzzle, and the answers must still fit), the skill cards
other than Stun, item descriptions. Their own pass if the user wants it.
