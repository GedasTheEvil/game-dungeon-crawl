# Denser levels: use the whole grid, more inscriptions, the route fork

Status: draft 2026-10-06, refined 2026-10-09 (decided, not implemented), more-inscriptions and route-fork merged in
2026-10-21 (user: they go hand in hand). From the user: every level is 40 x 47 cells, but most of it is solid wall.

Needs first: the new monster plans, so the denser levels place them too:
[camel-spider](solved/camel-spider.md) (solved), [hippo](solved/hippo.md) (solved),
[living-ushabti](solved/living-ushabti.md) (solved), [cursed-mimic](solved/cursed-mimic.md) (solved).
It also needs the mirrors and the new traps first (user, 2026-10-20), so the denser levels place them too:
[sunlight-mirrors](solved/sunlight-mirrors.md), [fire-jet-trap](fire-jet-trap.draft.opus.md),
[poison-drip-trap](poison-drip-trap.draft.opus.md), [trapped-chests](solved/trapped-chests.md).
The merged parts build on [skill-hieroglyphs](solved/skill-hieroglyphs.md) (the copy without the skill) and
[expedition-notes](solved/expedition-notes.md) (the page that warns before the fork), both solved.

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
* **Batches by boss block:** 1-5, 6-10, 11-14, one task each, play-tested in between. Each batch also places its
  levels' inscriptions ([More inscriptions](#more-inscriptions-from-level-1-on)). Last batch: the inscriptions of
  15-35 and the [route fork](#route-fork-left-or-right) on lvl34.
* Each level passes `bin/levelcheck` with no warnings.
* **Keep what other plans placed:** a redesign carries over the content that is not layout: inscriptions
  ([below](#more-inscriptions-from-level-1-on)), level titles ([duat-hours](duat-hours.draft.sonet.md)),
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

## More inscriptions, from level 1 on

(Merged from the more-inscriptions draft, 2026-10-12.)

Once an inscription the player cannot read is copied into the journal ("No idea what it says"), nothing stops
inscriptions in the early levels: they are a promise of the hieroglyph skill. So place them from level 1 on, and
more than one per level, also in the lower levels. Reading them later (learning the skill fills the copies in) pays
off the early ones. The denser levels have the room for them.

* Campaign rule: drop `INSCRIPTIONS_FROM_LEVEL` (`src/world/inscriptions.h`), its `levelcheck` warning
  (`checkInscriptions`, `tools/level/levelcheck.cpp`), the unit test's "before level 6" check
  (`tests/unit/inscriptions_test.cpp`) and the sentence in `docs/inscriptions.md` and `docs/levels.md`.
* Placement: levels 1-5 get inscriptions (suggested 1-2 each), the rest one or two more (suggested 2-3 per level).
  Same rules as in [hieroglyph-cartouches](solved/hieroglyph-cartouches.md): a reachable floor cell, mostly on or
  near the path; hints only, never needed. Edit the level files in `content/levels/` (an object `15 ID 0` on the cell, by
  hand or in the editor), never the ASCII drawings: the level files are the source of truth.
* Texts: new entries in `content/inscriptions/*.txt` with `G:` glyphs, then rebuild the atlas
  (`tools/textures/inscriptions.py`). Real texts (Pyramid Texts, Coffin Texts, the Shipwrecked Sailor, Sinuhe,
  Wenamun, workmen's ostraca from Deir el-Medina, the Amarna hymn to the Aten) and hints. Early-level hints suit
  what the player meets there (the first key and gate, the spike pit, the first mimic, the scarab king of level 5).
  Keep the boon share of [inscription-boons](solved/inscription-boons.md).
* Atlas capacity: ids stop at 64 (`INSCRIPTION_ATLAS_GRID` 8). Around 70-90 inscriptions need more: a larger grid
  (12 x 12 at 256 px: 3072 px square) or 128 px cells in the same 2048 px; implementer's choice, keep the id = cell
  rule.

## Route fork: left or right

(Merged from the route-fork draft, 2026-10-19, from the user's review.)

One level splits into two one-way routes that both reach the exit. The player picks one, warned before the choice.
It pays off the expedition's last page: "If you read this, take the left gate. I took the right."

* **Never a trap:** both routes reach the exit hall. The choice is cost and reward, not survival. The map stays as
  it is.
* **One-way:** the fork is two drop shafts (or two one-way gates) side by side in one hall. Once down, no way back
  up to the other route; the routes join again in the exit hall.
* **Left (the safe one):** longer, a crocodile wade or a monster hall of the level's set, little loot.
* **Right (the one that killed them):** shorter and harder: a trap gauntlet (rock falls, dart plates, rhythm spikes,
  the new fire jets and poison drips), then a treasure tomb and the earlier archaeologist's remains: a skeleton with
  their gear (a good weapon or amulet of the depth) and their journal's end. A brave player takes it for the loot.

Known before the choice, both hints on the way to the fork:

* **The expedition page** (always): the last page lies on the path to the fork, on a skeleton or a floor spot, never
  in a chest (no mimic, no trapped chest, no chance to miss it). Its text names the two gates and what the writer
  found behind the right one. This page is the exception to expedition-notes' "pages are optional, not on the exit
  path".
* **An inscription at the fork** (for readers): on the back wall between the shafts, a warning in the priests' words
  ("To the left, the long road of the living. To the right, the short road of the dead."). Without the
  [Words of Thoth](solved/skill-hieroglyphs.md) skill it is copied as unread, as today.

Placement: one fork in the campaign, on the level of the expedition's last page: lvl34 today (`docs/levels.md`).
Implementer's choice: a second, smaller fork earlier (e.g. in the denser 6-14), with an earlier page.

levelcheck must prove the fork is fair: from the fork, each route alone reaches the exit (keys, levers and gates on
each route, not one key on the other side). A new warning: a one-way drop whose landing cannot reach the exit. Check
whether today's checker already finds that softlock; if so, this needs only a test on the fork level.

## Tests

* Inscriptions, campaign unit test: every level 1-35 has at least one inscription, each id has a text and is used
  once.
* Fork, unit: the checker passes the fork level and warns about a copy where the right route's gate key lies on the
  left. Scenario with screenshots: the fork hall, the page, the inscription.
* `bin/levelcheck content/levels/lvl*` with no warnings.
