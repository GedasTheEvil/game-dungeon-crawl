# Inscription boons: a permanent stat for a few inscriptions

Status: solved 2026-10-15, play-tested by the user (`bin/game --play tests/play/inscription-boons.txt`).
From the user. Works alongside [more-inscriptions](denser-levels.md#more-inscriptions-from-level-1-on) (more texts to put boons on).

## Done (2026-10-15)

* Amounts (the user): max health +10 %, max stamina +10 %. Stamina is a percent too (`boon: stamina 10`), saved as
  `BOONS` in the player stats; `MaxStamina()` adds it.
* 7 of 32 campaign texts, one level each: 3 ushabti (lvl 8, stamina), 7 not dying a second time (lvl 11, life),
  16 djed (lvl 18, armor), 18 overthrowing Apep (lvl 20, might, Montu), 21 satire of the trades (lvl 22, stamina, the
  royal messenger), 28 heart scarab (lvl 28, armor, granite), 29 king of kings (lvl 29, might). One line added or
  rewritten in each to name the gift.
* Journal version 9 keeps each copy's boon. Page: "Gift: Might +1" in red ink under the comment, translated pages only.
* Scenario command `inscriptions PATH` (docs/testing.md); scenario `inscription_boons` (in `tests/scenarios/solved/`), unit tests in
  `tests/unit/inscriptions_test.cpp` (parsing, first reading only, a copy and learning, save/load, the campaign's
  share and spread).
* Not done: the optional gold flash, a levelcheck boon count.

## Idea

A few inscriptions give a small permanent bonus the first time they are translated: "the strength of kings" gives
Might +1. It is small, but it makes the hieroglyph skill worth more and makes a player want to read every wall.

Only a few inscriptions get one, about 1 in 5. Most stay atmosphere, hints and XP. It works like a lottery (the
user, 2026-10-14): the player does not know whether a wall holds a gift until it is read with the skill. Sometimes
you win, sometimes you don't. Nothing hints at a boon beforehand: not the wall, not the journal copy.

## Rules

* Kinds, exactly four (the user): Might +1, Armor +1, max HP +N %, max stamina +N. The first three are the
  potions' permanent gains (`PlayerStats::Drink`, `src/world/items.cpp`: Blood of Montu, Granite of Ptah, Life).
  Max stamina is new: `PlayerStats::MaxStamina()` is computed from the level only, so add a saved bonus field
  (player save version) and add it there.
* When: like the XP ([inscription-xp](inscription-xp.md)), on translation, once per inscription. Read with
  the hieroglyph skill: given at once. A copy made without the skill gives nothing until the skill is learned, then
  `Journal::TranslateInscriptions` gives it. This is the reward for reading the early copies.
* The text matches the boon: kings, Montu or the bull give Might; Ptah, granite, the djed pillar give Armor; the
  ankh, "breath of life", going forth by day give max HP; Shu and the wind, the royal messenger, the runner give
  max stamina.
* Rarity: about 1 in 5 inscriptions (the user; the exact share is the implementer's). With today's 32 texts that is
  6 or 7; as [more-inscriptions](denser-levels.md#more-inscriptions-from-level-1-on) adds texts, keep the share. Mix the four kinds,
  spread them out (not two in one level). Compare the might and armor potions placed in the campaign so that boons
  stay a side bonus. The implementer judges the balance.
* Candidates among the current texts: 29 "King of kings" (Might), 16 "The djed of Osiris" (Armor), 32 "Going forth
  by day" (max HP), 10 "Anubis upon his mountain" or 22 "The instruction of Amenemhat" (Might). Rewrite a line where
  needed so that the text names the gift. New texts are fine too (Montu's war hymn, Ptah's "lord of truth", the
  ankh formula "given life like Ra forever").

## Data

* `content/inscriptions/*.txt`: an optional line `boon: might 1` / `boon: armor 1` / `boon: life 10` / `boon: stamina 10`. Parsed by
  `InscriptionBook`; an unknown kind or a bad number is a load error (`file:line`).
* `JournalInscription` keeps a copy of the boon (the journal keeps a copy of each text, so editing the file later
  changes nothing). Bump the journal save version. Old saves can be ignored (AGENTS.md).
* `levelcheck`: no new warning needed. Optional: report the campaign's boon count by kind in its summary.

## Player feedback

* Status line, after the XP: "Deciphered the hieroglyphs. Journal updated. +N XP, Might +1".
* Learning the skill: the toast adds the boons, e.g. "3 inscriptions translated, +N XP, Might +1, Armor +1".
* Journal page: a small line under the comment, in the hand font, like "Gift: Might +1", so that the player sees
  where the stat came from. A copy that is not translated yet shows nothing: no boon is shown or hinted before
  the translation (decided by the user).
* Optional: the level-up sun beam or a short gold flash on the status line.

## Glossary

Add a row "boon": a permanent stat an inscription gives when it is translated. Code: the parsed field and
`JournalInscription`'s copy.

## Tests

* Unit (`tests/unit/inscriptions_test.cpp`): parse `boon:`, reject a bad one. The SimWorld case: the first read
  with the skill raises Might by 1, the second does not; a stamina boon raises `MaxStamina()`. A copy without the skill does not raise it, learning the
  skill does, once.
* Save/load keeps the boon on a copy, so translating after a load still gives it.
* `bin/levelcheck content/levels/lvl*` with no warnings.

## Open points

* The exact amounts for max HP % and max stamina, and the exact share (about 1 in 5).
