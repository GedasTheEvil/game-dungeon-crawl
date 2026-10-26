# Rebuild the campaign: zone by zone, the whole grid, forks

Status: draft 2026-10-06 (denser levels), refined 2026-10-09; more-inscriptions and route-fork merged in 2026-10-21;
turned into the full rebuild 2026-10-21 (user: "levels re-generated completely, adjusting to the new bosses", 40-60
levels in zones). Slug kept from the first version.

Needs first: [campaign-zones](solved/campaign-zones.md) (the zones and level count),
[monster-balance](monster-balance.draft.opus.md) (user, 2026-10-21: the levels are built on the balanced values),
[zone-looks](solved/zone-looks.md), and the new traps, so the rebuild places them:
[fire-jet-trap](solved/fire-jet-trap.md), [poison-drip-trap](solved/poison-drip-trap.md).
Built on [campaign-levelgen](campaign-levelgen.md) (the generator) and [campaign-texts](campaign-texts.md) (the texts
to place): both implemented, their play-test comes with this rebuild (user, 2026-10-26), so they don't block it.
Monster-balance already waits for every monster and boss draft, so they are all in before this.

## Idea

Every campaign level is generated again ([campaign-levelgen](campaign-levelgen.md)) from the zone table
([campaign-zones](solved/campaign-zones.md)): 40-60 levels in zones, each with its bosses, monsters, looks and
mechanics, using the whole 40 x 47 grid. Today's 35 levels are the reference for what a level carries, not a layout
to keep.

## Decided

* No format change: the grid stays 40 x 47 (`LEVEL_WIDTH`, `LEVEL_HEIGHT`). Halls, floors, side branches, loops
  (two ways to the same place), vertical shafts.
* **A ramp of open cells:** the first zone starts small (about 100-150 open cells, the way in) and grows to the full
  grid (500-700) by about the third zone. No jump like today's lvl14 (194) to lvl15 (663). The exact ramp goes in
  the zone table.
* **Redesign freely:** layouts, monsters, keys, riddles and chests may all change. Fixed: each zone's boss and
  the ankh on the last level. Monster count and treasure grow with the area, the monster mix comes from the zone.
  `docs/levels.md` gets a line per level again.
* **Carry over the content that is not layout:** inscriptions from level 1 on (2-3 a level, 1-2 in the first zone;
  the texts come from [campaign-texts](campaign-texts.md), the rules are in
  [hieroglyph-cartouches](solved/hieroglyph-cartouches.md): a reachable floor cell, mostly on or near the path, hints
  only), expedition pages, level titles ([campaign-texts](campaign-texts.md)), Hapi potions
  ([water-blessing-potion](solved/water-blessing-potion.md)), antidotes, mirror puzzles, ring transporters, the
  Stargate chamber, trapped chests, riddle and name gates. Same density per level or more; the spots may move.
* **Place every trap type:** [rhythm-traps](solved/rhythm-traps.md), dart plates, rock and sand falls,
  [fire-jet-trap](solved/fire-jet-trap.md), [poison-drip-trap](solved/poison-drip-trap.md).
* **Batches by zone:** one zone (or two short ones) per task, play-tested in between. The level files are written
  by the generator and then polished by hand or in the editor; they stay the source of truth.
* **Switching over:** the new levels replace `content/levels/lvl*` all at once, or zone by zone from the start, so
  the campaign always plays through. The zone table and `CAMPAIGN_LEVELS` switch with them.
* Each level passes `bin/levelcheck` with no warnings.

## Watch

* The spawn window (monsters spawn when they come into view), `MAX_MONSTERS` (32 live slots), the draft map's size on
  screen, the load time.
* The XP curve: recheck the XP sum per level after each batch against the curve from
  [monster-balance](monster-balance.draft.opus.md).
* Save games: old saves point to old level numbers. No real players yet: ignore them.

## Route fork: left or right

(Merged from the route-fork draft, 2026-10-19, from the user's review.)

A level splits into two one-way routes that both reach the exit. The player picks one, warned before the choice.
The last fork pays off the expedition's last page: "If you read this, take the left gate. I took the right."

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

Placement (user, 2026-10-21): forks come sooner too, several in the campaign. The last one on the level of the
expedition's last page (lvl34 today, `docs/levels.md`); earlier ones from the denser levels on, each hinted on the
way by an expedition page, an inscription, or both (the pages and texts for them are part of this plan). An early
fork is smaller: a short detour each way, not a whole level split.

levelcheck must prove the fork is fair: from the fork, each route alone reaches the exit (keys, levers and gates on
each route, not one key on the other side). A new warning: a one-way drop whose landing cannot reach the exit. Check
whether today's checker already finds that softlock; if so, this needs only a test on the fork level.

## Tests

* Inscriptions, campaign unit test: every campaign level has at least one inscription, each id is used once.
* Fork, unit: the checker passes each fork level and warns about a copy where one route's gate key lies on the other.
  Scenario with screenshots: a fork hall, its page, its inscription.
* Unit: every level's monsters are in its zone's set, every zone's boss is on its boss level.
* `bin/levelcheck content/levels/lvl*` with no warnings.
