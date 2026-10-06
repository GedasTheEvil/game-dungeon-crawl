# Sobek boss

Status: implemented 2026-10-06 (not play tested). Draft 2026-10-06. The lvl25 boss of the [longer campaign](longer-campaign.md), picked by the agent
from [more-bosses.draft.md](more-bosses.draft.md) while the user was away (easy to swap for another).

Sobek, the crocodile god of the Nile: a giant crocodile in a flooded boss room.

* Model: the crocodile's (all clips), scale ~1.7x, his own texture `crocodile_sobek` (dark green-black, gold scute
  tips, a gold-and-lapis collar, red eyes).
* **Charge:** when the player is on his row 2.5 to 6 tiles away, he lowers his head and charges in a straight line at 4x
  his speed. A charge that hits the player knocks them for double damage; a charge that hits a wall stuns him for 2.5 s
  (he takes double damage while stunned). The player dodges it by jumping over him (he is low) or by stepping off the
  row (a ladder).
* Swimmer (as the crocodile), so the half-water lake is his ground.
* Minions: crocodiles that surface from the water (`Summon::DigOut` look in water: a splash).
* Starting numbers: speed 6, 1600 HP, 70 damage every 1300 ms, 25000 XP; minions 1 / 3 / every 5 s / 6 per fight.

## Done (2026-10-06)

* `MonsterSobek` (23, glyph `W`), a lurking boss wakes from `BOSS_WAKE_RANGE` (4 tiles). The charge:
  `Monster::StartCharge` / `UpdateCharge`, `CHARGE_*` in `gameplay_config.h`; stunned he lies flat (the idle clip)
  and takes double weapon damage. Crocodiles dig out beside him. In lvl25's flooded boss room.
* Check: `tests/scenarios/sobek.txt`. Not play tested.
