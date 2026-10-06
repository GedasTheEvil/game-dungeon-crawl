# Inventory overhaul: tabs

Status: draft 2026-10-06, refined 2026-10-06, ready.

## Problem

The inventory (`src/ui/inventory.cpp`, [../ui.md](../ui.md)) shows every item at once in fixed rows: four weapons,
then the potions. The potion row is full: with the antidote
([poison-and-antidote.md](solved/poison-and-antidote.md)) it holds 8 slots, already narrowed to fit the panel. More item
kinds are planned: amulets ([amulets.draft.md](amulets.draft.md)), a resistance potion
([resistance-potion.draft.md](resistance-potion.draft.md)), rings maybe, and many more weapons
([egyptian-weapons.draft.md](egyptian-weapons.draft.md)).

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
  rows (the weapons, [egyptian-weapons.draft.md](egyptian-weapons.draft.md)).
* **Opening tab:** the last one used, weapons the first time.
* **Scenarios** pick an item by its name or slug, not by screen position. A scenario command for it; the scenarios that
  click slots by position (`inventory.txt`, `props.txt`, `chest_pickup.txt`, `bow.txt`, ...) move to it.
* **Items not found yet:** a greyed-out question mark, "Not found yet". Found and used up (count 0): today's empty
  look, "None left".
* **Antidote and resistance potion** stay in the potions group.
* **Order** in a tab: `ItemKind` order, as today. Other sorting: [inventory-sorting.draft.md](inventory-sorting.draft.md).
* **Arrow keys:** left / right stay inside the current tab (up / down between its rows), as today within a row.
* **Keys:** the number row and the slot key labels keep today's function; tabs and keys are
  [inventory-keys.draft.md](inventory-keys.draft.md).
* The detail panel on the right stays.

## Open

Nothing; ready to implement.
