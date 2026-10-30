# Amulet: shots come back faster

Status: draft 2026-10-30 (user). Needs [ranged-weaker-than-melee](solved/ranged-weaker-than-melee.md) first (the
shots and their recovery).

## Idea

A new amulet type that speeds up the recovery of the shots (plan ranged-weaker-than-melee): lesser +25%, minor +50%,
normal +75%, grand +100% (twice as fast). Both the time per shot and the delay after the last shot shrink by it.
The grand: one shot every ~1.25 s instead of ~2.5 s.

Its sibling [amulet-more-shots](amulet-more-shots.draft.opus.md) gives a bigger first volley; this one a steadier
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
