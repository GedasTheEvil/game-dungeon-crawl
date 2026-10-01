# Binary level format

Status: draft 2026-10-01. Low priority: do it after all other plans are done.

## Idea

Levels (`levels/lvl*`) are plain text: a header, then one `type attr value` line per cell, about 13 KB a file.
`loadLevelFile` / `saveLevelFile` (`src/world/level.cpp`) read and write them with `std::istream >>`. A simple
binary format (a small header with a magic and version, then the cells as fixed-size records) should load faster.

## Measure first

The files are small, so the text parse may already take under a millisecond. Time `Dungeon::LoadCampaignLevel`
before changing anything. If the gain is too small to notice, close this plan without doing it.

## What else to consider

* Every reader and writer of the format: the game, `levelcheck`, the editor (`tools/editor`), the level tools
  (`tools/level`), and the scenario tests.
* Plain text diffs well in git and can be fixed by hand. Keep the text format as the source and generate the
  binary files at build time, or have the loader read both formats.
