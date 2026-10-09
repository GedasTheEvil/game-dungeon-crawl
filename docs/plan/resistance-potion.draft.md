# Resistance potion

Status: draft 2026-10-06. Split off [poison-and-antidote.md](solved/poison-and-antidote.md): the antidote only
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

## Open (proposed defaults)

* Drinking another: the timer restarts at 2 min; greater replaces lesser, lesser does not cut a running greater
  short.
* Where they drop: lesser from the first poisoners on (scorpions), greater from the cobra / giant scorpion levels on.
  Chests and loot tables as the other potions. Does the level checker count them like the antidote?
* HUD: an icon with the time left. Journal note per potion.
* Model and icon (one bottle in two colours).
