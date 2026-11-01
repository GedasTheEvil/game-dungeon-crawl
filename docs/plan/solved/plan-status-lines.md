# Solved plans: stale status lines

Status: solved 2026-11-01. Was: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

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

## Done 2026-11-01

* 53 solved plans whose first Status paragraph did not say solved (stale "waiting for a play-test", "not
  implemented", or still "draft ...") now start "Status: solved <date> (play-tested by the user). Was: ...", the old
  text kept as history. The date is the commit that added the file to `solved/`. `formatting-fixes` and
  `campaign-zones` (no play-test, solved straight away) got no "(play-tested by the user)". Status lines that already
  said done / verified / play-tested were left as they are.
* `tools/plan.py move <slug> solved` now rewrites the Status line the same way (`mark_solved`), with today's date;
  the agent adds "(play-tested by the user)" when the user confirmed it (docs/plans.md, Solved).
