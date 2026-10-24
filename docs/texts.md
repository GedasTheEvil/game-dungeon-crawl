# Texts

The game's longer prose is in `content/texts/*.txt` ([plan](plan/texts-to-content.md)): the journal's
field notes, diary and expedition pages, the creature moves and monster notes, and the lore lines of the skill and
item cards. The game reads these files once (`gameTexts`, `world/texts.*`). The unit tests and the tools read the
same files from the repo root. To change the wording, edit a file: no rebuild is needed. A new text also needs code
that asks for its key.

| File | Texts | Asked for by |
|---|---|---|
| `journal.txt` | `[note.<slug>]` the field notes (`title`, `text`), in `FieldNote` order; `[diary.<page>]` the prologue (`title`, `p1`..`p3`) | `fieldNoteText`, `diaryText` (`world/journal_text.cpp`) |
| `expedition.txt` | `[page.<n>]` the earlier expedition's notebook (`heading`, `text`), n the Page tile's value | `expeditionPageText` |
| `moves.txt` | `[move.<move>]` one sentence per `CreatureMove`: `text`, or a variant chosen by the monster's rules (a summon's kind, a curse's name, one monster's own) | `creatureMoveNote` |
| `monsters.txt` | `[monster.<label>]` `note`: the journal description after the first kill | `monsterNote` |
| `skills.txt` | `[skill.<name>]` `lore`: the Heka card's details under the effect | `skillLore` (`world/skills.cpp`) |
| `items.txt` | `[item.<label>]` (an amulet: `[item.<model>]`) `lore1`, `lore2`: the inventory card's two lines | `itemText` (`world/items.cpp`) |

Rows whose names are in code (monsters, skills, items) are keyed by their name as `textSlug` writes it: lower case,
a dash for each space or `_`, apostrophes dropped (`Serket's favour` -> `serkets-favour`, `giant rat` ->
`giant-rat`).

Names, labels, the editor's hints, the effect lines, the curses' status lines and the zone titles stay in code. These
are short, and tests, the editor and the scenario commands match them. Inscriptions, riddles and royal names have
their own files ([inscriptions](inscriptions.md), [riddles](riddles.md), [names](names.md)).

## File format

Blank lines and `#` comment lines are ignored. A `[section]` line starts a group. Each `field: text` line under it is
the text of the key `section.field`. An indented line (tab or spaces) continues the text above it, joined with one
space. A long text therefore wraps like a paragraph.

```
[note.sand]
title: Sand
text: Sand. Underground. Somebody left a door open, three thousand years ago. I wade through the drifts at three
	quarters of my pace, and running in it is out of the question. Jumping still works.
```

Sections and fields use `a-z`, `0-9`, `-` and `.`. Use plain ASCII: the fonts have no other characters. The game logs
problems to `game.log` as `file:line: message`, and the unit tests fail on them. Examples: a line outside a section,
a key that is there twice, a field without its colon. A key the code asks for that has no text shows as `[key]`.
The unit test `texts_test.cpp` asks for every key and fails on a missing one.
