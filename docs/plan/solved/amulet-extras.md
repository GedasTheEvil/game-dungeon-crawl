# Amulet extras

Status: implemented 2026-10-23, waits for a play-test (`bin/game --play tests/play/amulet-extras.txt`). Drafted
2026-10-07, refined 2026-10-09. Left open by [amulets](amulets.md)
(done, play-tested).

## Decided (2026-10-09)

### Journal notes

* One note per amulet type (11 with venom, 14 with the typed might below), written the first time any tier of that
  type is found (`ItemBag::Find`, today "an amulet: none yet"). The note gives the lore and the four tiers' numbers.

### Typed might: Crushing, Cleaving, Piercing

Three new types, the warding amulets' mirror: more damage of one type.

| Amulet | Lesser | Minor | Normal | Grand | Model |
|---|---|---|---|---|---|
| Crushing (blunt) | +10% | +20% | +30% | +50% | a mace head |
| Cleaving (slash) | +10% | +20% | +30% | +50% | a khopesh blade |
| Piercing (pierce) | +10% | +20% | +30% | +50% | an arrowhead |

* Only the matching share of the weapon's damage mix grows: the mace (100% blunt) with grand Crushing +50%, the sword
  (85% slash) with grand Cleaving about +42%, the club (85 blunt / 15 slash) with grand Cleaving about +8%.
* Names: "Lesser Amulet of Crushing", "Amulet of Minor Crushing", ... (the usual tier pattern).
* Found like the others: lesser and minor in chests, normal and grand from bosses. New ids after venom, so old saves
  and levels keep theirs. In the Amulets tab after venom (the `ItemKind` order, found first).
* Models: Egyptian pendants in the amulet style (`items.py`), icons in `hud_icons.py`.

### Grand strength

* Kept at 1 / 2 / 4 / 6. +6 flat is a lot on light weapons (dagger 8 -> 14), but a grand one is rare (bosses only,
  2% per level). Watch in play.

## Dropped

* Endurance (sprint drain), for now.
* A faster escape from the crocodile's hold: the hold is short, no amulet ([crocodile-hold-bite](crocodile-hold-bite.md)).

## Done

* Typed might: `AmuletType::Crushing` / `Cleaving` / `Piercing`, `ItemKind` ids 42-53 after the fire guard,
  `AmuletBonus::typeMightPercent`, `PlayerStats::TypeMightPercent`; applied in `Dungeon::playerHit` to every weapon hit
  with a damage mix (melee and missiles), only the matching share of the mix grows. Names by their object: Seth's Mace,
  Montu's Khopesh, Neith's Arrowhead (labels "amulet of crushing" ...). Models `amulet_crushing|cleaving|piercing`
  (`items.py`), icons in cells 32-34 of the HUD atlas (grid now 8 x 6, curses from cell 35).
* Journal notes: one `FieldNote` per amulet type (`amuletNote`, after the hour notes so saved notes keep their numbers),
  written by `ItemBag::Find` on the first amulet of the type; the lore and the four tiers' numbers.
* Grand strength stays 1 / 2 / 4 / 6.
* Tests: `tests/unit/amulet_extras_test.cpp`, `tests/scenarios/solved/amulet_extras.txt` (tab, models, HUD).
