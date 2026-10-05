# Held weapon in toon mode

Status: draft 2026-10-05.

## Bug

In toon shading mode (F1) the monsters and the archaeologist are drawn 1.2 times bigger (`Ink::figureScale`), but the
weapon in the player's hand looks too small next to them. Decided 2026-10-05: in toon mode the held weapon is drawn 1.5
times its normal size (not just the figures' 1.2).

## Where to look

The code seems to scale it already, so find out why it does not show:

* `drawWeapon` (`src/graphics/draw.cpp`): `length = weapon->scale * Ink::figureScale()`, used for the grip position.
* `Item::Draw` (`src/entities/item.cpp`): `drawScale = scale * Ink::figureScale()`.
* Maybe one of them cancels the other, or the held weapon is drawn through another path than `Item::Draw`. Compare
  screenshots with toon on and off (`toon on|off` in a scenario) and measure the weapon against the player.

## Check

A scenario with the club, sword, spear and bow held, toon off and on: in toon mode each weapon is 1.5 times its size
with toon off, still in the fist (grip position, `WeaponMotion::grip`), and the bow's arrow matches the bow.
