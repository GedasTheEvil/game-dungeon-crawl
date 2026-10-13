# Inscriptions

Hieroglyphs on the back wall of a cell that the player can read ([plan](plan/solved/hieroglyph-cartouches.md)). An
`Inscription` tile (type 15, `i` in the [level legend](levels.md)) has a text id as its attribute. When the player
interacts in that cell, the status line says the hieroglyphs were deciphered (a repeat: already seen). The first time,
the translation is written in the journal (J, Inscriptions ribbon): the hieroglyphs copied from the wall, the
translation and the comment, with the level where it was read. Reading it again writes nothing new. The save game keeps
a copy of each text, so editing the file does not change the journal.

Inscriptions are for atmosphere first and hints second. They are never needed to finish a level. In the campaign
they start at level 6 (`INSCRIPTIONS_FROM_LEVEL`): the hieroglyph skill comes with the first boss's heka
([skill-hieroglyphs](plan/solved/skill-hieroglyphs.md)). `levelcheck` warns about an inscription before that
level, a text id with no text, the same text id twice among the files it checks, and an inscription the player cannot
stand in front of.

The game loads every `inscriptions/*.txt` file at start-up. Edit the texts there; no rebuild is needed. The glyphs
on the wall come from the atlas `textures/decorations/inscriptions.png`: after changing a `G:` or `style:` line,
rebuild it with `python3 tools/textures/inscriptions.py`.

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
| `id: N` | Text id, 1 to 64 (`INSCRIPTION_MAX`), the tile's attribute. Each id once over all files; the atlas has a cell per id. |
| `title: TEXT` | Heading of the journal page only. |
| `style: NAME` | The frame on the wall: `cartouche` (an upright royal oval, up to 5 signs), `column` (up to 7), `row` (up to 7) or `stela` (a round-topped slab with a winged sun, up to 12 in columns). Default: `cartouche`. Only the atlas tool reads it. |
| `G: CODES` | The signs by [Gardiner code](https://en.wikipedia.org/wiki/List_of_Egyptian_hieroglyphs), separated by spaces: `A1`, `M23`, `Aa1`, `N35A`. They are drawn with the Noto Sans Egyptian Hieroglyphs font (SIL Open Font License, package `fonts-noto-extra`). Only the atlas tool reads it. |
| `T: TEXT` | Translation, one line each. Long lines wrap on the journal page. About six lines fit in the hand font; a longer text is written smaller (up to seven lines). |
| `N: TEXT` | The archaeologist's comment: pencilled at the bottom of the journal page only. Keep it to one short line. |

An entry without a `T:` line is skipped. Use plain ASCII in the texts: the fonts have no other characters.

## Checking a file

The game log (`game.log`) lists every problem as `file:line: message` and the count, for example
`Loaded 32 inscriptions`. `levelcheck levels/lvl*` checks that every inscription in a level has a text.
