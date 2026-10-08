# Journal notes per kind: potions and weapons

Status: done 2026-10-07, play-tested by the user 2026-10-08. Draft 2026-10-07, from the user.

The field notes give away too much too early. Drinking the first health potion writes the "Potions" note, and that
note already names stamina potions, Aphethamine, Stone Skin and the Elixir of Life. The first weapon found writes the
"Weapons" note, which already names the club, sword, spear and bow and all three damage types.

## Idea

Each note covers only what the player has met. Notes come in groups, and a whole group is learnt at once:

**Potions:** one note per potion group, written the first time a potion of that group is picked up (decided
2026-10-07: collected is enough, no need to drink it).

| Group | Covers |
|---|---|
| Health | small and big health potions (one small one teaches the big one too), `H` |
| Stamina | stamina potions, `0` |
| Might | Aphethamine |
| Armour | Stone Skin |
| Life | the Elixir of Life |
| Antidote | the antidote, `=` (today in the "Poison" note) |

**Weapons:** one note per damage type, written the first time a weapon of that type is found.

| Group | Covers |
|---|---|
| Blunt | the club (and a later mace) |
| Slash | the sword |
| Pierce | the spear and the bow (arrows) |

The general part of the weapons note (keys 1-4, copies in chests, upgrades with `U`) goes with the first weapon
note, whichever comes first, or into a note of its own.

## Also to check

* The other notes name things not met yet. For example, "Hit points" names a healing potion and "Stamina" a stamina
  potion: drop those names until the potion's note exists.
* The creature pages' resistance lines are already learnt per damage type (`journal_tried`). They match the weapon
  groups.

## Decided (2026-10-07)

* A potion's note is written when it is picked up, not when it is drunk.
* Old save games do not matter (they get removed): new `FieldNote` bits, no conversion of the old "Potions" /
  "Weapons" bits.

## Done (2026-10-07)

* `FieldNote` (`src/world/journal.h`): `Potions` split into `HealthPotions`, `StaminaPotions`, `Might`, `Armor`,
  `Life`, `Antidote`; `Weapons` kept for the general part (the keys 1 / 2, copies, upgrades) and joined by `Blunt`,
  `Slash`, `Pierce`. `ItemBag::Find` writes them when an item is picked up (`potionNote`, `damageNote`); drinking no
  longer writes anything.
* A weapon teaches the note of the type it deals most of (`mainType`), the same rule the creature pages use. The
  weapons' damage mixes moved from `WEAPON_DEFS` (`assets.cpp`) to `weaponMix` in `src/world/items.cpp`, so the world
  can see them.
* The other notes no longer name potions: "Hit points" and "Stamina" lost their potion sentence, "Poison" its
  antidote one (the keys H, 0 and = are in the potion notes now).
* Journal save version 5: the field notes of an older save are dropped (their numbers meant other notes); its riddles
  and creatures stay.
* Tests: unit (`world_events_test.cpp`, `journal_test.cpp`), `tests/scenarios/journal_field_notes.txt`.

Decided on the way (easy to change):

* **Mixed weapons:** only the main type's note (the club, 85% blunt, teaches blunt).
* **The starting club** teaches nothing: it is held, not found. The blunt note comes with the first blunt weapon
  picked up (the sling on lvl3 in the campaign).
* The general weapons part is its own note ("Weapons"), written with the first weapon found, beside its type note.
