# Campaign levelgen: generate campaign-quality levels per zone

Status: draft 2026-10-21, from the user ("I expect levels to be re-generated completely"). Needs first:
[campaign-zones](campaign-zones.draft.opus.md) (the zone table the generator reads). Large: split it into steps (see
below), one task each.

## Idea

The campaign is rebuilt by generating levels ([denser-levels](denser-levels.draft.opus.md)), not by drawing each one
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
   gates, teleporters, ring transporters ([ring-transporter](ring-transporter.draft.opus.md), once it exists).
6. **Boss levels:** the teleporter to the sealed boss room, the room shaped for the boss (Apep's pit, Sobek's lake,
   Seth's oasis, ...), the boss gate and the key behind it.
7. **Treasure and supplies:** chests, mimics, cursed mimics, trapped chests, antidotes near poisoners, Hapi potions
   before water, resistance potions, weapon chests by depth.
8. **Carried content:** inscriptions, expedition pages, the forks and their hints (see
   [denser-levels](denser-levels.draft.opus.md#route-fork-left-or-right)), the Stargate chamber
   ([stargate-chamber](stargate-chamber.draft.opus.md), once it exists), level titles.
9. **Decor:** decor and decal layers in the zone's style (`DECOR_*`, the editor's layers).

The rules already in `docs/levels.md` (per-level table, trapped chests, Hapi potions, antidotes, ...) become generator
rules, so the rebuilt campaign keeps them without placing them by hand.

## Tests

* Unit per step: generated levels with the new feature pass `checkLevel` with no warnings, over a range of seeds per
  zone.
* Scenario with screenshots: one generated level per zone.
* `level gen:SEED:DIFFICULTY` in scenarios keeps working; a new form takes the zone.
