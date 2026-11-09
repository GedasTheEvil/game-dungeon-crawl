# Offerings scale with total XP, potions can be offered

Status: implemented 2026-11-09, waiting for the user's play-test. Was: draft 2026-11-09, size S, needs nothing first.

## Idea

The user: today a spare weapon offered at a shrine gives a flat 200 XP ([shrine-offerings](solved/shrine-offerings.feature.md)).
It should grow with the player: 200 + 0.1 % of the current total XP. Potions can be offered too: a lesser one for
200 + 0.1 % of total XP, a greater one for 350 + 0.2 %.
Rings and amulets by rank (I to IV): today's base plus 0.05 %, 0.1 %, 0.2 %, 0.4 % of total XP. No offering gives
less than today.

## Plan

* `offerXP(kind)` (`src/level/item_bag.cpp`) takes the player's total XP (`PlayerStats::CurrentXP()`, the total, not
  the share into the current level) and returns base + share, rounded down:
  * spare weapon: 200 + 0.1 % (`OFFER_XP_WEAPON` stays the base);
  * lesser potion: 200 + 0.1 %;
  * greater potion: 350 + 0.2 %.
  * amulet or ring: today's base (kept as is) plus a share by rank: I 0.05 %, II 0.1 %, III 0.2 %, IV 0.4 %.
    Amulet rank: lesser I, minor II, normal III, grand IV (base `OFFER_XP_PER_POINT * amuletWorth`: 100 / 200 / 400 /
    800). Ring rank: copper I, silver II, gold III, sky iron IV (base `OFFER_XP_RING`: 150 / 300 / 600 / 1200).
    One rank-to-share helper for both.
* Lesser / greater split (the user's word): lesser = SmallHealth, SmallStamina, Antidote,
  LesserResistance; greater = LargeHealth, LargeStamina, GreaterResistance, WaterBlessing, Might, Armor, Life. Keep it
  one table or a `constexpr` helper next to `isPotion`, not scattered checks. Every potion can be offered, also
  Might, Armor and Life (the user: a poor trade is the player's choice).
* `ItemBag::Spares`: a potion's spares are all its copies (nothing wears a potion), so `CanOffer` / `Offer` take
  potions. Update the comments that say "Potions: none" / "0: no offering (a potion)".
* Inventory (`src/ui/inventory.cpp`): the Offer button and the O key work on the potions tab at a shrine; the
  confirm text and the "+N XP" line show the scaled number (the confirm may name it: "Offer ... to Ptah for N XP?").
* The quick-potion slot and the empty-slot sort (plan empty-slots-last) handle an offered potion like a drunk one.
* Journal / items text that states the flat 200: update it (`content/texts/`).
* Unit tests (`tests/unit/`): offerXP at 0 XP and at e.g. 100 000 XP for a weapon (200 / 300), a lesser potion
  (200 / 300), a greater potion (350 / 550), an amulet of each rank (100 / 150, 200 / 300, 400 / 600, 800 / 1200) and
  a ring of each rank (150 / 200, 300 / 400, 600 / 800, 1200 / 1600); a potion's Spares equals its count.
* Play script `tests/play/offering-xp-scaling.txt`: at a shrine, god mode, some XP given, spare weapon, a small and
  a large potion, a spare amulet and a spare ring in the bag.

## Play-test

* `bin/game --play tests/play/offering-xp-scaling.txt`: open the inventory at the shrine.
* Upgrade the Club to level 5 first (U 4 times: no play command sets a weapon level), then offer a spare Club: the XP
  shown is 200 + 0.1 % of your total XP.
* On the potions tab, offer a small potion (200 + 0.1 %) and a large one (350 + 0.2 %); the counts drop by one.
* Offer a spare amulet and a spare ring: today's XP plus, by rank, 0.05 % (I) up to 0.4 % (IV) of total XP.
