# Ring transporter

Status: solved 2026-10-27 (play-tested by the user). Draft 2026-10-10, from the lore review, picked by the user (a Stargate
reference).

## Idea

The Goa'uld ring transporter: a ring platform in the floor; interact and five rings drop from the ceiling around the
player, a flash, and the player stands on the partner platform on another floor. It is a teleporter
(`GateTeleport`, `src/world/level.h`) with its own look and sound.

* A new gate type or a flag on the teleporter pair (implementer's choice); pairs as today (`teleportPair`).
* Visual: rings drop (staggered, ~0.6 s), a white flash, rings rise at the partner. The player cannot move during it.
  Monsters standing on the platform stay behind: only the player travels (user, 2026-10-22).
* Sound: the ring hum and whoosh, made (not ripped) to evoke it.
* Platform: a round metal plate with a ring groove; ceiling above shows a ring housing. Needs a ceiling cell above.
* `levelcheck` treats it as a teleporter.
* The empty "Rings" inventory tab: a journal joke at the first use ("At last, a ring for my Rings pocket. It does not
  fit on a finger.").
* Use from mid campaign, a few levels; not every teleporter changes.

## Tests

* Unit / sim: interact moves the player to the partner, like a teleporter.
* Scenario with screenshots: rings down, flash, arrival.

## Implemented (2026-10-27)

* `RING_PLATFORM` (512) on a teleporter's value (`level.h`, glyph `0`); the pair id as before. Interact (Enter):
  the player steps to the middle, the five rings drop (150 ms apart, 400 ms each, a 250 ms hum), a white flash
  (180 ms out and in), the player is moved, the rings rise at the partner. The world stands still meanwhile, as
  for any gate travel: only the player travels. Timings `RING_*` in `gameplay_config.h`; phases
  `GateTravel::Phase::RingsDown` / `RingsUp`, `Dungeon::RingDrop`.
* Models in `mechanism.py`: `ring_platform` (gold-bronze plate, the groove, seven marks round a gold centre),
  `ring_housing` (a dark shaft in the ceiling, a bolted bronze collar), `ring` (drawn five times). Sounds
  `tools/audio/ring_sounds.py`: `rings_down.wav`, `rings_up.wav`.
* `levelcheck`: a ring platform's partner must be one too, rock above each.
* Field note `FieldNote::Rings` at the first ride, with the Rings-pocket joke.
* Levels: lvl17 (priests' quarter) a new ring pair from a wing's end to a sealed room with a might potion, the
  ring's text (`RING_TEXT`) beside it; plain pairs turned to rings on lvl31, lvl39, lvl48 and lvl50.
* Tests: `tests/unit/rings_test.cpp`, scenario `tests/scenarios/solved/ring_transporter.txt` (`tests/levels/rings`).
  Play-test: `bin/game --play tests/play/ring-transporter.txt` (lvl17 on the platform).
