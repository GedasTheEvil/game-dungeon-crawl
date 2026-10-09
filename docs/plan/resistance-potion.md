# Resistance potion

Status: implemented 2026-10-09, to be play-tested. Draft 2026-10-06, split off [poison-and-antidote.md](solved/poison-and-antidote.md): the antidote only
cures, it gives no protection afterwards. The amulets moved to [amulets.md](solved/amulets.md).

## Decided (2026-10-08)

Two potions. For 2 minutes each lowers the chance that a poisoner's hit poisons the player, like the poison warding
amulet ([amulets.md](solved/amulets.md)) but for a while only. A poison that does land hurts as before.

| Potion | Chance to resist poison | Lasts |
|---|---:|---:|
| Lesser resistance | 50% | 2 min |
| Greater resistance | 95% | 2 min |

* With the amulet (10 / 25 / 50 / 80%, `AMULET_TYPES` in `src/world/items.cpp`): the percents add up, one roll
  (`player.cpp`, the amulet's `rng.percent`). Lesser potion + an amulet of 20% = 70% to resist. Capped at 100%:
  greater potion + any amulet, or lesser potion + grand amulet, makes the player immune while it lasts.
* Already poisoned when drunk: the greater potion also cures it (as the antidote); the lesser one does not, the
  running poison goes on.
* Differs from the antidote: the antidote only ends a poison, the potion keeps one from landing.

## Implemented (2026-10-09)

* `ItemKind::LesserResistance`, `GreaterResistance` (potion ids 8, 9) after the antidote. The amulets moved two slots
  down: the bag's save is `INV5`, an `INV4` save is read with its amulets shifted.
* `PlayerStats`: the potion's percent and time left (`RESIST` line after the poison in the save);
  `PoisonResistPercent` = amulet + potion, at most 100. Ends on death. Message on a ward: "You resist the poison".
* Drinking another: the timer restarts at 2 min; greater replaces lesser; a lesser one while a greater one runs is
  refused ("A stronger resistance runs"). The greater one says "Cured." only when a poison ran.
* Drops: placed by hand, a lesser one on levels 3-6, a lesser and a greater one on each of 15-30. Not in levelgen
  (no poisoners there), not mimic loot (as the antidote). The checker does not count them: it still wants an antidote.
* HUD: a stone tile right of the poison tiles, a blue drop (sky blue lesser, lapis greater; one or two pips) and the
  seconds left.
* Journal: one note "Resistance draughts" for both (one note per potion group, as health and stamina).
* Model: the cobra vial of the antidote, two liquid textures (`POTIONS` in `tools/blender/models/items.py`).
* Tests: unit (gain, block, stacking, save, `INV4` load), scenario `tests/scenarios/resistance_potion.txt`
  (`expect resist`).
