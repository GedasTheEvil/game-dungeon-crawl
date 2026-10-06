# Inventory overhaul: sliders

Status: draft 2026-10-06.

## Problem

The inventory (`src/ui/inventory.cpp`, [../ui.md](../ui.md)) shows every item at once in fixed rows: four weapons,
then the potions. The potion row is full: with the antidote ([poison-and-antidote.md](poison-and-antidote.md)) it holds
8 slots, already narrowed to fit the panel. More item kinds are planned: amulets and a resistance potion
([resistance-potion-and-amulets.draft.md](resistance-potion-and-amulets.draft.md)), maybe more weapons.

## Idea

* **Groups:** the item kinds sorted into groups (weapons, potions, amulets, ...), picked with a horizontal slider or
  with tabs (not decided).
* **Items:** the items of the chosen group in a vertical slider, scrolled with the wheel, the arrows or a drag.
* The detail panel on the right stays.

## Open

* Groups: a horizontal slider or tabs.
* The groups themselves, and where a kind goes (the antidote: potions, or a group of its own with the resistance
  potion?).
* How many items show at once in the vertical slider; what a slot shows (model, name, count, level).
* Keys: the number row picks a slot today (`HOTKEYS`, `1234567890-=`). With more items than keys, what do the keys
  pick: the weapon and potion hotkeys in game stay as they are.
* Mouse: wheel scrolls the items, drag, scrollbar.
* Scenarios that click slots by screen position (`inventory.txt`, `props.txt`, `chest_pickup.txt`, `bow.txt`, ...)
  need new positions, or a scenario command that picks an item by kind.
