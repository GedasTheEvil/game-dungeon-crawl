# Offerings scale with total XP, potions can be offered

Status: draft 2026-11-09, size S, needs nothing first.

## Idea

The user: today a spare weapon offered at a shrine gives a flat 200 XP ([shrine-offerings](solved/shrine-offerings.feature.md)).
It should grow with the player: 200 + 0.1 % of the current total XP. Potions can be offered too: a lesser one for
200 + 0.1 % of total XP, a greater one for 350 + 0.2 %.
Rings and amulets by rank (I to IV): 100 + 0.05 %, 200 + 0.1 %, 400 + 0.2 %, 800 + 0.4 % of total XP.

## Plan

* `offerXP(kind)` (`src/level/item_bag.cpp`) takes the player's total XP (`PlayerStats::CurrentXP()`, the total, not
  the share into the current level) and returns base + share, rounded down:
  * spare weapon: 200 + 0.1 % (`OFFER_XP_WEAPON` stays the base);
  * lesser potion: 200 + 0.1 %;
  * greater potion: 350 + 0.2 %.
  * amulet or ring by rank: I 100 + 0.05 %, II 200 + 0.1 %, III 400 + 0.2 %, IV 800 + 0.4 % (base and share both
    double per rank). Amulet rank: lesser I, minor II, normal III, grand IV (the bases match today's
    `OFFER_XP_PER_POINT * amuletWorth`). Ring rank: copper I, silver II, gold III, sky iron IV (the bases drop from
    today's `OFFER_XP_RING` 150 / 300 / 600 / 1200 to 100 / 200 / 400 / 800). One rank-to-(base, share) helper for both.
* Lesser / greater split (implementer's choice if a better one shows): lesser = SmallHealth, SmallStamina, Antidote,
  LesserResistance; greater = LargeHealth, LargeStamina, GreaterResistance, WaterBlessing, Might, Armor, Life. Keep it
  one table or a `constexpr` helper next to `isPotion`, not scattered checks.
* `ItemBag::Spares`: a potion's spares are all its copies (nothing wears a potion), so `CanOffer` / `Offer` take
  potions. Update the comments that say "Potions: none" / "0: no offering (a potion)".
* Inventory (`src/ui/inventory.cpp`): the Offer button and the O key work on the potions tab at a shrine; the
  confirm text and the "+N XP" line show the scaled number (the confirm may name it: "Offer ... to Ptah for N XP?").
* The quick-potion slot and the empty-slot sort (plan empty-slots-last) handle an offered potion like a drunk one.
* Journal / items text that states the flat 200: update it (`content/texts/`).
* Unit tests (`tests/unit/`): offerXP at 0 XP and at e.g. 100 000 XP for a weapon (200 / 300), a lesser potion
  (200 / 300), a greater potion (350 / 550), an amulet and a ring of each rank (I 100 / 150, II 200 / 300, III 400 / 600,
  IV 800 / 1200); a potion's Spares equals its count.
* Play script `tests/play/offering-xp-scaling.txt`: at a shrine, god mode, some XP given, spare weapon, a small and
  a large potion, a spare amulet and a spare ring in the bag.

## Play-test

* `bin/game --play tests/play/offering-xp-scaling.txt`: open the inventory at the shrine.
* Offer the spare weapon: the XP shown is 200 + 0.1 % of your total XP.
* On the potions tab, offer a small potion (200 + 0.1 %) and a large one (350 + 0.2 %); the counts drop by one.
* Offer a spare amulet and a spare ring: by rank, I 100 + 0.05 % up to IV 800 + 0.4 % of total XP.
