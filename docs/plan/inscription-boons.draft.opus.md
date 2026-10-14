# Inscription boons: a permanent stat for a few inscriptions

Status: draft 2026-10-14. Needs nothing first. From the user. Works alongside
[more-inscriptions](more-inscriptions.draft.opus.md) (more texts to put boons on) but does not need it.

## Idea

A few inscriptions give a small permanent bonus the first time they are translated: "the strength of kings" gives
Might +1. It is small, but it makes the hieroglyph skill worth more and makes a player want to read every wall.

Only a few inscriptions get one. Most stay atmosphere, hints and XP. If every inscription gave a stat, the boon
would be expected and stop feeling like a find.

## Rules

* Kinds: the permanent gains the potions already give (`PlayerStats::Drink`, `src/world/items.cpp`): Might +1
  (Blood of Montu), Armor +1 (Granite of Ptah), max HP +N % (Life). Optional: max stamina, if it is easy to add.
  Nothing new to balance in the stat model.
* When: like the XP ([inscription-xp](solved/inscription-xp.md)), on translation, once per inscription. Read with
  the hieroglyph skill: given at once. A copy made without the skill gives nothing until the skill is learned, then
  `Journal::TranslateInscriptions` gives it. This is the reward for reading the early copies.
* The text matches the boon: kings, Montu or the bull give Might; Ptah, granite, the djed pillar give Armor; the
  ankh, "breath of life", going forth by day give max HP.
* Count, suggested: 6 to 8 over the 30 levels, about 2 or 3 of each kind, spread out (not two in one level).
  Compare the might and armor potions placed in the campaign so that boons stay a side bonus. The implementer
  judges the balance.
* Candidates among the current texts: 29 "King of kings" (Might), 16 "The djed of Osiris" (Armor), 32 "Going forth
  by day" (max HP), 10 "Anubis upon his mountain" or 22 "The instruction of Amenemhat" (Might). Rewrite a line where
  needed so that the text names the gift. New texts are fine too (Montu's war hymn, Ptah's "lord of truth", the
  ankh formula "given life like Ra forever").

## Data

* `content/inscriptions/*.txt`: an optional line `boon: might 1` / `boon: armor 1` / `boon: life 10`. Parsed by
  `InscriptionBook`; an unknown kind or a bad number is a load error (`file:line`).
* `JournalInscription` keeps a copy of the boon (the journal keeps a copy of each text, so editing the file later
  changes nothing). Bump the journal save version. Old saves can be ignored (AGENTS.md).
* `levelcheck`: no new warning needed. Optional: report the campaign's boon count by kind in its summary.

## Player feedback

* Status line, after the XP: "Deciphered the hieroglyphs. Journal updated. +N XP, Might +1".
* Learning the skill: the toast adds the boons, e.g. "3 inscriptions translated, +N XP, Might +1, Armor +1".
* Journal page: a small line under the comment, in the hand font, like "Gift: Might +1", so that the player sees
  where the stat came from. A copy that is not translated yet shows nothing (the boon is a surprise).
* Optional: the level-up sun beam or a short gold flash on the status line.

## Glossary

Add a row "boon": a permanent stat an inscription gives when it is translated. Code: the parsed field and
`JournalInscription`'s copy.

## Tests

* Unit (`tests/unit/inscriptions_test.cpp`): parse `boon:`, reject a bad one. The SimWorld case: the first read
  with the skill raises Might by 1, the second does not. A copy without the skill does not raise it, learning the
  skill does, once.
* Save/load keeps the boon on a copy, so translating after a load still gives it.
* `bin/levelcheck content/levels/lvl*` with no warnings.

## Open points

* Max stamina as a fourth kind: implementer's choice.
* Whether the inscriptions screen hints which walls carry a boon (an icon on the wall): suggested no, discovery is
  the point.
