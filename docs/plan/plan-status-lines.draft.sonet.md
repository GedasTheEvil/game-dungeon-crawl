# Solved plans: stale status lines

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Docs only: solved in the same commit once done.

About 15 plans in `docs/plan/solved/` still say "waiting for the user's play-test" / "not play tested" / "to be
play-tested" on their first Status line, though the "Solved:" commits show the user confirmed them. Examples:
amulet-extras, amulet-upgrades, apep-serpent-boss, giant-cobra, inventory-overhaul, inventory-sort-orders,
resistance-potion, poison-drip-trap, trapped-chests, campaign-levelgen, campaign-texts, denser-levels, monster-balance,
frame-rate-cap. `sprint-toon-blur-and-extras.md` says "decided, not implemented", but toon blur is in
`src/graphics/ink.cpp` and `motion_fx.cpp`.

* Grep every solved plan's first Status line; rewrite the stale ones to "solved <date> (play-tested by the user)",
  the date from `git log` of the commit that moved it.
* Consider making `tools/plan.py move <slug> solved` rewrite the Status line itself, so it can't go stale again.
