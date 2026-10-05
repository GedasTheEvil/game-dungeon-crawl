# Crocodile bite with a hold

Status: draft 2026-10-05. Split off [crocodiles-and-flooded-cells.md](solved/crocodiles-and-flooded-cells.md);
the crocodile ships with a plain bite first.

## Idea

The crocodile's bite grabs the player: a hold, like a death roll.

* While held, the player cannot walk or jump. The crocodile keeps biting (or deals damage over time).
* The player breaks free by hitting it (a number of hits or an amount of damage), or by mashing a key.
* Telegraphed, so the player can back off before it lands.

## Open

* How the player breaks free: hits, damage or a key; how long a hold lasts at most.
* Damage while held, and whether armour helps.
* Only in water, or on land too?
* Look: the crocodile's grab and roll animation (Blender, [../remodeling.md](../remodeling.md)), the player's held
  pose.
* Does the checker or the balance need to know (a held player near a trap)?
