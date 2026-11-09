# Offering gift: a shrine god sometimes answers an offering with a gift

Status: implemented 2026-11-09, waiting for the user's play-test. Was: draft 2026-11-09, size M, needs nothing first.

## Idea

From the user. An offering at a shrine ([shrine-offerings](solved/shrine-offerings.feature.md),
[offering-xp-scaling](solved/offering-xp-scaling.feature.md)) gives XP. Now, about 1 offering in 10, the god also
gives a random gift on top of the XP. Like the [inscription boons](solved/inscription-boons.md), it is a lottery:
nothing hints at it beforehand.

Gifts (the user; full heal dropped 2026-11-09):

* max health bonus
* max stamina bonus
* Armor +1
* Might +1
* a random item

## Plan

* Where: `Inventory::Offer` (`src/ui/inventory.cpp`), after the XP. Merges give no gift (they are not offerings).
* Chance: a flat 10 % per offering, whatever is offered (the user, 2026-11-09, "for now"). One constant, so a
  weighted chance can come later.
* Permanent stats reuse the boons: `Boon` (`src/level/inscriptions.h`) and `PlayerStats::Grant`. Max health
  +5 % and max stamina +5 % (half a boon, so inscriptions stay worth reading), Might +1, Armor +1.
* Random item: rolled like a treasure container's item for the current depth (the treasure roll in the level
  builder), into the bag. No room for it: roll another gift.
* Each god has its own gift chances (the user). Starting weights, the implementer may tune them:

  | God | Might | Armor | Max health | Max stamina | Item |
  |---|---|---|---|---|---|
  | Montu | 50 | 10 | 15 | 15 | 10 |
  | Ptah | 10 | 50 | 15 | 10 | 15 |
  | Isis | 5 | 15 | 50 | 15 | 15 |
  | Serket | 15 | 10 | 15 | 50 | 10 |
  | Amun | 10 | 10 | 10 | 10 | 60 |

  A table next to `godDef` (one row per god, weights in percent, a unit test checks each row sums to 100).
* No save-scumming with quick save (F5 offer, F9 load, offer again): the roll is deterministic, seeded from the run
  and a saved offering counter (`PlayerStats::Offered` already counts), so a reload gives the same result.
* Feedback: the toast "Montu accepts the offering. +250 XP" gets the gift: "... Montu grants a gift: Might +1"
  (`boonText`), "Amun grants a Small Health Potion". The god's prayer sound already plays; a gold
  flash or a second sound for a gift is optional.
* Glossary: add "offering gift" (or extend the offering row). Journal: optional note on the first gift.
* Tests: unit tests for the roll (10 % chance, each god's weights, determinism by counter, every kind reachable, a full bag rerolls the item),
  a play script `tests/play/offering-gift.txt` at a shrine with spares (a test-only forced gift helps, like a
  `god`-style setup command).

## Implementation notes

* `RollOfferingGift` (`src/level/offering_gift.h`), the weights `GIFT_WEIGHTS` there (indexed by God), as drafted.
  Called from `Inventory::Gift` after the XP; `RunStats::offerings` counts the offerings (saved, "OFFERINGS n").
* The item: `RollTreasureItem` (`loot.h`), levelgen's chest roll moved out of the level builder (same draws, the
  levels unchanged), at `campaignDifficulty(level)` (`campaign.h`, the rebuild's 1-to-10 ramp).
* "A full bag rerolls the item" left out: the bag has no limit, an item always fits.
* Play scripts and scenarios: `gifts on` makes every offering bring one.

## Play-test

`bin/game --play tests/play/offering-gift.txt` (a shrine of Amun, 10 small health potions, every offering a gift).

* Walk right to the shrine, I, select the potion, O and Enter several times.
* Each toast reads "Amun accepts the offering. +N XP. Amun grants ..." with a stat ("a gift: Might +1") or an item.
* The stat rises in the inventory's bottom bar; an item gift shows in the bag.
