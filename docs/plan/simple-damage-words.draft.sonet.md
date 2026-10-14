# Simple damage words

Status: draft 2026-10-14. Ready, needs nothing first. From the user.

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

## Later, not in this plan

A full plain-English pass over the journal texts (shorter sentences, no rare words like "bitter", "brew",
"bludgeons"), skill cards and riddles. Bigger and a matter of taste: its own draft if the user wants it.
