# The Stargate chamber

Status: solved 2026-10-27 (play-tested by the user). Draft 2026-10-10, from the lore review, picked by the user (the user loves
Stargate). Works better with [hieroglyph-cartouches](hieroglyph-cartouches.md), does not need it.

## Idea

One secret chamber in the campaign: a great stone ring with chevrons, a dead dial device (DHD) beside it. It never
opens; it is an easter egg with a reward.

* Hidden behind a riddle gate with a Stargate riddle (e.g. "How many chevrons lock to dial another world?" A: seven),
  or a cartouche hint on a wall nearby. Level: mid campaign, off the main path, in one level only.
* Inside: the ring (static model, chevrons glow one by one when the player interacts, the seventh fails to lock:
  "Chevron seven... will not lock."), the DHD (interact: a journal note).
* Reward: an XP bonus and an amulet or a rare potion in a chest.
* Journal notes, humour and references: "Seven chevrons. Only six symbols on the wall. Daniel would know.",
  "Indeed.", "Abydos is not on this map."
* No trademarked logos or ripped assets: a ring of our own design that evokes it.

## Tests

* The chosen level passes `levelcheck` with no warnings (the chamber is optional, not on the exit path).
* Scenario with a screenshot of the chamber and the chevrons lit.

## Implemented (2026-10-27)

* Where: lvl48 (The Ring of Seven Marks), the room its levelgen stand-in (`--stargate`) put behind the level's name
  gate: hidden by the king's name, its cartouche elsewhere on the level (the draft's cartouche hint; no riddle gate,
  those hide nothing). The zone table (2026-10-21) put the chamber in the necropolis, later than "mid campaign".
  The room a cell longer: the ring's text (127) over the rare chest (an amulet) at the far end, the dial device, an
  empty cell, the ring, a torch beside it, the teleporter.
* Two placed props (`decor.h`): `DECOR_STARGATE` (the ring, its inner track turned by the engine, seven chevrons, a
  lit chevron drawn over each that glows) and `DECOR_DIAL` (a tilted top of 38 sign keys round a red dome). Models in
  `decor.py` (`stargate`, `stargate_track`, `stargate_chevron` baked unlit, `dial_device`), editor icons.
* Interact on the ring (Dial): the track turns back and forth and a chevron locks every 0.7 s, six of them; the
  seventh flickers out, "Chevron seven... will not lock.", the six fade. The first dial: a riddle's XP and the field
  note `FieldNote::Stargate` ("Seven chevrons. Only six symbols on the wall. Daniel would know."). The dial device
  (Press): a clunk, nothing, the note `FieldNote::DialDevice` ("Indeed.", "Abydos is not on this map."). An orange
  light on the walls while chevrons glow. Sounds `chevron_lock`, `chevron_fail`, `dial_press`
  (`tools/audio/ring_sounds.py`). No torch over the ring (scatter, checker).
* levelgen `--stargate` places the two props too, so a rebuild keeps the chamber.
* The way in (user, 2026-10-27): the name gate is a ring platform sealed by the name (`NAME_SEAL | RING_PLATFORM`),
  its partner in the chamber a ring platform; sealed, the plate glows and dims with the name gate's breath. Spoken,
  the rings carry the player. lvl48: rock over the gate's cell (col 38, row 23) for the rings.
* Tests: `tests/unit/stargate_test.cpp`, the levelgen test, scenario `tests/scenarios/solved/stargate_chamber.txt`.
  Play-test: `bin/game --play tests/play/stargate-chamber.txt` (lvl48, in the chamber).
