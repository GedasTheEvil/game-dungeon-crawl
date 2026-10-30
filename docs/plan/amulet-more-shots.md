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

## Implemented (2026-10-30)

* `AmuletType::Shots` after `Piercing`, its tiers `ShotsLesser` .. `ShotsGrand` after the piercing ones (ids 54-57),
  `AmuletBonus::shotsPercent` read by `PlayerStats::Shots` (now a member: the worn amulet; the HUD, the shot refusal
  and the scenario field all go through it). A self-bow 7, 8, 9, 10; composite bow and sling 8, 9, 11, 12; stick 4,
  5, 6, 6; javelin 3, 3, 4, 4.
* "Satet's Quiver", label "amulet of shots", effect "Ranged weapons hold N% more shots"; lore and journal note.
* Model `amulet_shots` (items.py, not amulets.py: the amulets still live there): a tilted leather quiver with gold
  bands, three fletched arrows. HUD icon in the atlas after the arrowhead (curses now from cell 37).
* Loot: the chests and the mimics roll every type, so it comes in with no change. By hand: lvl24 (13 giant cobras)
  a lesser one (it was a second minor cleaving), lvl32 (cobras, crocodiles, hippos) a minor one (a third minor
  cleaving); level checker clean.
* The HUD pips: more than 6 shots go in two rows (12 did not fit one).
* Taking the quiver off with more shots spent than the weapon now holds: none left until the extra ones are back
  (they come back at the normal pace). No refill trick by swapping.
* Balance: `bin/balance table --gear play --amulet 57` (the new `--amulet ID` option, worn on top of the gear): the
  kite loses less (giant cobra 54% -> 29%, hippo 12% -> 8%) but stays slower and costlier than the tank wherever it
  was before.
* Tests: `tests/unit/amulet_shots_test.cpp` (every tier for every ranged weapon, four javelins in the world),
  `tests/scenarios/amulet_shots.txt` (cards, icons, the pips). Play: `bin/game --play tests/play/amulet-more-shots.txt`.
