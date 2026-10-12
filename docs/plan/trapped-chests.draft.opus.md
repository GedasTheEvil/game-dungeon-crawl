# Trapped chests

Status: draft 2026-10-12. Needs [treasure-containers](treasure-containers.draft.opus.md) first (closed chests that
hide their item, the opening clip the trap fires in). From the user.

## Idea

Some chests are trapped: opening one sets off a trap. Opening any chest becomes a small gamble, beside the mimic
(Chest of Set), which is a monster, and the [cursed mimic](cursed-mimic.draft.opus.md).

* Traps, fired during the opening clip:
  * poison darts from the lock (medium poison, like the dart plate, `src/world/dungeon_darts.cpp`);
  * a puff of poison gas (a small cloud, weak poison, a few seconds);
  * a curse (one of `Curse`, `src/world/curses.h`), framed as an execration text on the lid; ties in with
    [tomb-curse-triggers](tomb-curse-triggers.draft.opus.md);
  * optional: a scarab swarm crawling out (minions, like a boss's summons).
* A tell for an attentive player, never obvious from afar (as the cursed mimic's): scratch marks round the lock, a
  broken seal, a darker lid. It must survive [treasure-containers](treasure-containers.draft.opus.md)'s looks and
  orientations.
* The [interact prompt](interact-prompt.draft.opus.md) shows "[E] Open" on every chest, trapped or not, and on a
  mimic: it never tells them apart.
* Journal: a field note the first time a trap goes off ("Some chests bite back."), per trap kind if it reads better.

## Contents (decided with the user, 2026-10-12)

A trapped chest may hold treasure, not always: the risk pays sometimes.

* Rolled when it is opened, on the gameplay stream (`Rng`, as `RollChestLoot` in `src/world/loot.h`), so scenarios
  stay reproducible: about 50% the placed item and one bonus roll more than a plain chest; else empty.
* Empty: a status line with the archaeologist's joke ("Empty. The trap was the treasure.").
* Never put what the level needs in one: the checker does not count a trapped chest's item (an antidote does not
  count as "in reach", `checkLevel`, `src/world/level_check.cpp`), and level docs say so.

## Level format

A new tile type `TrappedTreasure` (16) with the same attribute and value as `Treasure` (the item type and id), a
legend character and an editor icon, like `Inscription` was added (`tile_defs.*`). No format change.

* Many places test `t.type == Treasure` (`Dungeon::PickUp`, the chest count, the loot, the checker's treasure counts,
  the decoration that leaves treasure cells bare): add `isTreasure(const Tile&)` (`src/world/level.h`) and use it, so
  none is missed.
* Trap kind: implementer's choice. Random per chest from the gameplay stream on opening, by depth (darts early, curses
  deep), or one tile type per kind. If [treasure-containers](treasure-containers.draft.opus.md) also needs per-chest
  data (a forced look), weigh one general extra field per object (level format v3) against more tile types.
* Boss rule (AGENTS.md): no monster gains an ability here.

## Tests

* Unit: each trap kind fires once on opening, the contents roll (the 50%, the extra bonus roll), the same seed opens
  the same way, a save and load before opening keeps it trapped; the checker ignores a trapped chest's antidote.
* Scenario with screenshots: the tell next to a plain chest, a trap going off.

## Open points

* The trap kinds to ship first, and how they are chosen (random, by depth, per tile type).
* The tell's look.
* The odds: 50% is the suggestion.
