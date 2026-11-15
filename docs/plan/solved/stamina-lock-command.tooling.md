# Scenario command stamina: unlimited stamina switch

Status: solved 2026-11-15. Was: draft 2026-11-15, size S, needs nothing first.

## Problem

Scripts with many power hits, sprints or jumps run out of stamina and have to wait for it. `tire N` only drains it;
nothing keeps it full.

## Plan

* `stamina inf|off` (`src/level/scenario_script.cpp`, next to `tire`): `inf` keeps the player's stamina full (every
  spend is refilled at once, as `god` cancels damage); `off` ends it. Off by default; a level load keeps it, as `god`
  does.
* `stamina N` sets the stamina to N straight away (clamped to the max), for tests of the low-stamina rules.
* `tire N` still works while `inf` is on only for one tick (it refills next tick); the docs say so.
* HUD and the stamina potions behave as usual (the bar just stays full).
* Docs: the command row in `docs/testing.md`.
* A scenario: `stamina inf`, ten sprint seconds and five power hits, `expect stamina ==` the max.
