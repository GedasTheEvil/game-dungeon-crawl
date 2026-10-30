# Splash when a monster enters water

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

`WorldSound::Splash` plays only for the player's landing / wading and for dung balls (`src/world/dungeon_base.cpp`,
`dungeon_balls.cpp`). [crocodiles-and-flooded-cells](solved/crocodiles-and-flooded-cells.md) left monsters out.

* A monster crossing from air into a water cell (falling or walking in) plays the splash at its position, louder for
  large monsters, and spawns the existing splash particles if the player's landing has them.
* Not for swimmers that move inside water, nor for a submerged monster surfacing (or a softer sound for that).
* Unit test on the event, not the audio.
