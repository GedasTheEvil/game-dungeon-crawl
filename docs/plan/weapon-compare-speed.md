# Weapon compare: attack speed

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30. Implemented 2026-11-01, waiting
for the user's play-test: `bin/game --play tests/play/weapon-compare-speed.txt`, open the inventory (I), select each
weapon.

## Done

* A "Speed" row in the weapon details, attacks per second with one decimal ("1.8/s", 1000 / `AttackMs()`), between
  Damage and Range. Another weapon than the one in hand shows the difference, green faster, red slower. The rows sit
  closer (2.8 instead of 3.5) to fit the fifth one between the plinth and the lore rule.
* Ranged: `AttackMs()` already holds the draw (`hitMs`) and the recovery, so the same row covers the draw / reload
  time; no extra row.
* Scenario `tests/scenarios/weapon_compare_speed.txt`: club in hand, sword, spear, bow screenshots.

## Problem

The inventory's weapon comparison shows only Damage and Range differences (`src/ui/inventory.cpp`, compare
rows). Attack speed (`WeaponMotion::AttackMs`) is never shown, so a slow axe and a fast dagger look alike.

## Plan

* Add a "Speed" row (attacks per second, or "fast / medium / slow" words like the damage words) to the item
  details and to the comparison, with the same better / worse colouring.
* For ranged weapons, the reload / draw time if it differs.
* Update the inventory screenshot scenario.
