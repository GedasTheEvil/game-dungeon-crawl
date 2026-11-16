# Player manual (HTML): the game, controls, monsters, items

Status: draft 2026-11-16, size L, needs nothing first.

## Idea

A player manual in HTML for new or returning players. It looks as close to the game UI as HTML allows
([docs/ui.md](../ui.md): the backdrop, panels, papyrus scroll, `GOLD` / `BRONZE` / `LABEL` colours, the game's fonts as
web fonts). Sections: about the game, controls, monsters, items (potions, amulets, rings, weapons), conditions
(poison, bleeding, burning, daze, curses), skills.

No stats, a vague description instead. Example, the health potions: "Heals medium / large wounds. Stops bleeding."

## Plan

Decisions (the user, 2026-11-16):

* **Where:** its own directory, `docs/manual/`: one or more HTML files (`index.html` first) and its images
  (`docs/manual/images/`). Committed. Works offline from the folder, no build step to read it.
* **Monsters:** not a full list. 2-3 examples each for the normal and the giant monsters (e.g. rat and giant rat,
  cobra and giant cobra, scarab and giant scarab), each with a vague line on how it fights and how to deal with it.
  Bosses: say they exist (one per zone, their death opens the boss gates) and show one as the example, no full list.
* **Language:** English only for now.

The rest:

* Hand-written HTML and CSS (no generator: the manual lists examples, not every kind). One shared stylesheet in the
  game's look ([docs/ui.md](../ui.md): backdrop, panels, papyrus scroll, `GOLD` / `BRONZE` / `LABEL` colours, the
  game's fonts as web fonts from `content/`, copied or linked).
* Sections: about the game, controls (the default bindings, a note that Options can change them), monsters, items
  (potions, amulets, rings, weapons; examples, not every one), conditions (poison, bleeding, burning, daze, curses),
  skills (Heka), shrines.
* No stats, a vague description instead. Example, the health potions: "Heals medium / large wounds. Stops bleeding."
* Names: the in-game (Egyptian) names, the plain kind in brackets where it helps ("Chest of Set (mimic)").
* Images: monster renders (the model thumbnails), the inventory, skill and HUD icons; small PNGs.
* Drift check: a small test (`tools/docs_check.py` or `tests/unit/docs_test.cpp`) that the monster and item names
  the manual uses still exist in the game, and that every image link resolves.
* No JavaScript needed; `<details>` folds are fine where they help.

## Play-test

* Open `docs/manual/index.html` in a browser.
* It looks like the game's menus; sections for the game, controls, monsters (2-3 normal and giant examples, one boss), items; no numbers, only vague
  words.
