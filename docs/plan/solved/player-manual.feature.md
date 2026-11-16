# Player manual (HTML): the game, controls, monsters, items

Status: solved 2026-11-16 (play-tested by the user). Was: implemented 2026-11-16, waiting for the user's play-test. Was: draft 2026-11-16, size L, needs nothing first.

## Idea

A player manual in HTML for new or returning players. It looks as close to the game UI as HTML allows
([docs/ui.md](../../ui.md): the backdrop, panels, papyrus scroll, `GOLD` / `BRONZE` / `LABEL` colours, the game's fonts as
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
  game's look ([docs/ui.md](../../ui.md): backdrop, panels, papyrus scroll, `GOLD` / `BRONZE` / `LABEL` colours, the
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

## Implementation

* `docs/manual/index.html` (one page, a tab row of anchors to its sections) and `docs/manual/manual.css` (the
  colours of `src/render/ui_draw.h`: backdrop with vignette, title with rules and diamonds, panels with bronze frame,
  gold line and studs, lapis / stone tiles, key caps, papyrus cards with the ink rule, icon wells). No JavaScript; the
  curses sit in a `<details>` fold.
* Font: the game has only the Parchment MF sheet (`media/textures/fonts/parchment.png`); its TTF says "all rights
  reserved", so the repo does not carry it (the user, 2026-11-16). The CSS takes Parchment MF where it is installed
  (`local()`), else Palatino / Georgia.
* Pictures (`docs/manual/images/`, made by `tools/manual/images.py`): monster renders from `bin/model-viewer`
  (backdrop keyed out), the HUD, inventory and Heka screens from `tools/manual/shots.txt` run through `bin/game`,
  skill icons from `skill_icons.png`, weapon, amulet, potion (tinted) and ring (gem recoloured) icons from
  `hud_icons.png`, conditions, curses and blessings on their `status_cards.png` card, the backdrop and papyrus. The
  script removes pictures no page uses.
* Monsters: rat / giant rat, scarab / giant scarab, cobra / giant cobra, the Chest of Seth (mimic), Khepri as the
  boss example. Items: all eleven potions, eight weapons, eight amulets, the six ring gems and the metals.
* Drift check: `tools/docs_check.py` (`make check`): every `class="name"` text must be in a string of `src/` or a
  line of `content/texts/`, every local `src` / `href` must exist. `class="term"` is the same look, unchecked (Daze).
* Rows: glossary (player manual), where-is-x, readme (Play).
* No play command: the manual is not in the game (`move --force`).

## Play-test

* `xdg-open docs/manual/index.html` (any browser, offline).
* It looks like the game's menus; sections for the game, controls, monsters (2-3 normal and giant examples, one boss), items; no numbers, only vague
  words.
