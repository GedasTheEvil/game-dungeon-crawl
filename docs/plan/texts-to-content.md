# Texts to content: player-facing texts out of the code

Status: draft 2026-10-21, from the user ("analyze current in-code texts, perhaps it would make sense to move some of it
outside into content/ as text files"). Needs nothing first. An analysis first, then a move where it pays: the
implementer decides what moves (see [Analysis](#analysis)). Overlaps [campaign-texts](campaign-texts.md)
(zone titles and notes): do that one first or agree on where its texts live.

## Idea

Today most player-facing text lives in C++ tables: monster names and field notes (`MonsterKind::name`, `note`,
`monster_kinds.cpp`), item names and lines (`.text`, `items.cpp`), skills, curses, journal lines
(`journal_text.cpp`), end screens, zone names. Only inscriptions, riddles and royal names are in `content/` already
(`content/inscriptions/campaign.txt`, `content/riddles/`, `content/names/`).

Moving text out would let the user (and agents) edit wording without a rebuild and keep long prose out of the rules
tables. It would also make a later translation possible. But it costs a loader, error handling and tests, and splits a
monster's rules from its lore.

## Analysis

Go through the sources and sort each group of texts:

* **Move:** long prose that only the screens read and no rule depends on: field notes, item flavour lines, skill and
  curse descriptions, journal sentences, end screen texts, zone banners and titles.
* **Keep in code:** ids, editor labels and hints (`label`, `description`, read by tools and agents), short names that
  tests and the scenario commands match, format strings with arguments, unless the loader handles them cleanly.

For each group, report: where it is, how many lines, who reads it (game, editor, levelcheck, tests), and whether a
unit test pins its wording (`docs_test.cpp`, `journal_test.cpp`).

## Shape, if texts move

* One file per group under `content/texts/` (for example `monsters.txt`, `items.txt`), keyed by the existing slug or
  label, in the plain style of `content/inscriptions/campaign.txt` (no JSON or YAML dependency).
* Loaded once at start into a table (`world/`, GL-free, so the sim tests read it too); a missing key is a unit test
  failure, not an empty string at runtime.
* The unit test harness (`SimWorld`) and the tools load the same files.
* Docs: [development.md](../development.md#where-is-x) "where is X" points to the new files.

## Analysis result

Done 2026-10-24. Format, files and keys: [texts.md](../texts.md).

**Moved** to `content/texts/`, 6 files, 354 texts. The code went from about 700 lines of string tables to the
keys. The wording is unchanged: a dump of every text before and after the move is identical.

| Group | Was in | Texts | Read by | Wording pinned by a test |
|---|---|---|---|---|
| Field notes (title, text), incl. the hour and amulet notes | `FIELD_NOTES`, `journal_text.cpp` (~215 lines) | 65 | journal | `zones_test` ("The First Hour"); length checks in `zones_test`, `amulet_extras_test` |
| Diary (prologue) | `DIARY`, `journal_text.cpp` | 2 pages | journal | `journal_test` (not empty) |
| Expedition pages | `EXPEDITION_PAGES`, `journal_text.cpp` (~80 lines) | 24 | journal | `expedition_test` ("The last page", numbering) |
| Creature moves | `creatureMoveNote`, `journal_text.cpp` (~120 lines) | 50 | journal | none |
| Monster notes | `MonsterKind::note`, `monster_kinds.cpp` | 34 | journal | none |
| Skill lore | `SkillDef::lore`, `skills.cpp` | 9 | Heka screen | `skills_test` (not empty) |
| Item lore (2 card lines) | `ItemText::lore1/2` rows in `items.cpp`, amulets per type | 38 x 2 | inventory card | none |

**Kept in code:**

| Group | Where | Why |
|---|---|---|
| Monster names, labels, editor hints | `monster_kinds.cpp` | short; the editor, levelgen, the scenario commands and tests match them |
| Item names, short names, labels, effects | `items.cpp` | labels are the editor's and the scenarios' words; effects are printf formats with tier numbers, pinned by `items_test`, `amulet_extras_test` |
| Skill names and effect lines | `skills.cpp` | the scenario command `learn` and the leaderboard match the names; the effect is one card line |
| Curse names and status lines | `curses.cpp` | short HUD lines; the scenario command `curse` matches the names; `curses_test` compares the lines |
| Zone names, gates, hour words | `campaign.h`, `campaign.cpp` | short titles put together by `zoneTitle`, `levelBanner`; the level titles are already in the level files (plan campaign-texts) |
| End screens | `ui/end_screens.cpp` | a picture (`win.png`), no text |
| Menu, options, footers, toasts, death screen and Hall of Records labels | `ui/*.cpp` | UI words next to the layout that sizes them, short |

campaign-texts overlap: its hour notes are field notes (now in `journal.txt`); its level titles stay in the level
files.

The loader (`world/texts.*`, level library, no GL) reads `content/texts/` on the first lookup. The game also calls it at
start-up and logs the problems (`Texts:` in `game.log`). `SimWorld` and the tools reach the same table through the
same calls, from the repo root. A missing key shows as `[key]` and is listed by `missingTexts()`. `texts_test.cpp`
asks for every key (all field notes, pages, moves x kinds, monsters, items, skills) and fails on a missing one.
