# Tomb-curse containers: mimic jar, coffins

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: new container kinds and maybe a model. Left from [treasure-containers](solved/treasure-containers.md) and
[trapped-chests](solved/trapped-chests.md).

* A mimic jar: a canopic jar or sealed pot that is a mimic, smaller and weaker than the chest mimic, in tomb levels.
  Its own bite and wake sounds.
* Sarcophagi / coffins as tomb-curse objects: opening a coffin (not a mummy coffin that wakes) gives loot with a
  chance of a curse (`src/world/curses.cpp`), with a tell for the attentive (an inscription on the lid).
* Add to `src/world/containers.h`, loot rolls, the journal (field note), levelgen depth rules; place a few by hand.
