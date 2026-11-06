# Save remembers the stamina-skill toggle and the journal pages

Status: draft 2026-11-06, size S, needs nothing first.

## Idea

A load drops state the player set on purpose: the stamina-skill toggle (power hit, flaming arrows) comes back off,
the journal opens each section on its first spread. The save should keep them.

## Plan

* Stamina-skill toggle: `Player::staminaSkills` (`src/sim/player.h`, "Not saved"). Save and load it; drop the
  "Not saved" from its comment. The HUD sign shows it lit after a load.
* Journal: `JournalScreen` keeps the open section and the spread per section (`section`, `spread`,
  `src/ui/journal_view.h`), in memory only. Save both; a load opens the book where it was left, each section on its
  last spread. Clamp a spread past a section's end (pages can differ after a content change).
* Save format: add tagged lines the way `PlayerStats::Dump` does (`RESIST`, `WATER`, ...): an old save without them
  loads with today's defaults (toggle off, first spreads). The journal view is UI state, not `Journal` data: pass it
  through `SaveParts` (`src/sim/dungeon_save.cpp`) rather than mixing it into `Journal::Dump`.
* A new game resets both (toggle off, first spreads).
* Unit test: a save and load round trip keeps the toggle and the spreads; an old save without the lines loads.

## Play-test

* `bin/game --play tests/play/save-remembers-ui-state.txt`: turn the stamina-skill toggle on, open the journal on a
  later Creatures spread and another section, save, quit, load.
* The HUD sign is lit; the journal opens on the same section and spread; the other section on its last spread.
