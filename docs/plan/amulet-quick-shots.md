# Amulet: shots come back faster

Status: draft 2026-10-30 (user). Needs [ranged-weaker-than-melee](solved/ranged-weaker-than-melee.md) first (the
shots and their recovery).

## Idea

A new amulet type that speeds up the recovery of the shots (plan ranged-weaker-than-melee): lesser +25%, minor +50%,
normal +75%, grand +100% (twice as fast). Both the time per shot and the delay after the last shot shrink by it.
The grand: one shot every ~1.25 s instead of ~2.5 s.

Its sibling [amulet-more-shots](amulet-more-shots.md) gives a bigger first volley; this one a steadier
fire over a long fight. Balance check: with the grand one, kiting must still not beat melee on its own (the balance
sim's kite tactic, `tools/balance`); if it does, lower the top tier.

## Changes

* A new `AmuletType` at the end of the enum, its four tier `ItemKind`s at the end of the amulets (items.h: save order),
  its `AMULET_TYPES` row, a new `AmuletBonus` field (`shotRecoveryPercent`), read where plan
  ranged-weaker-than-melee computes the recovery times.
* Name and lore: Egyptian, swiftness or the hunt, not a god another amulet already has (Montu, Neith, Seth); e.g.
  "Wings of Horus", "Bastet's Swiftness". `content/texts/items.txt`.
* Model: a new amulet model in `tools/blender/models/amulets.py` (a bowstring, a feather, a falcon wing), per
  [docs/modeling.md](../modeling.md). Tier textures like the other types.
* Loot: in the amulet chests and mimic loot like the other types, the tier by depth.
* Journal and glossary rows. Unit test: each tier's recovery times.

## Implemented (2026-10-30)

* `AmuletType::QuickShots` after `Shots`, its tiers `QuickShotsLesser` .. `QuickShotsGrand` last of the amulets (ids
  58-61), `AmuletBonus::shotRecoveryPercent` read by `PlayerStats::ShotRecoveryPace` (now a member): both times divided
  by 1 + percent. Wait after the last shot / time per shot: 1.6 / 2 s, 1.33 / 1.67, 1.14 / 1.43, 1 / 1.25 (none: 2 /
  2.5).
* "Bastet's Cat" (Bastet: no other amulet's god), label "amulet of quick shots", effect "Shots come back N% faster";
  lore and journal note.
* Model `amulet_quick_shots` (items.py): a bronze cat sitting up, gold eyes and earring, its tail round its paws on a
  gold plinth. HUD icon after the quiver.
* Loot: rolled like every type, no change needed.
* Balance check (`bin/balance table --gear play --amulet 61`, the grand cat worn on top of the gear): kiting loses
  less and is quicker (giant cobra 54% 48 s -> 36% 34 s, Seth now killed by kiting: 156% in 119 s against the tank's
  60% in 26 s) but never beats melee where it did not before (it already did against the crocodile, hippo, rerek,
  baboon, scorpion queen). The top tier stays +100%.
* Tests: `tests/unit/amulet_shots_test.cpp` (every tier's times, a javelin back in 2.25 s in the world),
  `tests/scenarios/amulet_shots.txt`. Play: `bin/game --play tests/play/amulet-quick-shots.txt`.
