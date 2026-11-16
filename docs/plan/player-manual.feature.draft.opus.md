# Player manual (HTML): the game, controls, monsters, items

Status: draft 2026-11-16, size L, needs nothing first.

## Idea

A player manual in HTML for new or returning players. It looks as close to the game UI as HTML allows
([docs/ui.md](../ui.md): the backdrop, panels, papyrus scroll, `GOLD` / `BRONZE` / `LABEL` colours, the game's fonts as
web fonts). Sections: about the game, controls, monsters, items (potions, amulets, rings, weapons), conditions
(poison, bleeding, burning, daze, curses), skills.

No stats, a vague description instead. Example, the health potions: "Heals medium / large wounds. Stops bleeding."

## Plan

* Generated, not hand-written, so it does not drift from the game: a tool (a `bin/manual` target in C++ reading the
  data tables: `monster_kinds.cpp`, item and skill defs, the default key bindings / `controls_help`) writes
  `build/manual/index.html` plus its assets. The vague player texts live next to the data (a `manualText` field, or
  a table keyed by id) so a new monster or item without one fails a unit test.
* Hand-written parts (about the game, the story hook, basic tips) in one markdown or text file the tool includes.
* Pictures: monster and item renders (the model thumbnails, the inventory icons, the skill icons); the stars of the
  daze, the HUD tiles for the conditions.
* Names: the in-game (Egyptian) names, the plain kind in brackets where it helps ("Chest of Set (mimic)").
* Spoilers: bosses and late-zone monsters in a folded section ("Deeper in the tomb..."), closed by default; or zone
  by zone. Implementer's choice unless the user says.
* Controls: from the default bindings, with a note that Options can change them.
* One page, no JavaScript needed except the spoiler folds (`<details>`); works offline from the game folder.
* `make manual` builds it; `make check` builds it too (a missing text fails).

## Open questions for the user

* Spoilers: show bosses, or keep them hidden / folded?
* Shipped with the game (a menu link "Manual" that opens the browser) or only a web page?
* Language: English only?

## Play-test

* `make manual`, open `build/manual/index.html` in a browser.
* It looks like the game's menus; sections for the game, controls, monsters, items; no numbers, only vague words.
