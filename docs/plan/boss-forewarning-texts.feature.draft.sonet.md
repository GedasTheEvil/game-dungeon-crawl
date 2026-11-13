# Inscriptions that name the five bosses no wall mentions

Status: draft 2026-11-13, size M, needs nothing first. From the lore coverage audit of 2026-11-13.

## Idea

Six of the eleven boss levels have an inscription that names or describes their boss before the fight: lvl9 (id 5,
Shezmu, by his roost's teleporter), lvl27 (id 18, Overthrowing Apep), lvl32 (ids 95, 24, Sobek), lvl37 (id 49, the red
hippopotamus), lvl45 (id 122, The heat of Babi), lvl54 (ids 31, 128, Anubis). Five have none:

| Boss level | Boss | Its level's inscriptions | Boss named in any inscription? |
|---|---|---|---|
| lvl4 | Khepri | 58 You have not died | no: `grep -ci khepri content/inscriptions/*.txt` 0 |
| lvl13 | Mother of Webs | 71 Hymn to the evening sun | only Tayt and "the spider" in id 70 (lvl12) |
| lvl18 | High Priest of Anubis (mummy priest) | 75 The might of the king | no: "high priest" 0 |
| lvl22 | Scorpion queen | 86 The overseer's letter | Serket in ids 12, 80 (lvl19-20), no queen |
| lvl41 | Rerek | 114, 16, 115 | no: "rerek" 0 (only the monster note, after the kill) |

Counted with a script over the wall lists of `content/levels/lvl*` (type 15, the attribute is the text id) and the
titles in `content/inscriptions/*.txt`. The monster notes (`content/texts/monsters.txt`) name every boss, but only
after the kill.

## Plan

Five new inscriptions in `content/inscriptions/campaign.txt` (next free ids, see the loaded count in `game.log` or the
highest `id:`), one on each of the five boss levels, on a back wall the player passes before the boss gate or the
teleporter to the boss (`tools/level/lvledit.py LEVEL wall COL ROW 15 ID 0`). No boons (lvl4, 13, 18, 22, 41: check that
the level's boon count stays at most one). Format: [inscriptions.md](../inscriptions.md); new `G:` signs need
`python3 tools/textures/inscriptions.py`. Each text is atmosphere first and one hint at how the boss fights (the hint
must match `src/level/monster_kinds.cpp`). Sample texts, to be checked and tuned:

* **Khepri** (lvl4), style stela:
  T: I am Khepri in the morning, Ra at noon, Atum in the evening.
  T: I came into being of myself; the beetles are my children.
  T: When I call, they come out of the sand to me.
  N: The scarab god. He calls his children. I should kill them faster than he calls.
  (The formula is from the tale of Isis and the name of Ra; it also brings in Atum, who appears in no text today.)
* **Mother of Webs** (lvl13), style column:
  T: In the dark of the third hour sits the weaver who hates the lamp.
  T: Where she walks, the flames go out one by one.
  T: Keep a fire behind you, and she will not sit in your light.
  N: Something that puts torches out. I will bring spare torches, and no spare courage.
* **High Priest of Anubis** (lvl18), style stela:
  T: Here rests the chief lector of Anubis, who read the spells over the dead.
  T: His mouth was opened, that he might speak them still.
  T: The words of his mouth are curses, and the dead rise when he speaks.
  N: A priest who reads curses for a living. Or for a dying. Mind the purple.
  (Brings in the Opening of the Mouth, absent from every text today.)
* **Scorpion queen** (lvl22), style column:
  T: Seven scorpions walked with Isis; Tefen and Befen went before her.
  T: The eldest sits in the deep of the nest, and her eggs are many.
  T: Break the eggs, and her children are few.
  N: Isis had seven scorpions as bodyguards. One of them seems to have retired here.
  ([lore-additions](solved/lore-additions.feature.md) named the seven; this does not rename the boss.)
* **Rerek** (lvl41), style stela, after Book of the Dead spell 39 (paraphrase, mark it so in the comment):
  T: Get back, Rerek, crawl away! Your coils are cut, your head is off.
  T: You swim in the sand, but the sand will not hide you.
  T: Stand on the stone, and he must come up to bite.
  N: A spell against the sand serpent. Shorter than I hoped. The stone part I like.

Then `docs/levels.md` (the level table rows of lvl4, 13, 18, 22, 41 list their inscriptions), and
`bin/levelcheck content/levels/lvl*` with no warnings.

## Open questions

* Sample wording and hints are the implementer's to tune against the bosses' rules (Khepri summons scarabs; the
  Mother of Webs puts out torches; Rerek burrows and swallows; check each).

## Play-test

* `bin/game --level 4` (and 13, 18, 22, 41): read the new inscription before the boss (Words of Thoth needed: a play
  script with `give` or the skill set), see it in the journal (J, Inscriptions).
