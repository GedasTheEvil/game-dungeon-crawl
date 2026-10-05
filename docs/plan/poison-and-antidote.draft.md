# Poison damage and the antidote

Status: draft 2026-10-05. Needed by [scorpion-queen-boss.draft.md](scorpion-queen-boss.draft.md) and
[apep-serpent-boss.draft.md](apep-serpent-boss.draft.md).

## Poison

A poisoned hit makes the player lose health over time, also after running away.

| Tier | Damage per s | Duration | Total |
|---|---:|---:|---:|
| Weak | 1 | 20 s | 20 |
| Medium | 3 | 25 s | 75 |
| Strong | 5 | 30 s | 150 |

* For scale: a level 8 player has 134 HP, a level 21 player 290 HP. Strong poison takes half of the latter.
* Which monsters poison, and which tier: scorpion weak, scorpion queen strong, cobra medium (proposal).
* Poison is on top of the hit's normal damage (blunt / slash / pierce,
  [solved/damage-types-and-resistances.md](solved/damage-types-and-resistances.md)).

Open:

* A second poisoned hit: restart the timer, keep the stronger tier, or add up?
* Can poison kill, or does it stop at 1 HP?
* Does armour reduce poison?
* HUD: a poison icon with the tier and the time left; green tint on the health bar.
* Save / load: the poison state goes into the save.

## Antidote

* New potion (`ItemKind::Antidote`): removes any poison, whatever the tier. Does not heal.
* Model and texture: a potion bottle in a new colour (green).
* Found in chests and in the levels near poisoners. Count it in the loot / balance checks.
* A hotkey, like the quick potions ([solved/quick-potions.md](solved/quick-potions.md)), and a key in the
  remapping ([solved/settings-ini.md](solved/settings-ini.md)).
* The journal notes which monsters poison.
