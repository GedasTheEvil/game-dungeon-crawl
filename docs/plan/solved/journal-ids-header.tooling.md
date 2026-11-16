# The journal ids apart from the Journal class

Status: solved 2026-11-16. Was: draft 2026-11-16, size S, needs nothing first.

## Problem

The architecture review (2026-11-16, smell 8): `WorldEvent` carries a `FieldNote`, so `world_events.h` included the
whole `journal.h` (the `Journal` class, riddles, inscriptions, the level header, streams). Every change to the journal
recompiled what reports a world event.

## Plan

* `level/journal_ids.h`: `CreatureMove`, `FieldNote` and their helpers (`hourNote`, `skillNote`, `damageNote`,
  `potionNote`, `amuletNote`, `ringNote`, `shrineNote`, `curseNote`). `journal.h` includes it.
* `world_events.h` includes `journal_ids.h` only.
* Not done: the sim still calls `Journal` directly (`SeeMove` 33 times, some reads like `HasNote`). Routing the writes
  through `WorldEvents` changes when they apply; a separate plan if wanted.
