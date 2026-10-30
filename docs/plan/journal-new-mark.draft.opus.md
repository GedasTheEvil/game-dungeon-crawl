# Journal: mark new entries

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

## Problem

The journal gives no sign of new content. Only an inscription read posts "Journal updated"
(`src/world/dungeon_interact.cpp`). A new creature page, a creature turning from sketch to photo, new health or
weakness facts and new field notes appear silently.

## Plan

* Track "seen" per journal entry (saved with the journal).
* A small mark on the HUD journal hint / the J tab while there is something unseen, and a mark on the section
  ribbon and on the entry itself until the page has been shown.
* A short status line the first time a creature's facts are filled in ("Journal: rat weaknesses noted").
