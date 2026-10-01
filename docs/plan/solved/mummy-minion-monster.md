# Mummy monster: minion of an Anubis boss

Status: solved (2026-10-01), lvl11 played: looks right, XP lowered. The Anubis boss is in too (step 4 of
[boss-rooms.md](boss-rooms.md)).

## What

* A mummy monster, the minion of the Anubis boss to come, and a normal monster from level 11 on.
* It lies on its back in an open, empty coffin (decor prop `coffin`, drawn on its spawn tile). When the player comes
  within 1.6 tiles on its row, or hits it, it wakes: sits up, swings its legs over the rim, climbs out towards the
  walk line, stands, then walks to the player and strikes. The empty coffin stays.
* Separate from the decorative mummy by the sarcophagus: [statue-and-mummy-decorations.md](statue-and-mummy-decorations.md).

## Decisions

* Stats: speed 2.5, 150 HP, 20 damage, a slow swing (1600 ms), 2500 XP (was 4500: too much for its strength in play), scale 18. Threat 5 in `monsterThreat`.
* Campaign: mummies in levels 11-15. The Anubis guard moved to level 13 on (lvl10's became a giant scarab, lvl11's
  and lvl12's mummies). `levelgen`: mummy from difficulty 7, Anubis from 9.
* Glyph `u`, monster type 13 (`MonsterMummy`). The Anubis boss becomes type 14.

## How it works

* `Locomotion::Entombed`: walks like `Walk`, lurks like the mimic (`Monster::Lurk`, `lurking`) until woken
  (`Monster::wake`), then plays the rise clip (`ModelState::Rise`, `mummy_rise.md3`, once) and does not act while
  `Rising`. Clips: `ENTOMBED_CLIPS` (`character_model.h`), the walk file is the reference.
* The body is drawn `MUMMY_COFFIN_DEPTH` (14.4) world units back towards the wall while it lies in the coffin; over
  the rise clip from `MUMMY_CLIMB_FROM` to `MUMMY_CLIMB_TO` it slides out to the walk line (`Monster::tomb`). In world
  units, so the toon figure scale does not move it out of the coffin.
* Coffin: `DECOR_COFFIN`, set on every mummy spawn tile by `Dungeon::scatterDecorations`, never scattered at random
  (`DECOR_SCATTERED`).
* Model `tools/blender/models/mummy.py`, sounds `tools/audio/mummy_sounds.py`. Test: `tests/scenarios/mummy.txt`.

## Open questions

* Done: the Anubis boss's mummies climb out of the coffins round him (`Summon::Coffin`,
  [boss-rooms.md](boss-rooms.md) step 4). A mummy summoned any other way still skips the coffin and walks
  at once (`Monster::MakeMinion`).
* Tune the stats in play.
