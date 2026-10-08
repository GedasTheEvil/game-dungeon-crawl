# Amulet of venom: poison monsters on hit

Status: solved 2026-10-08, play-tested (the poison and the journal entry on a giant bat). Drafted 2026-10-07, split
off [amulets.md](amulets.md). Built on [monster-poison](../monster-poison.md).

## Idea

An amulet, tiered like the others, that gives the player's weapon hits a chance to poison the monster.

## Decided by the implementer (the user was away)

| Tier | Chance a hit poisons | Poison |
|---|---|---|
| Lesser | 10% | weak |
| Minor | 15% | weak |
| Normal | 20% | medium |
| Grand | 30% | strong |

* Name "Amulet of Venom" ("Venom I .. IV" in a slot), to tell it from poison warding. Effect line: "30% of hits poison
  (strong)". Lore: "Wadjet's rearing cobra. / Your blade grows fangs."
* Weapon hits only, melee and missiles alike (`Dungeon::playerHit` with a damage mix); a hit that kills does not roll.
  The roll is on the gameplay stream, then the monster's own resistance (`poisonResistPercent`). A poison kill is the
  player's: XP, journal, drop.
* Found like the other amulets: lesser and minor in chests, normal and grand from bosses. The chest roll skips
  regeneration (no lesser or minor tier) by index now, as venom comes after it.
* Journal: the first poisoning tried on a creature (taken or shrugged off) writes its poison resistance into the
  TRIED form, in the free cell after PIERCE: normal (0%), resists (up to 50%), tough (below 100%), immune. The status
  line says so ("Journal: Rat, no resistance to poison"); later poisonings say "Rat is poisoned". Saved in the
  journal's `tried` bits (bit 3), no save format change.
* Model: Wadjet's uraeus in gold, from the front: the flared hood with a carnelian inlay and three lapis bands, the head
  on top, the tail coiled below (`items.py` `amulet_venom`). Icon: cell 30 (`hud_icons.py` `uraeus`).

## Done

* `ItemKind::VenomLesser` .. `VenomGrand` (amulet ids 34-37, after regeneration: older saves and levels keep their
  ids), `AmuletType::Venom`, `AmuletBonus::venomPercent` / `venomTier` (`src/world/items.cpp`, `VENOM_TIERS`).
* `PlayerStats::VenomPercent` / `VenomTier`, `Dungeon::venomHit`, `Journal::TryPoison`,
  `JournalCreature::TriedPoison`, `poisonResistanceWord` (`monster_kinds.h`).
* Tests: `items_test.cpp`, `loot_test.cpp` (venom comes out of chests), `tests/scenarios/venom_amulet.txt` (the tab,
  the hit that poisons, the journal, the poison kill, the HUD).

## To test

* The chances and tiers in play: grand strong poison on 30% of hits may be too much against mid-size monsters (150 HP
  over 30 s), or too little against bosses (Sobek 1600). With [monster-balance](../monster-balance.draft.md).
