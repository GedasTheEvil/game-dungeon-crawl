# Poison damage and the antidote

Status: draft 2026-10-05, all decided 2026-10-06, ready to implement. Needed by the
[scorpion-queen-boss.draft.md](scorpion-queen-boss.draft.md) and
[apep-serpent-boss.draft.md](apep-serpent-boss.draft.md).

## Poison

A poisoned hit makes the player lose health over time, also after running away.

| Tier | Damage per s | Duration | Total |
|---|---:|---:|---:|
| Weak | 1 | 20 s | 20 |
| Medium | 3 | 25 s | 75 |
| Strong | 5 | 30 s | 150 |

* For scale: a level 8 player has 134 HP, a level 21 player 290 HP. Strong poison takes half of the latter.
* Poison is on top of the hit's normal damage (blunt / slash / pierce,
  [solved/damage-types-and-resistances.md](solved/damage-types-and-resistances.md)).

Decided 2026-10-06:

* **Who poisons:** scorpion weak, cobra medium, scorpion queen strong. Apep himself does not poison; his cobras do.
* **The tiers stack, each tier once.** Each tier has its own timer. A hit of a tier already running restarts that
  tier's timer, at the same strength (weak + weak = weak, timer reset). A hit of another tier runs beside it (medium,
  then weak: both, each on its own timer). All three at once: 9 HP/s.
* **Poison can kill.** No stop at 1 HP.
* **Armour does not help.** Poison deals its full damage whatever the armour (`ignoreArmor`).
* **HUD:** the health bar turns green while poisoned. A poison icon in three looks, one per tier; each running tier
  shows its icon with its time left.
* **Save / load:** each tier's time left goes into the save.
* The journal notes which creatures poison.

* The tier numbers above are approved (2026-10-06).

## Antidote

* New potion (`ItemKind::Antidote`): removes all poison, every tier. Does not heal, gives no immunity afterwards.
* Model and texture: a potion bottle in a new colour (green).
* A quick-potion hotkey, like the others ([solved/quick-potions.md](solved/quick-potions.md)), and a key in the
  remapping ([solved/settings-ini.md](solved/settings-ini.md)): `=` (the keys `5`-`0` and `-` are taken).
* Loot: in the chests of the levels that have poisoners (scorpions, cobras, the queen); none before the first
  poisoner. Count it in the loot / balance checks.
* Campaign levels with poisoners get antidote chests. Where a level has no chest to spare, add a few cells (a short
  side passage) with antidote chests. The levels must still pass `./levelcheck` with no warnings. No level has a
  poisoner yet: this lands with the scorpion / cobra placement.

Not in scope: a poison resistance potion and amulets,
[resistance-potion-and-amulets.draft.md](resistance-potion-and-amulets.draft.md).
