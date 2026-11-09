# Offering gift: a shrine god sometimes answers an offering with a gift

Status: draft 2026-11-09, size M, needs nothing first.

## Idea

From the user. An offering at a shrine ([shrine-offerings](solved/shrine-offerings.feature.md),
[offering-xp-scaling](solved/offering-xp-scaling.feature.md)) gives XP. Now, about 1 offering in 10, the god also
gives a random gift on top of the XP. Like the [inscription boons](solved/inscription-boons.md), it is a lottery:
nothing hints at it beforehand.

Gifts (the user):

* max health bonus
* full heal (only when injured)
* max stamina bonus
* Armor +1
* Might +1
* a random item

## Plan

* Where: `Inventory::Offer` (`src/ui/inventory.cpp`), after the XP. Merges give no gift (they are not offerings).
* Chance: about 10 % per offering (the user). Open point, implementer's choice: flat, or weighted by the offering's
  worth so cheap spares can't be farmed for permanent stats. Lesser potions are the cheap spare (any copy can be
  offered): 30 small health potions at a flat 10 % are 3 permanent gifts. Suggested: lesser potion and weapon 5 %,
  greater potion 10 %, ring and amulet 10 % to 20 % by rank. Keep the average near 10 %.
* Permanent stats reuse the boons: `Boon` (`src/level/inscriptions.h`) and `PlayerStats::Grant`. Max health and
  max stamina by the boon amounts or smaller (boons give +10 %; a gift +5 % keeps inscriptions worth reading).
  Compare with the boons, the stat skills and the Montu / Ptah potions so gifts stay a side bonus. The implementer
  judges the balance.
* Full heal: only rolled when the player is injured (below max health); otherwise roll among the rest.
* Random item: rolled like a treasure container's item for the current depth (the treasure roll in the level
  builder), into the bag. A full bag or a kind with no room: roll another gift.
* The god weights the gift (suggested): Montu Might, Ptah Armor, Isis full heal and max health, Serket max stamina,
  Amun the random item. The god's own gift about half the time, the rest spread over the others.
* No save-scumming with quick save (F5 offer, F9 load, offer again): the roll is deterministic, seeded from the run
  and a saved offering counter (`PlayerStats::Offered` already counts), so a reload gives the same result.
* Feedback: the toast "Montu accepts the offering. +250 XP" gets the gift: "... Montu grants a gift: Might +1"
  (`boonText`), "Isis heals you", "Amun grants a Small Health Potion". The god's prayer sound already plays; a gold
  flash or a second sound for a gift is optional.
* Glossary: add "offering gift" (or extend the offering row). Journal: optional note on the first gift.
* Tests: unit tests for the roll (chance, no full heal at full health, determinism by counter, every kind reachable),
  a play script `tests/play/offering-gift.txt` at a shrine with spares (a test-only forced gift helps, like a
  `god`-style setup command).

## Play-test

*
