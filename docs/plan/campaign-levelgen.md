# Campaign levelgen: generate campaign-quality levels per zone

Status: implemented 2026-10-25 (all nine steps), waiting for the user's play-test, which comes
with [denser-levels](denser-levels.md) (user, 2026-10-26: testable only on the rebuilt campaign). Preview:
`bin/game --level gen:1:6:hall_of_judgment:boss` (any zone's slug, with or without `:boss`). Draft 2026-10-21, from
the user ("I expect levels to be re-generated completely"). Needed first:
[campaign-zones](solved/campaign-zones.md) (the zone table the generator reads). Done in steps (see below).

## Idea

The campaign is rebuilt by generating levels ([denser-levels](denser-levels.md)), not by drawing each one
by hand. Today's `bin/levelgen` (`src/world/level_gen.cpp`, `docs/levels.md`) makes test levels: one-cell corridors,
ladders and drops, gates with keys or levers, spikes, rock falls, mimics, monsters by difficulty. The campaign levels
have much more. The generator must build levels like lvl15-35, from a zone description, and pass `bin/levelcheck`
with no warnings.

## Input and output

* Input: the zone (from the zone table), the level's place in the zone (first, middle, boss level), a seed, the
  target open cells and difficulty, and the content the level must carry (below).
* Output: a v4 level file in `content/levels/`. The level file stays the source of truth: an agent may polish it in
  the editor or by hand afterwards, and the generator is not run again over a polished level unless asked.
* Fixed seeds: the same input gives the same level. The seed per campaign level lives in one place
  (a table or the level header), so a level can be regenerated on its own.

## Progress

* **Step 1, shape: done (2026-10-24).** `bin/levelgen --zone SLUG [--cells N]`, `level gen:SEED:D:ZONE` in
  scenarios ([levels.md](../levels.md#zone-levels)): bands of halls 1-3 rows tall across the grid, ladders, drops and
  shafts, pillar loops, hub halls with wings, ledges; the keys and levers in the wings. Content (monsters by
  difficulty, test-level traps) stays the test level's until steps 3, 4 and 7. Not yet: the place in the zone (first,
  middle, boss level: step 6) and the seed table per campaign level (with the rebuild).
* **Step 2, zone mechanics: done (2026-10-24).** The zone table's terrain (`ZONE_TERRAIN`, campaign.h): wades,
  pools over deep water and flooded halls; sand drifts, quicksand, sand falls; dark halls with a few torches,
  braziers, daylight cracks; sun discs as gate openers through a mirror under a light shaft. Played in the game on
  monster-free copies (`levelcheck --script`): cisterns, Seth's desert (the mirror turned), Sokar and the web halls
  pass. With their monsters, path scripts can stall: a monster's bites drain the stamina before a pit's jump (a
  point for steps 3 and 7, or the monster-balance plan). Fire (the Lake of Fire's burning floors) waits for step 4
  (traps: fire jets).
* **Step 3, monsters by zone: done (2026-10-24).** `ZONE_FAUNA` (campaign.h): the target campaign's set per zone
  with weights and the zone's mimic shares; swimmers and lotuses only in half water, worms likelier in sand, camel
  spiders likelier in the dark halls (chosen before the monsters now), mummies between decoy coffins, living ushabti
  among decor ushabti. Climbers and roosting flyers need nothing of the generator: every climber in a set climbs,
  every flyer roosts under its hall's ceiling. Played in the game with their monsters, the path scripts of four of
  six zone levels pass; in the other two a grab (a crocodile's, a giant scorpion's) holds the god-mode player, so
  `levelcheck --script` cannot prove a level with grabbers. The level itself passes without its monsters.
* **Step 4, traps: done (2026-10-25).** `ZONE_TRAPS` (campaign.h): per zone the weights of spikes, death pits
  (the one-cell pits with a death trap, as before) and rock falls against a test level's, and of the new kinds along
  the halls: dart plates, wall spikes, fire vents, poison drips; and its trap gauntlets (0 to 2 a level: a stretch of
  a hall on the route, a trap on every other cell, now and then a pair of rhythm traps in step, checked crossable).
  An antidote comes with the plates and drips. Test levels stay byte-identical. `levelcheck --script` now waits
  before a run with wall spikes (the new scenario command `waittraps`): the path scripts of twelve monster-free zone
  levels (serpent temple, lake of fire, necropolis, scorpion nest, priests' quarter, hall of judgment, two seeds
  each, difficulty 8) pass in the game. The counts (e.g. 6-14 dart plates in the serpent temple, about 10-20 fire
  vents by the Lake of Fire) are a first guess, for the rebuild's play-tests to tune. Rock falls by an existing
  test-level weight already reach 20-50 in the bat galleries (200%).
* **Step 5, gates: done (2026-10-25).** `ZONE_GATES` (campaign.h): riddle gates (in a riddle room up a ladder or
  at a wing's end), a name gate by the zone's chance to a secret room sealed in the rock with a rare chest, its
  cartouche on the route; hidden rooms behind a plain teleporter (a treasure cistern, a hidden tomb); treasure tombs
  (a wing behind a second gate of a colour the route opens). Keys and levers chain where a free wing lies past the
  gate before. Test levels stay byte-identical. Played in the game: a hidden room's teleporter and a name gate
  (`speakname`) of a necropolis level take the player into their rooms; the path scripts of three monster-free
  levels pass. Left for later: boss gates (step 6, with the boss room), ring transporters (once
  [ring-transporter](ring-transporter.md) exists), the secret room's lore inscription (step 8, with the
  texts). Gates in half water only in tombs.
* **Step 6, boss levels: done (2026-10-25).** `GenOptions::boss` (`levelgen --boss`, `gen:SEED:D:ZONE:boss`) and
  `ZONE_BOSS` (campaign.h, the target campaign's bosses; none in the necropolis): the boss room sealed under the
  halls, a teleporter to it from a late wing, the boss gate and behind it the key of the last gate (on the last
  hall's crossing) and a rare chest. The room by the boss: egg clusters for the scorpion queen, Sobek's wade,
  Seth's pool between drifts, Rerek's sand, Babi's gaps, the Mother of Webs' braziers in the dark, Apep's pit
  between ledges, a tall room for the vampire bat and Anubis. Of the first and middle levels, only the boss level
  builds anything of its own so far. Played in the game (the boss kept, the other monsters taken out): the path
  scripts of nine of eleven boss levels pass; in Sobek's a crocodile he called blocks the way to the boss gate, in
  the High Priest's his curses shorten a later jump into a pit: the god-mode script models neither (as the grabs of
  step 3).
* **Step 7, treasure and supplies: done (2026-10-25).** The antidote in the wing nearest before the first hall with
  poison; lesser (and from the scorpion nest greater) resistance potions with poison; a Blessing of Hapi a few cells
  before the first wade on the way; the zone's trapped chests (`ZONE_TRAPPED_CHESTS`), plain potions only. Chests,
  mimics and cursed mimics by the zone's shares came with step 3, weapon chests by the difficulty as before (the
  difficulty stands for the depth until the rebuild's seed table gives each level its own). Not done: a cursed
  mimic always beside a real treasure (they fall where chests do, many in wings off the path). Test levels stay
  byte-identical.
* **Step 8a, carried texts: done (2026-10-25).** `GenOptions` takes the carried content: the inscriptions (given,
  or 2-3 of the zone's texts from `content/inscriptions/rebuild.txt` by `ZONE_TEXT_FIRST`, at most one boon, the
  one-thing hints only with their thing), an expedition page at a wing's end, the level's title; each secret room
  gets a lore text. The Stargate chamber, by the user's choice (2026-10-25), stands in as a password gate for now:
  `stargate` makes the level's name gate (its password the king's name) lead to a room with the ring's text
  (`STARGATE_TEXT`) and a rare chest; the real chamber replaces it once
  [stargate-chamber](stargate-chamber.md) is built. Played in the game: the name gate spoken, the ring's
  text copied in its room. Which page and texts each campaign level carries is the rebuild's choice (its seed
  table), so no id repeats over the campaign.
* **Step 8b, forks: done (2026-10-25).** `GenOptions::fork` (`--fork 1|2`): a hall split by a pillar under two
  side-by-side drops, the long left route and the short right one with its traps and the earlier expedition's
  remains and gear, both one-way into the join hall; on the last fork the warning (`FORK_TEXT`) between the drops
  and the page on the way. A requested fork, Stargate stand-in or page wins over a candidate without it. Played in
  the game: the path script of a necropolis fork level passes by the right route, and of a copy with the right drop
  filled by the left one. Not done: the early forks' own hint texts (none written yet), and the fork's
  "left / right gate" is two drops, the first one met walked into unless jumped.
* **Step 9, decor: done (2026-10-25).** The decor layer where it tells something: tombs and sealed rooms dressed
  (a sarcophagus, canopic jars, pottery, scrolls), the zone's guardian statue (`ZONE_GUARDIAN_PROP`) and a winged
  sun at the boss gate, webs in the web halls' dark halls; everything else stays the game's scatter by the zone's
  look. Checked in the game (screenshots of a boss gate and a tomb).
* All nine steps are in. Play-test: `bin/game --level gen:1:6:hall_of_judgment:boss` (or any zone's slug, with or
  without `:boss`); `tests/scenarios/campaign_levelgen.txt` shows one level per zone. Tuning left for the rebuild:
  the trap counts, the rare key chains, first and middle levels of a zone built alike, and the seed table per
  campaign level (its pages, texts, title, fork and Stargate level).

## What it must build

Missing today, roughly in this order (each step its own task, with its checker rules and unit tests):

1. **Shape:** halls 2-3 rows tall, the whole 40 x 47 grid, loops (two ways to the same place), vertical shafts,
   pillar loops, ledges, hub halls with wings.
2. **Zone mechanics:** half and deep water (pools, wades, flooded halls), sand, quicksand, sand falls, darkness and
   light (torches, braziers, light shafts), mirrors and sun discs. The zone table says which ones.
3. **Monsters by zone:** the zone's monster set instead of the difficulty unlocks; swimmers in water, burrowers in
   sand, climbers, roosting flyers, living ushabti among decor ushabti, mummies in coffins.
4. **Traps:** rhythm traps, dart plates, fire jets, poison drips, death pits, trap gauntlets.
5. **Gates:** riddle gates and riddle rooms, name gates with their cartouche, colour keys and levers in chains, boss
   gates, teleporters, ring transporters ([ring-transporter](ring-transporter.md), once it exists).
6. **Boss levels:** the teleporter to the sealed boss room, the room shaped for the boss (Apep's pit, Sobek's lake,
   Seth's oasis, ...), the boss gate and the key behind it.
7. **Treasure and supplies:** chests, mimics, cursed mimics, trapped chests, antidotes near poisoners, Hapi potions
   before water, resistance potions, weapon chests by depth.
8. **Carried content:** inscriptions, expedition pages, the forks and their hints (see
   [denser-levels](denser-levels.md#route-fork-left-or-right)), the Stargate chamber
   ([stargate-chamber](stargate-chamber.md), once it exists), level titles.
9. **Decor:** decor and decal layers in the zone's style (`DECOR_*`, the editor's layers).

The rules already in `docs/levels.md` (per-level table, trapped chests, Hapi potions, antidotes, ...) become generator
rules, so the rebuilt campaign keeps them without placing them by hand.

## Tests

* Unit per step: generated levels with the new feature pass `checkLevel` with no warnings, over a range of seeds per
  zone.
* Scenario with screenshots: one generated level per zone.
* `level gen:SEED:DIFFICULTY` in scenarios keeps working; a new form takes the zone.
