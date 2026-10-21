# Denser levels: use the whole grid

Status: draft 2026-10-06, refined 2026-10-09 (decided, not implemented). From the user: every level is 40 x 47
cells, but most of it is solid wall.

Needs first: the new monster plans, so the denser levels place them too:
[camel-spider](solved/camel-spider.md) (solved), [hippo](solved/hippo.md) (solved),
[living-ushabti](solved/living-ushabti.md) (solved), [cursed-mimic](solved/cursed-mimic.md) (solved).
It also needs the mirrors and the new traps first (user, 2026-10-20), so the denser levels place them too:
[sunlight-mirrors](solved/sunlight-mirrors.md), [fire-jet-trap](fire-jet-trap.draft.opus.md),
[poison-drip-trap](poison-drip-trap.draft.opus.md), [trapped-chests](trapped-chests.md).

## Today (2026-10-09, `bin/levelcheck`)

* Levels 15-30 already fill the grid: 516-736 open cells, 43-45 rows tall.
* Levels 1-14 do not: 84-194 open cells, 5-16 rows tall. From lvl14 (194) to lvl15 (663) the size jumps more than 3x.

## Decided (2026-10-09)

* No format change: the grid stays 40 x 47 (`LEVEL_WIDTH`, `LEVEL_HEIGHT`). More halls, floors, side branches, loops
  (two ways to the same place), vertical shafts.
* **A smooth ramp** of open cells (about, not exact):

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Open cells | ~100 | ~110 | 150 | 190 | 220 | 260 | 300 | 340 | 370 | 410 | 450 | 490 | 520 | 560 |

* Levels 1 and 2 stay about as they are (the way in).
* **Redesign freely:** only the level's place in the difficulty curve is fixed. Monsters, keys, riddles, chests and
  layout may all change; the boss levels (5, 10) keep their boss. Monster count and treasures grow with the area, the
  monster mix follows what is unlocked by then (`docs/levels.md`). Update the level's line in `docs/levels.md`.
* **Batches by boss block:** 1-5, 6-10, 11-14, one task each, play-tested in between.
* Each level passes `bin/levelcheck` with no warnings.
* **Keep what other plans placed:** a redesign carries over the content that is not layout: inscriptions
  ([more-inscriptions](more-inscriptions.draft.opus.md)), level titles ([duat-hours](duat-hours.draft.sonet.md)),
  Hapi potions ([water-blessing-potion](solved/water-blessing-potion.md)), expedition pages, mirror puzzles,
  ring transporters, the Stargate chamber, trapped chests. Same count or more; the spots may move.
* **Place the timed traps** that landed before this plan: [rhythm-traps](solved/rhythm-traps.md),
  [fire-jet-trap](fire-jet-trap.draft.opus.md), [poison-drip-trap](poison-drip-trap.draft.opus.md). Not blocking: a
  trap plan still in draft is skipped; it then places its own runs in the finished levels.

## Watch

* The spawn window (monsters spawn when they come into view), `MAX_MONSTERS` (32 live slots), the draft map's size on
  screen, the load time.
* The XP curve: more monsters on 1-14 means a higher player level at lvl15 (today about 35,
  [monster-balance](monster-balance.draft.opus.md)). Recheck the sum per level after each batch.
