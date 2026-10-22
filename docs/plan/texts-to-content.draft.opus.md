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
