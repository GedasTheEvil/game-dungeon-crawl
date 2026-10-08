# Apep serpent boss

Status: implemented 2026-10-06 (not play tested). Draft 2026-10-05. Picked from the candidates in [more-bosses.draft.md](more-bosses.draft.md).

Apep (Apophis), the serpent of chaos, a giant snake boss.

## The boss

* Dives into holes in the floor and comes up out of another hole. The player waits at the right hole or gets bitten
  from behind.
* Hits only while it is out of a hole.
* Model: a long segmented body. The hardest model and animation so far: a spline body in Blender, or segments that
  follow the head.

## Minions: cobras

* New regular monster, the **cobra**: [cobra.md](solved/cobra.md) (implemented, not placed yet).
* Bite or venom gives **medium poison** ([poison-and-antidote.md](solved/poison-and-antidote.md)).
* Also used outside the boss room.
* Cobras and Apep are swimmers: 125% speed in half water
  ([crocodiles-and-flooded-cells.md](solved/crocodiles-and-flooded-cells.md)).

## Summon: baskets / urns

* New `Summon` kind (for example `Summon::Basket`): cobras come out of baskets or urns in the boss room.
* Maybe the urns can be broken, like the scorpion queen's egg clusters.

## Open

* Attack damage mix ([monster-attack-damage-types.md](solved/monster-attack-damage-types.md)): the bite
  pierce, Apep and the cobra alike? Apep's coils crushing (blunt)?
* Placement and its rank against the scorpion queen and the Anubis boss:
  [longer-campaign.md](longer-campaign.md).
* New tile or prop for the holes, and how `levelcheck` checks them.
* Numbers.

## Decided by the agent (2026-10-06, the user away; easy to change)

* Model: the cobra's (all clips), scale ~70, his own texture `cobra_apep` (deep red-black with gold-green scale
  edges, glowing yellow eyes). A segmented body is a later upgrade.
* Holes without a new tile: Apep dives (sinks into the floor like a dig-out minion, reversed) every few seconds,
  then comes up 2 cells behind the player on his row (or in front if there is no floor behind), with the sand / dust
  effect and a dark hole drawn where he dived and where he comes up. He bites only while up.
* Minions: cobras that dig out of the sand (`Summon::DigOut`); the baskets are a later upgrade.
* No poison himself. Bite: pierce (the cobras' mix). Starting numbers: speed 6, 1100 HP, 60 damage every 1200 ms,
  20000 XP; minions 2 / 4 / every 3 s / 10 per fight.

## Done (2026-10-06)

* `MonsterApep` (22, glyph `P`), `Locomotion::Burrow` (`Monster::Dive`, `UpdateBurrow`, `Dungeon::burrowTarget`,
  `BURROW_*` in `gameplay_config.h`), holes drawn with the dig-out grit (`Dungeon::drawHole`), hidden from weapons
  and arrows while down. Cobras dig out. In lvl20's boss room.
* Not done: the baskets, a segmented body, a hole tile. Check: `tests/scenarios/apep.txt`. Not play tested.
