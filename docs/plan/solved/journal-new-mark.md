# Journal: mark new entries

Status: solved 2026-11-04 (play-tested by the user). Was: implemented 2026-11-04, waiting for a play-test (`bin/game --level 1`: the first rat, the HUD's book tile). Was: draft 2026-10-30, from the project audit of 2026-10-30.

## Problem

The journal gives no sign of new content. Only an inscription read posts "Journal updated"
(`src/world/dungeon_interact.cpp`). A new creature page, a creature turning from sketch to photo, new health or
weakness facts and new field notes appear silently.

## Plan

* Track "seen" per journal entry (saved with the journal).
* A small mark on the HUD journal hint / the J tab while there is something unseen, and a mark on the section
  ribbon and on the entry itself until the page has been shown.
* A short status line the first time a creature's facts are filled in ("Journal: rat weaknesses noted").

## Done

* `Journal` keeps a set of new entries (`JournalPart` and a key that stays when entries are added: creature type,
  note, diary page, expedition page number, the index for riddles and inscriptions); saved (journal version 14),
  older saves load with nothing new. New: every new entry; a creature's kill, first hit taken, move seen, weapon type
  or venom tried; an inscription translated. Not: a riddle's hint or answer (the player was there), the log.
* Marks (`ui::newMark`): red dot and "new" in the page's top inner corner, a dot on the section's ribbon, on the J
  screen tab, and a book tile right of the HUD panel with the Journal key while anything is new.
* Seen once the spread is left: a page turn, another screen tab, the book closed (`JournalScreen::SeeShown`).
* Status line on the first kill of a kind: "Journal: <name>, photo and HP noted"; weapon types and venom tried
  already had theirs ("Journal: rat, weak to blunt"). Moves stay silent (mid-fight; the marks show them).
* Tests: `tests/unit/journal_test.cpp` (new marks, page inserted in front, save and load), scenario
  `tests/scenarios/solved/journal_new_mark.txt` (HUD tile, page, ribbon and tab marks, gone once all is read).
