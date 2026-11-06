# Save remembers the player's settings: toggle, facing, lit arrows, inventory, journal pages

Status: solved 2026-11-06 (play-tested by the user). Was: implemented 2026-11-06, waiting for the user's play-test. Was: draft 2026-11-06, size M, needs nothing first.

## Idea

A load drops state the player set on purpose: the stamina-skill toggle (power hit, flaming arrows) comes back off,
the journal opens each section on its first spread, the player faces right, lit fire arrows are gone, the inventory
forgets its sort orders and tab. The save should keep them.

## Plan

* Stamina-skill toggle: `Player::staminaSkills` (`src/sim/player.h`, "Not saved"). Save and load it; drop the
  "Not saved" from its comment. The HUD sign shows it lit after a load.
* Journal: `JournalScreen` keeps the open section and the spread per section (`section`, `spread`,
  `src/ui/journal_view.h`), in memory only. Save both; a load opens the book where it was left, each section on its
  last spread. Clamp a spread past a section's end (pages can differ after a content change).
* Facing: the camera's yaw (`Camera::Facing`, `GameState::camera`; there is no `Dungeon::facing`), -1 / +1; saved in the
  `FACING` line (`UiViewState`, not `Dungeon::Dump`). A load
  turns the player (view, weapon side) the way they looked.
* Lit fire arrows: `Player::fireArrows` (`src/sim/player.h`, "not saved"): the count of bow shots a brazier lit. Save
  and load it; drop the "not saved".
* Inventory: `InventoryScreen` (`src/ui/inventory.h`): the sort order per tab (`sortOrder`, "not saved") and the
  selected tab (`tab`). Save both; a load opens the inventory on that tab, each tab sorted as left. The selected slot
  and scroll (`tabSlot`, `scrollRow`) are not saved: the bag can differ, they start at the top.
* Save format: add tagged lines the way `PlayerStats::Dump` does (`RESIST`, `WATER`, ...): an old save without them
  loads with today's defaults (toggle off, facing right, no lit arrows, found-first sort, Weapons tab, first spreads).
  The journal and inventory views are UI state, not `Journal` / `ItemBag` data: pass them through `SaveParts`
  (`src/sim/dungeon_save.cpp`) rather than mixing them into `Journal::Dump` or `ItemBag::Save`.
* A new game resets all of them to those defaults.
* Unit test: a save and load round trip keeps each value; an old save without the lines loads with the defaults.

## Play-test

* `bin/game --play tests/play/save-remembers-ui-state.txt` (level 1, bow and Flaming arrows learnt): Q turns the
  stamina-skill toggle on, light arrows at the brazier (col 13), face left, sort a tab and leave the inventory on
  Potions, open the journal on a later Creatures spread and another section, save, quit, load.
* The HUD sign is lit, the next bow shot burns, the player faces left.
* The inventory opens on the same tab, sorted as left; the journal on the same section and spread, the other section
  on its last spread.
