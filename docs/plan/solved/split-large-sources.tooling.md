# Split the four largest source files at their seams

Status: solved 2026-11-06. Was: draft 2026-11-06, size L, needs nothing first.

## Problem

Four files are much larger than the rest (the next largest is under 1100 lines). Agents read them in pieces, edit
them often, and two tasks touching them at once conflict in merges:

| File | Lines | Shape |
|---|---|---|
| `src/level/level_gen.cpp` | 2124 | one class, `LevelBuilder` (lines ~146-2045), in an anonymous namespace |
| `src/level/level_check.cpp` | 1691 | `Nodes` and `Walker` (the path model, ~160-520), ~25 `check*` functions (~560-1380), `checkLevel` (~240 lines), the glyph helpers |
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

* If [src-dirs-by-layer](src-dirs-by-layer.tooling.md) comes first, the new files go straight into
  their new directories. The two plans can go in either order. Don't run them at the same time.
* File names: check [glossary.md](../../glossary.md) and use its terms.

## Done (2026-11-06)

After [src-dirs-by-layer](src-dirs-by-layer.tooling.md), so the files sit in `src/level/` and `src/ui/`.
One commit per file, all four in `.git-blame-ignore-revs`. Moves only; what compiling across files needed: each
file's anonymous namespace became a named internal one (`levelcheck`, `levelgen`, `menu_internal`,
`journal_view`), large members of `Walker` and `LevelBuilder` are defined out of the class.

| Was | Now |
|---|---|
| `level_check.cpp` 1691 | `level_check.cpp` 700 (checkLevel, counts, terrain and shape checks, score, glyphs), `level_walker.h` 347 / `.cpp` 240 (Nodes, Walker, reach searches), `level_check_dressing.cpp` 168, `level_check_mechanisms.cpp` 340, `level_check_internal.h` |
| `level_gen.cpp` 2124 | `level_gen.cpp` 141 (entry points), `level_builder.h` 367 (`LevelBuilder`), `level_gen_layout.cpp` 500, `level_gen_gates.cpp` 366, `level_gen_content.cpp` 632, `level_gen_dressing.cpp` 230 |
| `menu.cpp` 1415 | `menu.cpp` 613 (frame, input), `menu_options.cpp` 421, `menu_slots.cpp` 171, `menu_icons.cpp` 140, `menu_internal.h` 138 |
| `journal_view.cpp` 1392 | `journal_view.cpp` 764 (book, ribbons, page turns, input), `journal_pages.cpp` 669, `journal_internal.h` 19 |

Sameness: `levelcheck` reports, maps and path scripts over every campaign and test level byte-identical (so `make
paths` plays the same scripts with the same game: not rerun); 166 generated levels (test, every zone, boss, fork,
finale) byte-identical; the menu scenarios' 61 screenshots identical but the save time in 8; the journal scenarios'
74 screenshots pixel-identical. `journal.txt`, `journal_field_notes` and `inscription_boons` (solved scenarios) fail on
master already, the same before and after.
