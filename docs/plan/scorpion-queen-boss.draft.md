# Scorpion queen boss

Status: draft 2026-10-05. Picked from the candidates in [more-bosses.draft.md](more-bosses.draft.md).

A boss for the Egyptian setting (Serket, the scorpion goddess). She ranks below the Anubis boss: she comes before him
in the campaign, and he stays the last boss.

## The boss

* Big scorpion: segmented body, two claws, a raised tail with a sting. Built in Blender like the other monsters
  ([../remodeling.md](../remodeling.md)).
* Claws for melee; the sting gives **strong poison** ([poison-and-antidote.md](poison-and-antidote.md)).
* Resistances: open. Idea: the club cracks her shell (unlike the scarabs, where the spear wins), so the club gets a boss
  of its own.
* A row in `MONSTER_DEFS` and `BOSS_DEFS`, like the other bosses.

## Minions: scorpions

* New regular monster, the **scorpion**: small, quick, its sting gives **weak poison**. Built 2026-10-06
  ([poison-and-antidote.md](poison-and-antidote.md#next)); the queen can reuse its model with her own texture.
* Also used outside the boss room, in the levels around the queen.
* Maybe a giant scorpion later (medium poison), like the giant rat / scarab / bat.

## Summon: egg clusters

* New `Summon` kind (for example `Summon::Hatch`), beside `DigOut`, `Drop` and `Coffin`.
* Egg clusters stand on the floor or the walls of the boss room. A cluster hatches scorpions every N s.
* The player can smash a cluster to stop it: a choice between the boss and the eggs.
* Needs a model for the egg cluster, a hatch effect and a sound.

## Anubis boss stays on top

The queen comes before the Anubis boss, so he must be the stronger fight. Today: speed 4.5, 1500 HP, 110 damage
every 1400 ms (`MonsterAnubisBoss`). Retune him after the queen's numbers are set, in
[monster-balance.draft.md](monster-balance.draft.md).

## Open

* Placement: the queen needs a boss level before the Anubis boss. Today the bosses sit at lvl5 / 10 / 15 and lvl15
  holds the ankh. See [longer-campaign.draft.md](longer-campaign.draft.md).
* Numbers (HP, damage, summon counts), after the poison numbers are settled.
* `levelcheck` rules for egg clusters (inside the boss room, reachable).
