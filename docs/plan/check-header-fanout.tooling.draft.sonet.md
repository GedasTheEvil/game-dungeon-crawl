# Split gameplay_config.h: it is in 257 of 315 translation units

Status: draft 2026-11-16, size M, needs nothing first. Found by [check-build-header-rebuild](solved/check-build-header-rebuild.tooling.md).

## Problem

A real edit of `src/level/gameplay_config.h` (660 lines, 12 commits lately) recompiles about 255 objects, 189 s idle.
ccache does not help for a semantic change. Includers per header (`.d` files, 315 translation units): gameplay_config.h
257, core/paths.h 248, level/level.h 247, damage.h, poison.h, rgb.h, tier_drain.h 239, decor.h 238, skills.h 228,
curses.h 227, items.h 226, terrain.h 222.

## Plan

* Split `gameplay_config.h` by topic (per system: monsters, items, skills, ...), so a tuning change touches the few
  files that read that part. Keep one umbrella include only where a file needs most of it.
* Check `core/paths.h` and `level/level.h`: forward-declare or move what few files use.
* Measure: a real edit of one constant, cold build time before and after.
