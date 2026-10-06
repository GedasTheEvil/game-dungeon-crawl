# Scorpion queen boss

Status: implemented 2026-10-06 (not play tested). Draft 2026-10-05. Picked from the candidates in [more-bosses.draft.md](more-bosses.draft.md).

A boss for the Egyptian setting (Serket, the scorpion goddess). She ranks below the Anubis boss: she comes before him
in the campaign, and he stays the last boss.

## The boss

* Big scorpion: segmented body, two claws, a raised tail with a sting. Built in Blender like the other monsters
  ([../remodeling.md](../remodeling.md)).
* Claws for melee; the sting gives **strong poison** ([poison-and-antidote.md](solved/poison-and-antidote.md)).
* Resistances: open. Idea: the club cracks her shell (unlike the scarabs, where the spear wins), so the club gets a boss
  of its own.
* A row in `MONSTER_DEFS` and `BOSS_DEFS`, like the other bosses.

## Minions: scorpions

* New regular monster, the **scorpion**: small, quick, its sting gives **weak poison**. Built 2026-10-06
  ([poison-and-antidote.md](solved/poison-and-antidote.md#next)); the queen can reuse its model with her own texture.
* Scorpions are no stranger to Egypt: the plain scorpion lives in the early levels 3-6 (2026-10-06), not only
  around the queen.
* Her minions are a stronger scorpion (decided 2026-10-06): a giant scorpion, like the giant rat / scarab / bat, on the
  same model with its own texture, medium poison.

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

* Attack damage mix ([monster-attack-damage-types.md](solved/monster-attack-damage-types.md)): her own group,
  not the scorpions'; maybe all three types (claws slash, sting pierce, a blow of the tail blunt).
* Placement: the queen needs a boss level before the Anubis boss. Today the bosses sit at lvl5 / 10 / 15 and lvl15
  holds the ankh. See [longer-campaign.md](longer-campaign.md).
* Numbers (HP, damage, summon counts), after the poison numbers are settled.
* `levelcheck` rules for egg clusters (inside the boss room, reachable).

## Decided by the agent (2026-10-06, the user away; easy to change)

* Model: the scorpion's, scale ~48, her own texture `scorpion_queen` (Serket: pale gold carapace, lapis-blue
  joints, gold sting). Like the boss scarab on the scarab model.
* Strong poison on her sting. Attack mix: her own group row is not possible (keyed by model): she deals the
  scorpions' mix (claws, sting).
* Egg clusters: a new rooted monster, the **egg cluster** (`Locomotion::Stationary`, never bites, some HP, killable
  with any weapon). Placed in the boss room by the level. `Summon::Hatch`: her minions (giant scorpions) come out of a
  living egg cluster (a dig-out effect on its cell); with no cluster left she summons no more.
* Starting numbers: speed 5, 700 HP, 45 damage every 1100 ms, strong poison, 15000 XP; minions: 2 on arrival, at most
  4, one every 3 s, 10 per fight. Egg cluster: 60 HP, 0 damage, 300 XP.

## Done (2026-10-06)

* `MonsterScorpionQueen` (21, glyph `U`), `MonsterEggCluster` (20, `e`, model `tools/blender/models/egg_cluster.py`),
  `MonsterGiantScorpion` (19, `J`, the minion), `Summon::Hatch` (`Dungeon::summonMinion`; the first minions wait for a
  cluster in play). Numbers as decided above. In lvl15's boss room with two egg clusters.
* Check: `tests/scenarios/scorpion_queen.txt`. Not play tested.
