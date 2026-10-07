# Amulets

Status: done 2026-10-07, play-tested by the user. Split off
[resistance-potion.draft.md](../resistance-potion.draft.md). See [Done](#done).

## Idea

* A new item kind. **One amulet worn at a time**; each gives one bonus for as long as it is on.
* Where found: lesser and minor from chests (a random chance), normal and grand only from bosses (random amulet).
* Swapping: anytime in the inventory, no cooldown (kept simple for now).
* Poison vs antidote: the antidote cures poison; the poison amulet only lowers the chance of being poisoned in the
  first place (today a poisoner's hit always poisons). So the poison amulet does not count toward the level checker's
  antidote rule.
* Duplicates allowed: the player can have more than one amulet of the same type and tier.
* Later: [amulet upgrades](../amulet-upgrades.draft.md) (combine or boost), [amulet of venom](../venom-amulet.md)
  (poison monsters on hit).

### UI (decided 2026-10-07)

* Its own Amulets tab in the inventory ([inventory-overhaul.md](inventory-overhaul.md)), the details panel
  text per amulet.
* The worn amulet's icon on the HUD.
* The worn amulet and the carried ones are saved with the game.

### Tiers (decided 2026-10-07)

Most amulets come in four tiers, weakest first: **lesser, minor, normal, grand**. All tiers of one amulet share one
model and differ only in the bonus (and the name). Example: the amulet of strength is an animal tooth (wolf, jackal or
similar) on a string. Names: "Lesser amulet of strength", "Amulet of minor strength", "Amulet of strength",
"Grand amulet of strength".

| Amulet | Lesser | Minor | Normal | Grand | Notes |
|---|---|---|---|---|---|
| Strength | +1 | +2 | +4 | +6 | adds to might (`PlayerStats::Might`, added to every hit) |
| Armour | +1 | +2 | +4 | +6 | adds to armour (`PlayerStats::Armor`), same numbers as strength |
| Health | +5% | +10% | +20% | +30% | more max HP; HP keeps its share of the max when put on or taken off |
| Regeneration | - | - | 1 HP/s | 2 HP/s | normal and grand tiers only; out of combat only |
| Poison | 10% | 25% | 50% | 80% | the chance to resist a poisoned hit (the poison, not the hit's damage) |
| Traps | 25% | 50% | 75% | immune | less trap damage |
| Pierce (blunt, slash alike) | 8% | 16% | 28% | 40% | less damage of one type |

* **Health amulet:** putting it on or taking it off changes the max HP but keeps the HP at the same share of it: no
  heal, no loss (full stays full, half stays half). Unlike a max HP potion (`PlayerStats::AddMaxHP` heals fully).
* **Regeneration:** the simplest rule: a flat number of HP per second, only out of combat. No cap, no % of max HP.
  **Out of combat = no monster is chasing you:** as soon as a monster has noticed you and moves towards you,
  regeneration stops (the fear of the monster). It starts again when none is chasing. A monster that cannot chase or
  attack you (rooted, stuck, lost you, out of reach) does not count: you are safe. Poisoned: no regeneration.
  Code note: `Monster::alerted` stays set once a monster has acted (it drives the health bar), so "chasing now" needs
  its own per-tick state.
* **Chance vs reduction:** a chance for effects that are on or off (poison), a % off for damage.
* **Typed instead of "physical":** a cut of all monster damage is stronger than every other amulet. One amulet per
  damage type instead (blunt, slash, pierce). Needs the monsters to deal typed damage:
  [monster-attack-damage-types.md](monster-attack-damage-types.md).

### Filled in by the implementer (2026-10-07, the user agreed to the suggestions)

* Chests: an opened chest holds an extra amulet at 6%: lesser only up to level 10, from level 11 minor 40% / lesser
  60%; any type but regeneration (it has no lesser or minor). The mimic's chest gives none.
* Bosses: a boss leaves an amulet chest instead of its weapon chest: normal or grand (grand at 2% per level: 10% at
  level 5, 60% at level 30), any of the nine types.
* Generated levels get amulets the same way (chests, bosses). `./levelcheck` does not count them (random).
* Health amulet: the bonus is a share of the base max HP (`PlayerStats::MaxHP`), so a max HP potion drunk while
  wearing it is not counted twice.
* Traps amulet: spike and death traps (`TrapHurt`), not falling rocks.
* Order in the Amulets tab (rows of 4, one type a row): strength, armour, health, poison, traps, blunt, slash, pierce,
  regeneration (normal, grand) last.
* Models: one per type, all tiers share it. Strength: a jackal tooth on a cord. The others Egyptian pendants: armour
  a bronze scarab, health a carnelian heart (ib), poison a scorpion (Serket), traps the eye of Horus, blunt the djed
  pillar, slash the tyet knot, pierce the shen ring, regeneration a green lotus.
* No journal note yet.

## Done

* Kinds: `ItemKind` `StrengthLesser` .. `RegenerationGrand` (34, after the potions; `AmuletType`, `AmuletTier`,
  `amuletOf`, `amuletKind`, `amuletBonus` in `src/world/items.h`). Level files and saves: item type 4 (`ItemType::AMULET`),
  id = the place among the amulets. Saves: `INV4` adds the worn amulet; older saves load (`ORDERED_SAVE_SLOTS`).
* Rules: `ItemBag::Use` puts on / takes off (`Worn`); `PlayerStats::Wear` applies the bonus (might, armour, max HP with
  the HP share kept, resistances), `TrapDamage` (with a hundredths carry), `Regenerate` (per tick, while
  `Dungeon::PlayerSafe`: no `Monster::Threatens` with a clear row, no poison), `Player::Poison` rolls the ward on the
  gameplay stream. Loot: `RollChestAmulet`, `RollBossAmulet` (`src/world/loot.h`).
* UI: the Amulets tab (Wear / Take off, effect text, "Worn now"), the HUD's fourth slot with the type icon and the tier
  numeral, `sounds/items/amulet.wav` (`tools/audio/amulet_sound.py`).
* Models: `items.py` `amulet_<type>`; icons in `hud_icons.py` (cells 21-29).
* Tests: `tests/unit/items_test.cpp`, `loot_test.cpp`; `tests/scenarios/amulets.txt`.

## Open

Moved to [amulet-extras.draft.md](../amulet-extras.draft.md).
