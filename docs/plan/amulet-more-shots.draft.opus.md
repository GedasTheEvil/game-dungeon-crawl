# Amulet: more shots

Status: draft 2026-10-30 (user). Needs [ranged-weaker-than-melee](solved/ranged-weaker-than-melee.md) first (the
shots).

## Idea

A new amulet type that raises every ranged weapon's shots (`WeaponDef::shots`, plan ranged-weaker-than-melee):
lesser +25%, minor +50%, normal +75%, grand +100% (double), rounded up, at least +1. A self-bow's 5 shots: 7, 8, 9,
10.

For the player who wants to kite more: wearing it means not wearing a strength, armour or ward amulet, so the choice
costs something.

## Changes

* A new `AmuletType` at the end of the enum, its four tier `ItemKind`s at the end of the amulets (items.h: save order),
  its `AMULET_TYPES` row, a new `AmuletBonus` field (`shotsPercent`), read where plan ranged-weaker-than-melee
  computes a weapon's shots.
* Name and lore: Egyptian, archery. Neith is already the piercing amulet ("Neith's Arrowhead"); e.g. Satet, the
  archer goddess of the cataracts ("Satet's Quiver"). `content/texts/items.txt`.
* Model: a new amulet model in `tools/blender/models/amulets.py` (a quiver or a bundle of arrows on the amulet's
  plate), per [docs/modeling.md](../modeling.md). Tier textures like the other types.
* Loot: in the amulet chests and mimic loot like the other types, the tier by depth. A campaign level or two gets one
  by hand where a ranged-heavy fight sits (level checker clean).
* The HUD pips (plan ranged-weaker-than-melee) show the larger count.
* Journal and glossary rows. Unit test: each tier's count for each ranged weapon.
