# Mummy monster: minion of an Anubis boss

Status: idea, not started.

## What

* A mummy monster that serves as a minion to an Anubis guard acting as a "boss".
* Separate from the decorative mummy by the sarcophagus: [statue-and-mummy-decorations.md](solved/statue-and-mummy-decorations.md).

## Where it fits

* New monster type: `MonsterTypeId` in `src/world/level.h`, a row in `MONSTER_DEFS` (`src/state/assets.cpp`), editor text
  in `tools/editor/tile_info.cpp`, threat in `monsterThreat` (`src/world/level_check.cpp`), generator unlock in
  `src/world/level_gen.cpp`.
* Model: a new `tools/blender/models/mummy.py` with move, `_att`, `_die` clips (optional `_idle`). See [../remodeling.md](../remodeling.md).

## Open questions

* How is the boss/minion link made: minions spawn near the Anubis, get woken by it, or die with it?
* Does the Anubis boss need its own changes (more HP, a boss health bar, an arena room)?
* Should the mummy ever rise from the decorative sarcophagus?
