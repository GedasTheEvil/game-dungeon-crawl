# Apep serpent boss

Status: draft 2026-10-05. Picked from the candidates in [more-bosses.draft.md](more-bosses.draft.md).

Apep (Apophis), the serpent of chaos, a giant snake boss.

## The boss

* Dives into holes in the floor and comes up out of another hole. The player waits at the right hole or gets bitten
  from behind.
* Hits only while it is out of a hole.
* Model: a long segmented body. The hardest model and animation so far: a spline body in Blender, or segments that
  follow the head.

## Minions: cobras

* New regular monster, the **cobra**: rises, then strikes. Maybe spits venom at range.
* Bite or venom gives **medium poison** ([poison-and-antidote.md](solved/poison-and-antidote.md)).
* Also used outside the boss room.
* Cobras and Apep are swimmers: 125% speed in half water
  ([crocodiles-and-flooded-cells.md](solved/crocodiles-and-flooded-cells.md)).

## Summon: baskets / urns

* New `Summon` kind (for example `Summon::Basket`): cobras come out of baskets or urns in the boss room.
* Maybe the urns can be broken, like the scorpion queen's egg clusters.

## Open

* Attack damage mix ([monster-attack-damage-types.md](monster-attack-damage-types.md)): the bite
  pierce, Apep and the cobra alike? Apep's coils crushing (blunt)?
* Placement and its rank against the scorpion queen and the Anubis boss:
  [longer-campaign.draft.md](longer-campaign.draft.md).
* New tile or prop for the holes, and how `levelcheck` checks them.
* Numbers.
