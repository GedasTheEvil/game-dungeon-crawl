# Sandstorm halls

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: a level feature, visuals and level design. [desert-chapter](solved/desert-chapter.md) left the sandstorm halls out;
only a sandstorm monster or level flag exists (`monster_kinds`, `level.h`): check what it does today.

* A hall in the desert zone where wind blows sand: pushes the player and light monsters sideways in gusts, lowers
  view distance (haze), drowns sound; arrows drift. Gusts on a rhythm with a tell (rising hiss, grit), so the player
  can time crossings, like the rhythm traps.
* Shelter: pillars and alcoves block the wind.
* Place 2-3 in the desert levels; checker models the push (or treats the hall as passable with a warning-free rule).
* Sounds via `tools/audio/sand_sounds.py`.
