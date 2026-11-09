# Used-up potions, amulets and rings sort after the held ones

Status: implemented 2026-11-09, waiting for the user's play-test. Was: draft 2026-11-09, size S, needs nothing first.

## Idea

An empty slot (found once, none left: a potion drunk, an amulet or ring upgraded or merged away) is as useless now as
one not found. In the Potions, Amulets and Rings tabs the grid shows the held items first, then the empty ones, then
the ones not found yet, so the top rows hold what can be used.

## Plan

* `tabOrder`: Potions, Amulets, Rings: held, then empty, then not found; held and empty each in the tab's sort order
  (every order, the default "found" too). Weapons: unchanged (found, then not found); their copies never run out.
* No re-sort while the screen is open: an item the open inventory uses up (drunk, an amulet upgraded, a spare
  offered, a ring merged away) counts as held until it closes (`Inventory::usedUpHere`, cleared each tick while the
  screen is closed), so its slot stays put. Everything else still sorts live (a sort button, a new find).
* The layout functions (`positionOf`, `slotRect`, `hitAt`, ...) take the shown grid order instead of the bag and sort.
* Opened again: the selection is brought back into view, as the used-up item may have moved.
* Tests: unit `a potion used up goes after the held ones ...`, scenario `empty_slots_last`.

## Play-test

* `bin/game --play tests/play/empty-slots-last.txt`, open the Potions tab: Heal, Large Heal, Might, Cure held.
* Drink the Might in the inventory: its slot stays where it was. Close and reopen: it sits after the held potions,
  before the "?" ones. Try a-z / * / new: the empty one stays after the held ones.
