# Ranged weapons show the wrong HUD icon

Status: solved 2026-11-17 (play-tested by the user). Was: implemented 2026-11-17, waiting for the user's play-test. Was: draft 2026-11-17, size S, needs nothing first.

## Idea

From the user: a regression in the HUD's weapon slot. A sling and a bow showed a spear and an amulet.

Cause: `PlayerHud::weaponIcon` (`src/level/hud_icon.h`) is `Icon::FirstWeapon + itemIndex(weapon)`, which assumes
the atlas cells (`ICONS` in `tools/textures/hud_icons.py`: club .. spear, bow, composite_bow, sling, throwing_stick,
javelin, cells 8..20) follow `ItemKind` one to one. Plan test-weapons (81ac90c9) put `SmashTest`, `StabTest`,
`CutTest`, `FireTest` between `Spear` and `SelfBow`, and `BowTest` after `Javelin` (`src/level/items.h`). Every ranged
weapon moved 4 cells on: SelfBow → the javelin cell (looks like a spear), CompositeBow → the first amulet cell, Sling →
the second amulet, ThrowingStick and Javelin → more amulets. The test weapons show the bow .. sling cells.

## Repro

`bin/game --level N`, `give`/`equip` a sling, a self-bow, a composite bow (debug console), look at the HUD's first
quick slot.

## Expected

Each weapon shows its own icon. Test weapons show the icon of the melee/ranged weapon they stand for (smash: club or
mace, stab: spear or dagger, cut: short sword, fire: any melee, bow test: self-bow); no new atlas cells needed.

## Actual

Ranged weapons show a neighbouring weapon's or an amulet's icon.

## Plan

* `weaponIcon`: map by kind, not by offset: a `switch` or a table over every weapon kind (`WEAPON_KIND_COUNT`), the
  test weapons onto a real weapon's cell. Keep `ItemKind` as it is (save order).
* A `static_assert` or the unit test below: every weapon's icon lies in `[FirstWeapon, FirstAmulet)`, so a weapon
  added later can't spill into the amulets again.
* Fix the comment in `hud_icon.h` ("in ItemKind order") and the docstring in `tools/textures/hud_icons.py` to say the
  cells follow the real weapons only.
* Check the other user, `src/ui/inventory.cpp:39` (tab icon, fine) and anything else that draws a weapon icon
  (inventory list, loot, journal weapon pages) for the same offset assumption.

## Fixed when

* Unit test (names the plan): `weaponIcon` of each ranged weapon is its own cell (SelfBow → FirstWeapon + 8, Sling →
  FirstWeapon + 10, ...) and no weapon maps to FirstAmulet or beyond. It fails before the fix.
* Scenario screenshot of the HUD slot with sling, bows, throwing stick, javelin equipped.

## Play-test

* Equip each ranged weapon in turn: the HUD slot shows a sling, the bows, the throwing stick, the javelin.
