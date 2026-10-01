# Loading bar jumps forward and back near the end

Status: draft 2026-10-01.

## The problem

Towards the end of the loading screen the bar runs to full, then jumps back and fills again.

## Cause (from reading the code, not yet checked in play)

`Assets::Load` (`src/state/assets.cpp`) counts the percentage by hand:

* starts at 30;
* `loadMonsterTypes`: +5 per `MONSTER_DEFS` entry, 12 entries: 30 → 90;
* `loadItems`: +2 per `ITEM_DEFS` entry, 6 entries: 90 → 102 (`DrawLoad` clamps at 100);
* then fixed values: `progress(80, …)`, `83`, `95`, `100`.

So the bar reaches 100 during the items, drops back to 80, and fills again. The steps were sized when there were
fewer monsters; every new monster (the latest, the vampire bat) pushes the bar further.

## Fix idea

Compute the steps from the table sizes, not fixed increments. For example, give each phase a share
(monsters 30–65, items 65–80, then the fixed steps) and step by `share / std::size(DEFS)`. Or count the total
number of load steps once and report `done / total`. The bar then only moves forward, however many monsters or
items get added.

Check: a scenario that records the `DrawLoad` percentages (the `loading` log has only the text now; add the value) and asserts they never go
down.
