# Docs: stale references and omissions found by a consistency sweep

Status: draft 2026-11-13, size S, needs nothing first.

From a mechanical docs / content / code cross-check on 2026-11-13. Everything else checked came back clean (readme
monsters, bosses, weapons, key bindings, gate gems, zone names; levels.md per-level monster counts, keys, levers,
sun discs, pages, shrines, zone ranges and bosses against all 54 level files; make targets and file paths in docs).

## Problem

* `docs/level-format.md:105`: `[Zones](#zones)` points at a heading that is not in that file (`## Single cells by hand`
  is the only other heading). The zones section is `### Zones` in `docs/levels.md:18`.
* `docs/level-format.md:66-68`: names `waterSpeed`, which no longer exists in `src/`, and per-kind swim multipliers
  ("crocodile and Sobek 2.5 times their land speed, the hippo and Seth 3, the cobras and Apep 1.25") that the code does
  not have. Swimmers have `Wading::Swimmer` and are native to water: their speed comes from the terrain affinity
  (`TERRAIN_ROWS[0]` water native speed factor 2, `src/level/gameplay_config.h:161`; `terrainFactors`,
  `src/level/terrain.h`; `MonsterKind::wading`, `src/level/monster_kinds.h`).
* `docs/levels.md` zone table (lines 56-69), "Monsters" column against `ZONE_FAUNA` (`src/level/campaign.h:116-134`):
  the table omits: web halls giant rats; priests' quarter and scorpion nest giant scarabs; serpent temple crocodiles
  and giant bats; cisterns of Sobek lotus and giant bats; necropolis giant scorpions. (The level files do place them:
  e.g. 14 giant scorpions in the necropolis.)
* `readme.md:43` trap list ("Spikes, pits, rock falls, dart plates, poison drips, wall spikes and fire vents") omits the
  sand gusts of the sandstorm halls and the ceiling spikes / ceiling death traps (`TILES` in `src/level/tile_defs.cpp`:
  `SandGust`, `CeilingSpikes`, `CeilingDeath`). The monster sentence (`readme.md:5-7`) omits tiny spiders, egg
  clusters, the mummy priest and mimic jars as kinds (the High Priest is named as a boss).

## Plan

* Fix `docs/level-format.md:105`: point the Zones link at the `#zones` anchor of `docs/levels.md` (same folder).
* `docs/glossary.md:265` (heka row) says "there are 16 skills"; `SKILL_COUNT` is 17 (`src/level/skills.h:34`, Quiver of Neith was
  added later). Fix the number, or drop it and name `SKILL_COUNT`.
* Rewrite the swim-speed sentence in `docs/level-format.md` to say a swimmer is native to water and gets the terrain
  affinity's water speed bonus (read `TERRAIN_ROWS` for the number; do not keep per-kind numbers the code lacks), and
  name `Wading`, `TERRAIN_ROWS` instead of `waterSpeed`.
* Add the omitted monsters to the `docs/levels.md` zone table (read them from `ZONE_FAUNA`; keep the column short).
* Add the three trap kinds and the missing monsters to `readme.md` in the same plain voice (one clause each). Keep it a
  pitch, not a list of everything.

## Verification

* `tools/plan.py links`, `make check`.
* Re-run the checks in [docs-anchor-identifier-checks](docs-anchor-identifier-checks.tooling.draft.opus.md) once that
  lands: the anchor and `waterSpeed` cases are what it guards.
