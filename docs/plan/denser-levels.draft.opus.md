# Rebuild the campaign: zone by zone, the whole grid, forks

Status: draft 2026-10-06 (denser levels), refined 2026-10-09; more-inscriptions and route-fork merged in 2026-10-21;
turned into the full rebuild 2026-10-21 (user: "levels re-generated completely, adjusting to the new bosses", 40-60
levels in zones). Slug kept from the first version.

Needs first: [campaign-zones](solved/campaign-zones.md) (the zones and level count),
[monster-balance](monster-balance.md) (user, 2026-10-21: the levels are built on the balanced values),
[zone-looks](solved/zone-looks.md), and the new traps, so the rebuild places them:
[fire-jet-trap](solved/fire-jet-trap.md), [poison-drip-trap](solved/poison-drip-trap.md).
Built on [campaign-levelgen](campaign-levelgen.md) (the generator) and [campaign-texts](campaign-texts.md) (the texts
to place): both implemented, their play-test comes with this rebuild (user, 2026-10-26), so they don't block it.
Monster-balance already waits for every monster and boss draft, so they are all in before this.

## Progress

Batch 1, the tomb entrance: done 2026-10-26, play-tested (user, 2026-10-26: "looks good").
Batch 2, the bat galleries (levels 5-9): done 2026-10-26, play-tested (user, 2026-10-26: "looks good").
Batch 3, the web halls (levels 10-13): done 2026-10-26, play-tested (user, 2026-10-26: "looks good").
Batch 4, the priests' quarter (levels 14-18): done 2026-10-26, waiting for the user's play-test (`bin/game --level 14`,
play to lvl19). Next: batch 5, the scorpion nest (19-22; today's 19-23, Seth's desert, give way: one level fewer; Seth
comes back with zone 8).
User, 2026-10-26: the bosses are retuned from the sim once all levels are done (not per batch).

* **Zone by zone from the start:** the rebuilt zones first (`REBUILT_LEVELS`, campaign.h), today's levels after them,
  one number earlier (today's lvl6 is lvl5, `CAMPAIGN_LEVELS` 34). Old lvl1-5 are gone; the decor tiers
  (`DECOR_TIER_FROM`), tests, play scripts and docs moved with the numbers. The drawings in `tools/level/campaign/`
  keep the old numbers (old helpers).
* **The ramp:** `ZONE_SIZE` (campaign.h): 120 to 280 open cells in the tomb entrance, 320 to 500 in the bat galleries,
  540 to 650 in the web halls, the whole grid on. levelgen builds small levels now (under 450 cells: only the halls
  the size needs, at least four, about 70% of the width); `bin/balance curve` follows the ramp.
* **The seed table:** `tools/level/rebuild.txt` (seed, difficulty, size, texts, page, title per level),
  `tools/level/rebuild.py LEVEL` generates one again.
* **Tomb entrance (1-4):** four levels as the target table (not five: with five the player would come to Khepri at
  level 8, she is tuned for 6). Difficulty 1, 1, 1, 2; 124, 183, 241 and 300 open cells. Rats, scarabs, bats, plants;
  keys only (levers come with the bat galleries, `LEVERS_FROM`), a riddle a level; inscriptions 53-58 (two, two, one,
  one), pages 1-3 on lvl1-3, titles kept from today's levels. Polished by hand: the bronze sword on lvl3 (Khepri is
  tuned for it), no mimic before lvl3 (page 3 tells of the biting chest), rats on lvl1. `bin/balance today`: the player
  comes to Khepri at level 7. All four pass the checker and their path scripts in the game.
* **Bat galleries (5-9):** difficulty 2; 323, 379, 412, 487 and 531 open cells. Bats, giant bats, worms, giant rats;
  levers come in (lvl5 opens its gate with one, lvl9 has one), rock falls (4 to 11 a level), a short wade on each
  level, a riddle each, name gates to secret rooms on 5, 6 and 8 (lore 33-35: `--lore`, the ids today's lvl5-9 used),
  inscriptions 59-65, pages 4-7 (5, 6, 8, 9; page 9, the early fork's, must follow page 8 on lvl10, so the first fork
  goes to the web halls). Titles from today's 5-9. Polished by hand: the one crocodile (the preview) moved off the way
  into a pool at the east end of lvl7's upper hall with a chest past it, so it can be skipped; three of lvl5's six
  worms made bats. The path scripts pass in the game; lvl8's only without its monsters (the giant bats' screech
  knocks the god-mode player off the ladders, the script cannot model it).
* **Web halls (10-13):** difficulty 3; 587, 600, 614 and 690 open cells, the whole grid from here. Camel spiders,
  giant scarabs, giant rats, mimics (30% of the chests); most halls dark with a few torches and braziers, webs under
  their ceilings; two trapped chests a level, a treasure tomb, a riddle; secret rooms on 10 and 11 (lore 36, 37, the
  last free ones: the other lore texts stay on today's later levels). 26 to 34 monsters a level (today's full levels
  have 27 to 38). The first fork on lvl12 (`--fork 1`): page 8 on the way, the left route through camel spiders, the
  right one to the earlier expedition's tomb; `tests/scenarios/route_fork.txt` shows it. Pages 8 and 9 swapped their
  texts (the fork's Day 11 first, the dead's Day 12 on the boss level, a hint at the priests' quarter); the fork page
  now tells of a dark long route with many legs in it instead of water and a priest's warning (no warning stands at an
  early fork). Today's 10-14 (the old scorpion nest and the queen) are gone, its pages 10 and 11 unplaced until the
  priests' quarter and the scorpion nest are rebuilt; 15-34 became 14-33. The decor tiers now go by zone
  (`DECOR_TIER_FROM` 1, 5, 10, 14). All four levels pass their path scripts in the game, with their monsters.
* **Priests' quarter (14-18):** difficulty 3, 3, 4, 4, 4; the whole grid. Mummies (8 to 17 a level, half between
  decoy coffins), living ushabti among their figures, giant scarabs, a few mimics, Anubis guards (one on lvl14, up to
  three later: the sim tuned them for the necropolis, a sprint gets past one); 3 to 5 fire vents a level and
  braziers, one trapped chest, a riddle; name gates to secret rooms on 14-17. The zone's "many inscriptions": 2 or 3 of
  its texts a level plus texts of today's campaign the replaced levels freed (`campaign.txt` 1, 3, 4, 6, 7, 10, 11,
  fitting the priests), one boon a level at most; the secret rooms' lore 38, 39 and two of those (6, 11); the
  boss level has no name gate (no lore left). Pages 10 (the ring) and 11 (the scorpions, on the boss level). The
  High Priest (`ZONE_BOSS`) is the zone's boss; today's lvl32 lost him: a mummy lies in his tomb, its boss gate gone,
  so the necropolis has no boss as the target table says. Today's 14-18 (the old serpent temple, Apep) are gone.
  The path scripts pass in the game with the monsters; lvl32's old path stalls among its monsters as before, its
  former boss room checked on its own (the teleporter's room walked to the exit).
* **XP (open, for the sim pass at the end):** `bin/balance today` puts the player at level 6 at Khepri (lvl4), 10 at
  lvl5, 15, 18, 21 and 26 at Shezmu (lvl9), 30 at lvl10. The monster-balance sim tuned the bat galleries for about
  level 8 and the web halls for 15 (its curve scales the monster XP by 0.27): the zone 2 monsters (giant rat 2000, giant
  bat 1800, worm 1200 XP) and Khepri's 6000 give about 4 times too much. Not changed per batch: the XP and the bosses
  are retuned together from the sim once all levels are rebuilt.
* Not yet: forks (from the bat galleries or the web halls on), the level gem still changes every five levels (not by
  zone), the XP over the whole campaign (the full levels' 45-55 monsters, see Watch).

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
  [monster-balance](monster-balance.md) (`bin/balance curve`). Today's levelgen puts 45-55 monsters on a
  full level (today's levels about 31): the player would end at level 79, not 60 (2026-10-26).
* From [monster-balance](monster-balance.md#done-2026-10-26): the first zone must bring the player to
  about level 6 and the sword before Khepri; the bat galleries' preview crocodile kills a level 8 player (drop it or
  make the wade skippable); levelgen builds no zone level under about 400 open cells (the ramp's first levels).
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
