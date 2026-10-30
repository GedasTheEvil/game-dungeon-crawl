# Weapon compare: attack speed

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

## Problem

The inventory's weapon comparison shows only Damage and Range differences (`src/ui/inventory.cpp`, compare
rows). Attack speed (`WeaponMotion::AttackMs`) is never shown, so a slow axe and a fast dagger look alike.

## Plan

* Add a "Speed" row (attacks per second, or "fast / medium / slow" words like the damage words) to the item
  details and to the comparison, with the same better / worse colouring.
* For ranged weapons, the reload / draw time if it differs.
* Update the inventory screenshot scenario.
