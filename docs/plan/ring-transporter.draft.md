# Ring transporter

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user (a Stargate
reference).

## Idea

The Goa'uld ring transporter: a ring platform in the floor; interact and five rings drop from the ceiling around the
player, a flash, and the player stands on the partner platform on another floor. It is a teleporter
(`GateTeleport`, `src/world/level.h`) with its own look and sound.

* A new gate type or a flag on the teleporter pair (implementer's choice); pairs as today (`teleportPair`).
* Visual: rings drop (staggered, ~0.6 s), a white flash, rings rise at the partner. The player cannot move during it.
  Monsters standing on the platform come along? Default: no.
* Sound: the ring hum and whoosh, made (not ripped) to evoke it.
* Platform: a round metal plate with a ring groove; ceiling above shows a ring housing. Needs a ceiling cell above.
* `levelcheck` treats it as a teleporter.
* The empty "Rings" inventory tab: a journal joke at the first use ("At last, a ring for my Rings pocket. It does not
  fit on a finger.").
* Use from mid campaign, a few levels; not every teleporter changes.

## Tests

* Unit / sim: interact moves the player to the partner, like a teleporter.
* Scenario with screenshots: rings down, flash, arrival.
