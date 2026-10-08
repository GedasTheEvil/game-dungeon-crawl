# Resistance potion

Status: draft 2026-10-06. Split off [poison-and-antidote.md](solved/poison-and-antidote.md): the antidote only
cures, it gives no protection afterwards. The amulets moved to [amulets.md](solved/amulets.md).

## Decided (2026-10-08)

Two potions, both cut the poison damage the player takes for 2 minutes:

| Potion | Poison damage taken | Lasts |
|---|---:|---:|
| Lesser resistance | -50% | 2 min |
| Greater resistance | -95% | 2 min |

* Differs from the poison warding amulet ([amulets.md](solved/amulets.md)): the amulet is a chance not to be
  poisoned at all; the potion makes a poison that lands hurt less. They stack: the amulet rolls first, the potion
  cuts what gets through.
* Differs from the antidote: the antidote ends a poison, the potion protects ahead of it.

## Open (proposed defaults)

* Already poisoned when drunk: the cut applies to the running poison too (yes).
* Drinking another: the timer restarts at 2 min; greater replaces lesser, lesser does not cut a running greater
  short.
* Where they drop: lesser from the first poisoners on (scorpions), greater from the cobras / giant scorpion levels on.
  Chests and loot tables as the other potions; the level checker counts them like the antidote?
* HUD: an icon with the time left, as other timed effects. Journal note per potion.
* Model and icon (two colours of one bottle, as other potion tiers).
