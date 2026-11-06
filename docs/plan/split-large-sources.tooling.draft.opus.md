# Split the four largest source files at their seams

Status: draft 2026-11-06, size L, needs nothing first.

## Problem

Four files are much larger than the rest (the next largest is under 1100 lines). Agents read them in pieces, edit
them often, and two tasks touching them at once conflict in merges:

| File | Lines | Shape |
|---|---|---|
| `src/world/level_gen.cpp` | 2124 | one class, `LevelBuilder` (lines ~146-2045), in an anonymous namespace |
| `src/world/level_check.cpp` | 1691 | `Nodes` and `Walker` (the path model, ~160-520), ~25 `check*` functions (~560-1380), `checkLevel` (~240 lines), the glyph helpers |
| `src/ui/menu.cpp` | 1415 | `// ----` sections: layout, icons, options, save slots, drawing (`MainMenu`) |
| `src/ui/journal_view.cpp` | 1392 | sections: layout, pages, ribbons, page turns, input, drawing (`JournalScreen`) |

## Plan

Split each file along the seams it already has. Move code only: no renames, no behaviour changes. Do one commit per
file and add each to `.git-blame-ignore-revs`.

* `level_check.cpp`: `level_walker.cpp` (+ an internal `level_walker.h`: `Nodes`, `Walker`, `Edge`), the
  per-feature checks grouped by subject (for example `level_check_dressing.cpp`: props, decals, containers;
  `level_check_mechanisms.cpp`: locks, teleporters, name gates, sun beams, wall spikes, gusts), `level_check.cpp`
  keeps `checkLevel`, the score and the glyph helpers. Shared helpers go in an internal header
  `level_check_internal.h`.
* `level_gen.cpp`: move `LevelBuilder` into an internal header and spread its members over 3-4 files by phase
  (layout and rooms, locks and gates, monsters and items, dressing). Pick the cut from the member functions' own
  grouping. If a clean cut needs more than moving code, leave this file and say why here.
* `menu.cpp`: `menu_options.cpp` (the Options rows, the key cells), `menu_icons.cpp` (`drawIcon`, `drawMouse`),
  `menu_slots.cpp` (save and load slots); `menu.cpp` keeps `MainMenu`'s frame and input.
* `journal_view.cpp`: `journal_pages.cpp` (each section's page drawing), `journal_view.cpp` keeps the book, ribbons,
  page turns and input.
* The level library list in the makefile (`LEVEL_LIB_SOURCES`) gets the new `level_*` files. Docs:
  development.md "Where is X" and Libraries, ui.md, levels.md: each row points at the new file.
* `make check`, `make paths` (the checker must give the same report: diff `bin/levelcheck content/levels/lvl*`
  output before and after), and `make test` on the scenarios that cover the menu and the journal.

## Open points

* If [src-dirs-by-layer](src-dirs-by-layer.tooling.draft.opus.md) comes first, the new files go straight into
  their new directories. The two plans can go in either order. Don't run them at the same time.
* File names: check [glossary.md](../glossary.md) and use its terms.
