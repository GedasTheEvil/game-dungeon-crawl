# Amulet upgrades

Status: draft 2026-10-07, refined 2026-10-09 (decided, not implemented). Split off [amulets.md](solved/amulets.md).

## Why not "3 of a kind"

A campaign has about 322 treasures (`./levelcheck`, levels 1-30). At 6% a chest holds an amulet: about 19 per run,
spread over 9 types x 2 tiers (lesser, minor), about one each. Bosses add 6 (normal or grand, 10 types). Three of one
type and tier almost never meet; even a pair is rare. So spares of one type pool their value, whatever the tier.

## Decided (2026-10-09)

* **Same-type points.** An amulet is worth lesser 1, minor 2, normal 4, grand 8 points. Upgrading one to the next
  tier costs that tier's worth: the amulet itself plus as many points again from spares of **the same type**.
  * Lesser -> minor: + 1 lesser.
  * Minor -> normal: + 1 minor, or 2 lessers.
  * Normal -> grand: + 1 normal, or 2 minors, or 1 minor + 2 lessers, ...
* Only spares of a lower or the same tier pay (a grand never pays for a normal). Spares are taken largest first; with
  powers of two that always ends exactly, no change given back.
* Regeneration has no lesser or minor: normal + normal -> grand.
* **Grand is the top**, no fifth tier.
* **Where:** the inventory's Amulets tab, an Upgrade action on the selected amulet, like the weapons' Upgrade. Shown
  only when the spares pay for it. The worn amulet can be upgraded and stays worn (the new bonus applies, the health
  amulet keeps the HP share); it is never used as a spare.
* **Drop rate:** the chest amulet chance goes from 6% to 10% (about 32 per run). Tier mix unchanged (lesser only up to
  level 10, then minor 40% / lesser 60%). Bosses unchanged.

## Notes for the implementer

* No save format change: an upgrade only changes counts of existing `ItemKind`s.
* `RollChestAmulet` (`src/world/loot.cpp`) for the 10%; `loot_test.cpp` follows.
* `ItemBag::CanUpgrade` / `Upgrade` already exist for weapons: extend them for amulets, or a sibling pair.
* Tests: unit (each tier step, mixed spares, the worn one upgraded and kept worn, not enough points, grand refused,
  regeneration), a scenario in `tests/scenarios/`.
