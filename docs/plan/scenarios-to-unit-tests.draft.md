# Scenarios to unit tests

Status: draft 2026-10-09 (idea, not decided). From the user: scenarios that don't need a screenshot move to unit
tests. Expected result: a faster test run, fewer resources.

## Today

* 106 scenarios in `tests/scenarios/` ([../testing.md](../testing.md)). Each starts the game under Xvfb, so
  `make test` is the slow part (~7-8 min for 97 at the time of [sim-unit-tests](solved/sim-unit-tests.md)) and
  loads the machine (`JOBS`, `RESERVE_CORES`, `MIN_FREE_MB`, `MIN_SWAP_FREE_MB`).
* 19 doctest files in `tests/unit/` link `liblevel.a` and `libbase.a`: no window, no GL, fast.
* 10 scenarios take no screenshot. Many others take one or two only as a debugging aid, and check the behaviour with
  `expect` lines (e.g. `weapon_hotkeys.txt`: 2 screenshots, 22 expects; `amulets.txt`: 3 and 27).

## Idea

* Sort each scenario into one of three groups:
  1. **Behaviour only:** no screenshot, or screenshots nobody needs to look at; all checks are `expect` lines on
     rules in the library (stats, items, damage, loot, water, progression). Move to a unit test, delete the scenario.
  2. **Behaviour over code still in the game binary:** monster AI, missiles, mechanisms, boss. Blocked until the rules
     leave the GL files ([sim-unit-tests-monster-rules](sim-unit-tests-monster-rules.draft.md)). Stay scenarios for
     now.
  3. **Visual or whole-game:** HUD, screens, model / animation, filtering, aspect, smoke, input path end to end. Stay
     scenarios.
* A moved test checks the same thing as the scenario's `expect` lines, through the library's public interface.
* Some scenarios mix both: split them (the rule to a unit test, the look stays a trimmed scenario).

## Open

* The input path: a scenario presses keys (`tryAttack`, hotkeys). A unit test calls the rule. Keep one end-to-end
  scenario per input feature, or trust the unit test?
* Scenario-only helpers (`src/test/scenario_fields.cpp` getters): if a unit test needs the same state, expose it
  from the library, not via the scenario layer.
* Measure: `make test` time and peak memory before and after.
* Docs: [../testing.md](../testing.md) ("Test the rules there first") and `AGENTS.md` ("To check game behaviour ...
  write a script in `tests/scenarios/`"): point behaviour checks to unit tests, scenarios to visuals.
