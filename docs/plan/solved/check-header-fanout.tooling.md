# Split gameplay_config.h: it is in 257 of 315 translation units

Status: solved 2026-11-16. Was: draft 2026-11-16, size M, needs nothing first. Found by [check-build-header-rebuild](check-build-header-rebuild.tooling.md).

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

## Result (2026-11-16)

`level/gameplay_config.h` (660 lines) is gone. Its numbers sit in `level/gameplay_<topic>.h`, each included only by the
files that read it (include-what-you-use, by identifier). No file needs most of them, so no umbrella header.

| Header | Holds | Objects including it |
|---|---|---|
| `gameplay_shared.h` | numbers that inline code in widely included headers reads (`player.h`, `monster.h`, `terrain.h`, `damage.h`, `monster_kinds.h`, `bleeding.h`, ...): `UPDATE_TICK_MS`, `WALK_SPEED`, terrain rows, the bite reach, ... | 251 |
| `gameplay_trap_damage.h` | the trap hurt numbers `trap_hurt.h` reads, `FULL_TRAP_SHARE`, fire vent | 178 |
| `gameplay_player.h` | the player's body, crouch, drag, walk, ladder, jump, fall | 114 |
| `gameplay_world.h` | gates, cut-scenes, ring, Stargate, name gate, mirrors, death screen | 104 |
| `gameplay_monster_kinds.h` | per kind: shots, burrowers, chargers, holds, bats, climbers' kin | 50 |
| `gameplay_monsters.h` | walkers, jumpers, kiters, climbers, terrain affinity rest | 32 |
| `gameplay_skills.h` | skills (power hit, stun, bleed, flaming arrows), fire in water, shots | 19 |
| `gameplay_traps.h` | rock fall, darts, plates, trapped chests, drips | 17 |
| `gameplay_rhythm_traps.h` | wall spikes, sand gusts (`spike_cycle.h`) | 16 |
| `gameplay_environment.h` | half water, sand, quicksand, sand fall | 14 |
| `gameplay_head_bump.h`, `gameplay_combat.h`, `gameplay_bleeding.h` | head bump, bow and melee reach, bleed sources | 6, 5, 1 |

Of 312 translation units. A number a widely included header reads stays wide by nature (`gameplay_shared.h`,
`gameplay_trap_damage.h`); put a new number in a topic header, in `gameplay_shared.h` only when such a header needs it.

* `core/paths.h`: `decor.h` read it for two texture paths only used by `assets.cpp`; they moved there. Includers
  248 to 62. The rest (`zone_music.h`, `inscription_layout.h`) read `MEDIA_DIR` in a constant.
* `level/level.h`: left alone (245 includers). It is the level type itself (`Tile`, `Fixture`, `Level`) that `monster.h`,
  `dungeon.h` and most headers need; 4 commits in its life, so no rebuild pain to buy back.

Measure: edit of `DART_REARM_MS` (a trap number), `NO_CCACHE=1 make all`, the build warm before the edit:

| | Before | After |
|---|---|---|
| Objects rebuilt | 255 | 17 |
| Build time | 6 min 25 s (cold build 4 min 54 s, 312 objects; the machine was shared) | 16 s |
