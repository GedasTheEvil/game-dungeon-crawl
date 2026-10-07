# Inventory overhaul: tabs

Status: draft 2026-10-06, refined 2026-10-06, implemented 2026-10-06; waiting for a check in play.

## Problem

The inventory (`src/ui/inventory.cpp`, [../ui.md](../../ui.md)) shows every item at once in fixed rows: four weapons,
then the potions. The potion row is full: with the antidote
([poison-and-antidote.md](poison-and-antidote.md)) it holds 8 slots, already narrowed to fit the panel. More item
kinds are planned: amulets ([amulets.md](amulets.md)), a resistance potion
([resistance-potion.draft.md](../resistance-potion.draft.md)), rings maybe, and many more weapons
([egyptian-weapons.md](egyptian-weapons.md)).

## Decided

* **Tabs** pick the group: weapons, potions, amulets, rings. Four tiles across the top of the items panel, in place of
  the "Arms" / "Elixirs" headings. Look of the screen tabs (`ui::screenTabs`): lapis active, stone the others.
* **Tab icons:** flat, pre-rendered into a texture, like the HUD icons (`tools/textures/hud_icons.py`, sword and flask
  are there already; amulet and ring are new).
* **Disabled tab:** a group the player has no item of yet is dimmed and cannot be clicked; on hover its name and
  "none yet". This also covers groups with no item kinds in the game (amulets, rings today), so they can ship as tabs
  now.
* **Grid:** the same for every tab, 4 items per row, as many rows as the group needs (rows computed from the item
  count). Every slot the size of today's weapon slot (19 x 22). One `slotRect`, one model scale, one name band. Two
  rows fit the panel at once; potions (8 kinds) fill them, their models grow from 9.8 x 15.5 slots.
* **Scrolling:** a group with more rows than fit scrolls by whole rows (wheel, arrow down past the last visible row, a
  thin scroll bar shown only then). Keep the layout open for it, but implement it later, when a group first passes two
  rows (the weapons, [egyptian-weapons.md](egyptian-weapons.md)).
* **Opening tab:** the last one used, weapons the first time.
* **Scenarios** pick an item by its name or slug, not by screen position. A scenario command for it; the scenarios that
  click slots by position (`inventory.txt`, `props.txt`, `chest_pickup.txt`, `bow.txt`, ...) move to it.
* **Items not found yet:** a greyed-out question mark, "Not found yet". No name, type, effect or lore either (play test:
  the player must not learn it exists): the slot has an empty name band, the details say "Unknown" and "Still hidden
  somewhere in the tomb...". Found and used up (count 0): today's empty look, "None left".
* **Antidote and resistance potion** stay in the potions group.
* **Order** in a tab: `ItemKind` order, as today. Other sorting: [inventory-sorting.md](inventory-sorting.md).
* **Arrow keys:** left / right stay inside the current tab (up / down between its rows), as today within a row.
* **Keys:** the number row and the slot key labels keep today's function; tabs and keys are
  [inventory-keys.draft.md](../inventory-keys.draft.md).
* The detail panel on the right stays.

## Implemented

* `ItemGroup` and `groupItems` (`src/world/items.h`); the screen in `src/ui/inventory.cpp`, look in
  [../ui.md](../../ui.md#inventory-group-tabs).
* "Found" per item in `ItemBag` (`Found`, `AnyFound`): a potion used up is found, one never had is not. Needed for
  the question mark vs "None left" and for the disabled tabs. The save tag is `INV3` (found flags after the levels);
  `INV2` saves load with found = held.
* Icons: amulet and ring added to `tools/textures/hud_icons.py` (cells 5, 6); the potions tab tints the flask red.
* A disabled tab on hover: a label under it, "Amulets: none yet".
* The number row skips a slot whose tab is disabled (a potion key before any potion was found).
* The detail panel of a used-up potion shows its effect and lore (before: "Effect unknown").
* Scenario command `select ITEM` ([../testing.md](../../testing.md)); `inventory.txt`, `props.txt`, `bow.txt`,
  `weapons_held.txt` use it. `inventory.txt` still clicks one slot and the tabs by position, to test the mouse.
* Scrolling: not built, as decided.
