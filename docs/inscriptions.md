# Inscriptions

Hieroglyphs on the back wall of a cell that the player can read ([plan](plan/solved/hieroglyph-cartouches.md)). An
`Inscription` tile (type 15, `i` in the [level legend](levels.md)) has a text id as its attribute. It is a fixture in
the cell's wall layer ([wall-layer](plan/solved/wall-layer.md)), so the cell can also hold an object: a chest under it is
opened by the first press, the inscription read with the next. When the player interacts in that cell, the status line says the hieroglyphs were deciphered (a repeat: already seen). The first time,
the translation is written in the journal (J, Inscriptions ribbon): the hieroglyphs copied from the wall, the
translation and the comment, with the level where it was read. Reading it again writes nothing new.

The translation also gives XP ([plan](plan/solved/inscription-xp.md)): a third of a riddle's reward (`riddleXP(level) / 3`,
`INSCRIPTION_XP_PART`, at least 166), on the status line ("+N XP"). It is given once per inscription, when it is
translated: read with the hieroglyph skill, or, for a copy made without it, when the skill is learned (each copy gives
the full XP at the player's level then; the toast adds the total). Copying without the skill gives none. The save game keeps
a copy of each text, so editing the file does not change the journal.

About 1 in 5 inscriptions also hold a boon ([plan](plan/solved/inscription-boons.md)): a permanent Might +1, Armor +1, max
health +N % or max stamina +N %, given with the XP and only once. The status line and the skill's toast name it; the
journal page shows "Gift: ..." under the comment. Nothing hints at a boon before the translation. In the campaign:
at most one per level, the text names the gift (kings and Montu: Might; the djed and granite: Armor; the breath of
life: max health; tireless legs and runners: max stamina).

A name cartouche ([names](names.md), plan secret-name-gates) is an inscription with the text id 100
(`NAME_CARTOUCHE_ID`, no text id: the ids skip it): it holds the level's royal name for its name gate, not a text from the files. It is read, copied,
translated and rewarded like the others (no boon); its signs spell the name (the name's `G:` line in `content/names/`).

Inscriptions are for atmosphere first and hints second. They are never needed to finish a level. In the campaign
they stand from level 1 on, 2 to 3 per level (1 to 2 in the first zone); without the hieroglyph skill, which comes
with the first boss's heka ([skill-hieroglyphs](plan/solved/skill-hieroglyphs.md)), a copy waits unread in the journal.
`levelcheck` warns about a text id with no text, the same text id twice among the files it checks, and an inscription the player cannot
stand in front of.

The game loads every `content/inscriptions/*.txt` file at start-up. Edit the texts, styles and signs there; no rebuild
is needed. The game composes each inscription when it draws it ([plan](plan/composed-inscriptions.md),
`src/world/inscription_layout.cpp`): the **plate** of its style (the empty frame,
`media/textures/decorations/inscription_plates.png`), then a quad per sign from the **hieroglyph sheet**
(`hieroglyphs.png`, a cell per sign, its manifest `hieroglyphs.txt`), each sign tinted with its paint. A text that only
reuses signs costs no texture. A `G:` code the sheet lacks is a load error in `game.log` (and fails a unit test): rebuild
the sheet with `python3 tools/textures/inscriptions.py`. It collects the codes of every `G:` line in
`content/inscriptions/` and `content/names/`.

## File format

Blank lines and `#` comment lines are ignored. Each entry starts with its `id:` line.

```
id: 2
title: The curse of Meni
style: column
G: I3 N35A I14 N25 A2
T: The crocodile be against him in the water,
T: the snake be against him on land,
N: A crocodile in the water. Oddly specific. Noted.
```

| Line | Meaning |
|---|---|
| `id: N` | Text id, 1 to 256 (`INSCRIPTION_MAX`), the tile's attribute (100 is a name cartouche, no text: skip it). Each id once over all files. |
| `title: TEXT` | Heading of the journal page only. |
| `style: NAME` | The frame on the wall: `cartouche` (an upright royal oval, up to 5 signs), `column` (up to 7), `row` (up to 7) or `stela` (a round-topped slab with a winged sun, up to 12 in columns). Default: `cartouche`. Another name is a load error. |
| `G: CODES` | The signs by [Gardiner code](https://en.wikipedia.org/wiki/List_of_Egyptian_hieroglyphs), separated by spaces: `A1`, `M23`, `Aa1`, `N35A`. They are drawn with the Noto Sans Egyptian Hieroglyphs font (SIL Open Font License, package `fonts-noto-extra`). Signs past the style's cap are not drawn. |
| `T: TEXT` | Translation, one line each. Long lines wrap on the journal page. About six lines fit in the hand font; a longer text is written smaller (up to seven lines). |
| `N: TEXT` | The archaeologist's comment: pencilled at the bottom of the journal page only. Keep it to one short line. |
| `boon: KIND N` | Optional. The boon the translation gives: `might 1`, `armor 1`, `life 10` (max health %), `stamina 10` (max stamina %). An unknown kind or a number below 1 is a load error. |

An entry without a `T:` line is skipped. Use plain ASCII in the texts: the fonts have no other characters.

## Checking a file

The game log (`game.log`) lists every problem as `file:line: message` and the count, for example
`Loaded 45 inscriptions`. `levelcheck content/levels/lvl*` checks that every inscription in a level has a text.
