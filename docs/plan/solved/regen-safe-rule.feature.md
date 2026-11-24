# Regeneration: one safe rule for all, no damage in the last 5 s

Status: solved 2026-11-24 (play-tested by the user). Was: implemented 2026-11-24, waiting for the user's play-test. Was: draft 2026-11-24, size S, needs nothing first.

## Idea

The user: every regeneration (the lotus amulet, the garnet rings, the regeneration skill of
[skill-regeneration](../skill-regeneration.feature.md)) uses one rule for "safe": no HP lost in the last
5 s (`REGEN_DAMAGE_PAUSE_MS`, plan regen-after-damage), plus what blocks it today (burning, poison, bleeding, dead,
full HP). A monster that sees or chases the player no longer stops it; being hit does.

Today the amulet also needs `Dungeon::PlayerSafe()` (no monster threatens the player); the rings don't. The rule
change only touches the amulet: it now heals while chased, as long as nothing hurt the player for 5 s.

## Plan

* `PlayerStats::Regenerate` (`level/player_stats.cpp`): drop the `safe` gate from the amulet; one `healing` check
  (no poison, no bleeding, not `RegenBlocked`, alive, below max) for every source. Drop the `safe` parameter and
  the `dungeon.PlayerSafe()` argument (`sim/player_control.cpp:113`); keep `PlayerSafe` if other callers use it
  (`state/first_hints.cpp` does).
* Fix the comments that say "while no monster chases the player" (`level/items.h` `regenHpPerSecond`, the garnet
  comment "also in a fight" in `player_stats.cpp` and `items.h`).
* Texts: the lotus field note (`content/texts/journal.txt` `note.amulet-regeneration`: "while nothing chases me")
  and the garnet note/lore if it says "also in a fight"; glossary rows on regeneration, amulet, ring; the solved
  plans stay as history.
* Unit test (`tests/unit/`): amulet worn, a monster threatening, no loss for 5 s: HP rises; a loss: none for 5 s,
  then it rises again. Same for a garnet ring. Update tests that expect the amulet to stop while chased.

## Play-test

* `bin/game --play tests/play/regen-safe-rule.txt`: regeneration amulet and a garnet ring worn, HP 30 off, level 1 (monsters near).
* Let it chase you without being hit: HP rises.
* Get hit: no HP for 5 s, then it rises again.
