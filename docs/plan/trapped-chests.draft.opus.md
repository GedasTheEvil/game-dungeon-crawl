# Trapped chests

Status: draft 2026-10-12. Needs [treasure-containers](solved/treasure-containers.md) first (closed chests that
hide their item, the opening clip the trap fires in). From the user.

## Idea

Some chests are trapped: opening one sets off a trap. Opening any chest becomes a small gamble, beside the mimic
(Chest of Set), which is a monster, and the [cursed mimic](cursed-mimic.draft.opus.md).

* Traps, fired during the opening clip:
  * poison darts from the lock (medium poison, like the dart plate, `src/world/dungeon_darts.cpp`);
  * a puff of poison gas (a small cloud, weak poison, a few seconds);
  * a curse (one of `Curse`, `src/world/curses.h`), framed as an execration text on the lid. A trapped canopic jar
    ([treasure-containers](solved/treasure-containers.md)) curses by default (disease);
  * optional: a scarab swarm crawling out (minions, like a boss's summons).
* A tell for an attentive player, never obvious from afar (as the cursed mimic's): scratch marks round the lock, a
  broken seal, a darker lid. It must survive [treasure-containers](solved/treasure-containers.md)'s looks and
  orientations.
* The [interact prompt](solved/interact-prompt.md) shows "[E] Open" on every chest, trapped or not, and on a
  mimic: it never tells them apart.
* Journal: a field note the first time a trap goes off ("Some chests bite back."), per trap kind if it reads better.

## Curses from objects: one rule

This plan owns the rule for every curse an object sets off; [tomb-curse-triggers](tomb-curse-triggers.draft.opus.md)
uses it for the objects that are not treasure (a sarcophagus, a coffin, pottery, an ushabti):

* One roll per object, on opening, from the gameplay stream. A container that is trapped rolls its trap only; no
  second curse roll on top (a canopic jar is a container, its curse is its trap).
* The tell: a close-up hint, never a status line or a prompt text, never obvious from afar.
* The trap-ward amulet's chance (below) covers every one; the [Blessing of Isis](solved/skill-blessing-of-isis.md)
  halves the curse's time, as any curse.
* Loot in a cursed object follows [Contents](#contents-decided-with-the-user-2026-10-12).

## Contents (decided with the user, 2026-10-12)

A trapped chest may hold treasure, not always: the risk pays sometimes.

* Rolled when it is opened, on the gameplay stream (`Rng`, as `RollChestLoot` in `src/world/loot.h`), so scenarios
  stay reproducible: about 50% the placed item and one bonus roll more than a plain chest; else empty.
* Empty: a status line with the archaeologist's joke ("Empty. The trap was the treasure.").
* Never put what the level needs in one: the checker does not count a trapped chest's item (an antidote does not
  count as "in reach", `checkLevel`, `src/world/level_check.cpp`), and level docs say so.

## The trap-ward amulet (decided with the user, 2026-10-12)

No new skill: the trap-ward amulet (Eye of Horus, `AmuletType::TrapWard`, tiers 25 / 50 / 75 / 100,
`AmuletBonus::trapCutPercent`, `src/world/items.*`) covers trapped chests.

* It cuts a chest trap's damage as it does the floor traps', and with the same percentage it is the chance that the
  trap does not go off at all. Expected harm: lesser 56%, minor 25%, normal 6%, grand none (immune, as today).
* The chance also covers the traps the damage cut cannot (a curse, a swarm).
* Rolled on opening from the gameplay stream, apart from the contents roll: a trap that does not go off leaves the
  contents as they are. It is spent either way (the chest is open).
* Amulet text: "Trap damage -%d%%" becomes e.g. "Traps -%d%%: damage, and chests' traps go off less"; the grand one
  keeps "Immune to traps".

## Level format

A new tile type `TrappedTreasure` (the next available ID, possibly 16) with the same attribute and value as
`Treasure` (the item type and id), a legend character and an editor icon, like `Inscription` was added (`tile_defs.*`). No format change.

* Many places test `t.type == Treasure` (`Dungeon::PickUp`, the chest count, the loot, the checker's treasure counts,
  the decoration that leaves treasure cells bare): add `isTreasure(const Tile&)` (`src/world/level.h`) and use it, so
  none is missed.
* Trap kind: implementer's choice. Random per chest from the gameplay stream on opening, by depth (darts early, curses
  deep), or one tile type per kind. If [treasure-containers](solved/treasure-containers.md) also needs per-chest
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
