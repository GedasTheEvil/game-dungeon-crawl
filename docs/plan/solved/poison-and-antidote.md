# Poison damage and the antidote

Status: solved 2026-10-06, confirmed in play (the level-up cure added after). Draft
2026-10-05. Needed by the
[scorpion-queen-boss.md](../scorpion-queen-boss.md) and
[apep-serpent-boss.md](../apep-serpent-boss.md).

## Poison

A poisoned hit makes the player lose health over time, also after running away.

| Tier | Damage per s | Duration | Total |
|---|---:|---:|---:|
| Weak | 1 | 20 s | 20 |
| Medium | 3 | 25 s | 75 |
| Strong | 5 | 30 s | 150 |

* For scale: a level 8 player has 134 HP, a level 21 player 290 HP. Strong poison takes half of the latter.
* Poison is on top of the hit's normal damage (blunt / slash / pierce,
  [solved/damage-types-and-resistances.md](damage-types-and-resistances.md)).

Decided 2026-10-06:

* **Who poisons:** scorpion weak, cobra medium, scorpion queen strong. Apep himself does not poison; his cobras do.
* **The tiers stack, each tier once.** Each tier has its own timer. A hit of a tier already running restarts that
  tier's timer, at the same strength (weak + weak = weak, timer reset). A hit of another tier runs beside it (medium,
  then weak: both, each on its own timer). All three at once: 9 HP/s.
* **Poison can kill.** No stop at 1 HP.
* **A level up cures it** (2026-10-06, after the play test), as it heals fully.
* **Armour does not help.** Poison deals its full damage whatever the armour (`ignoreArmor`).
* **HUD:** the health bar turns green while poisoned. A poison icon in three looks, one per tier; each running tier
  shows its icon with its time left.
* **Save / load:** each tier's time left goes into the save.
* The journal notes which creatures poison.

* The tier numbers above are approved (2026-10-06).

## Antidote

* New potion (`ItemKind::Antidote`): removes all poison, every tier. Does not heal, gives no immunity afterwards.
* Model and texture: a potion bottle in a new colour (green).
* A quick-potion hotkey, like the others ([solved/quick-potions.md](quick-potions.md)), and a key in the
  remapping ([solved/settings-ini.md](settings-ini.md)): `=` (the keys `5`-`0` and `-` are taken).
* Loot: in the chests of the levels that have poisoners (scorpions, cobras, the queen); none before the first
  poisoner. Count it in the loot / balance checks.
* Campaign levels with poisoners get antidote chests. Where a level has no chest to spare, add a few cells (a short
  side passage) with antidote chests. The levels must still pass `./levelcheck` with no warnings. No level has a
  poisoner yet: this lands with the scorpion / cobra placement.

Not in scope: a poison resistance potion and amulets,
[resistance-potion.draft.md](../resistance-potion.draft.md), [amulets.md](amulets.md).

## Done

* `Poison` (`src/world/poison.h`, unit tests in `tests/unit/poison_test.cpp`): a timer per tier, a tick of damage per
  full second. Owned by `PlayerStats`, saved after its line (`POISON`; older saves load with none).
* `Player::Poison` (status line "You are poisoned!", the field note), `Player::UpdatePoison` once a tick from the game
  loop (stands still behind the screens, like everything else), no damage in `god` mode. Death ends the poison;
  New Game and Load too.
* Monsters: `MonsterType::poison` (an optional tier); a bite applies it and writes the journal's "poison" move. No
  monster has it yet.
* `ItemKind::Antidote` (potion id 7, "Cure", dark malachite), blocked while not poisoned. Not in the mimic's loot
  nor in generated chests (weight 0). Inventory slot key and in-game key `=` (`quick_antidote`).
* HUD: green health bar and heart; above the panel one stone tile per running tier, a drop in the tier's green with
  one to three pips, and its seconds left.
* Journal: field note "Poison" (the first time poisoned).
* Scenario: `poison weak|medium|strong`, `expect poison` (bit mask); `tests/scenarios/poison.txt`.

## Next

In this order (2026-10-06):

1. The scorpion: a regular monster with weak poison, from [scorpion-queen-boss.md](../scorpion-queen-boss.md).
   Done 2026-10-06: `MonsterScorpion` (16, glyph `j`), speed 10, 14 HP, 3 damage every 900 ms, 450 XP, scale 15;
   weak to blows, resists points (the bow does little: it has to be fought up close); weak poison (`POISON_DEFS` in
   `src/state/assets.cpp`). Model `tools/blender/models/scorpion.py` (deathstalker), sounds
   `tools/audio/scorpion_sounds.py`. Check: `tests/scenarios/scorpions.txt`. Not placed in any level yet.
2. Then the levels: the scorpions' places in the campaign and antidote chests in those levels (a short side passage
   where a level has no chest to spare), each level still without `levelcheck` warnings.
   Done 2026-10-06: scorpions in levels 3-6 (one each in 3-5, two in 6), each level with an antidote chest in a new
   one-cell niche ([../levels.md](../../levels.md#campaign-order)). `levelcheck` warns about a level with a poisoner
   (`isPoisoner`, `src/world/monster_kinds.h`) and no antidote in reach.
