# Lean scenario tests: solve the old scenarios

Status: draft 2026-10-10, needs nothing first. From the user. Tooling only (no gameplay, visuals, levels or balance):
once done and `make test` passes, move it straight to `docs/plan/solved/`.

## Goal

`make test` runs 79 scenarios (`tests/scenarios/*.txt`), each a full game under Xvfb: slow. Apply the AGENTS.md
rule (a solved plan's scenarios become unit tests; screenshot-only ones go to `tests/scenarios/solved/`) to the
scenarios written before the rule. Keep tests lean and fast.

## Work

1. Time the baseline: `make test` wall time (the last lines of `tools/run_scenarios.sh`) and `make unit` time.
2. For every `tests/scenarios/*.txt`, sort it into one bucket:
   - **Unit test.** Its checks are `expect` lines on the sim (position, hp, poison, items, monsters, journal counts,
     ...) and the screenshots are only for eyeballing. Rewrite as doctest cases with `SimWorld`
     (`tests/unit/sim_world.h`), in the matching `*_test.cpp` (`sim_test`, `monster_rules_test`, `world_rules_test`,
     `items_test`, `journal_test`, ...). Then delete the scenario. Check first: an existing unit test may cover it
     already; then only delete.
   - **Parked.** Its point is the picture (drawing, lighting, HUD layout, model look, UI screens, page turn), so it
     needs a screenshot, and the plans it belongs to are solved. `git mv` to `tests/scenarios/solved/`.
   - **Split.** Both: move the sim checks to a unit test, keep a trimmed scenario with the picture part (parked if its
     plans are solved).
   - **Keep.** Belongs to an unsolved plan (`docs/plan/*.md`, draft or implemented), or is a cheap whole-game guard
     worth running every time (`smoke.txt`; at most a handful more, say why in the scenario's first comment).
3. A scenario's plans: grep its file name in `docs/plan/` and `docs/plan/solved/`. One with no plan link counts as
   solved (it predates the plans), unless it tests something a draft is changing.
4. Update links: docs, plans (also solved ones) and `docs/testing.md` that name a moved or deleted scenario. A deleted
   one links the unit test case instead. `docs_test.cpp` checks docs; run `make unit`.
5. Levels: a `tests/levels/*` used only by deleted scenarios and no unit test goes too; one now used by a unit test
   stays.
6. Time again, write both timings into this plan (before/after) when moving it to solved.

## Starting map (2026-10-10, re-check: scenarios change)

Grep of scenario names in the plans; "shots" = `screenshot` lines.

- Linked to an unsolved plan (keep for now): `anubis_bolt` (anubis-ranged-attack draft), `inventory_keys`, `keys`
  (inventory-keys), `ladder` (no-attack-on-ladder), `sobek` (sobek-boss), `summon_effects` (anubis-speed),
  `water_ceiling` (water-ceiling), `weapon_reach` (monster-balance draft).
- No screenshots at all, first unit-test candidates: `attack_recovery`, `bats_bite`, `generated_path`,
  `ladder_jump_off`, `loot`, `monster_slots`, `spikes`, `sprint`, `stamina`, `trap_hop`.
  (`generated_path` is also used by `make paths`, `tests/out/paths/`; check before deleting.)
- Likely parked (UI or visuals): `aspect_*`, `credits`, `draft_map`, `filtering`, `hud_*`, `player_hud*`, `menu`,
  `options`, `render_window`, `riddle*`, `screen_tabs`, `status_box`, `journal_page_turn`, `lighting`, `surfaces`,
  `toon*`, `props`, `weapons_held`, `blood_at_start`, `monster_blood`.
- The rest (monsters, items, potions, journal): read each; most are likely split or unit.

## Open points (implementer's choice)

- Which few whole-game scenarios stay in `make test` beside `smoke.txt`.
- Whether a pure-behaviour scenario whose screenshots show a model in motion (e.g. `sprint_motion`, `bats`) is unit
  or parked: unit if the behaviour is the point, parked if the look is.
