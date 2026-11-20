# Credits: second section with music, font and agent credits, game version

Status: implemented 2026-11-20, waiting for the user's play-test. Was: draft 2026-11-20, size S, needs nothing first.

## Idea

The credits screen (`MainMenu::DrawCredits`, `src/ui/menu.cpp`) shows only the credits PNG sheet. Add a second
section on the same screen with the credits that are not on the sheet:

* Music: one line per track (track, author, licence), from `media/sounds/music/CREDITS.md`.
* Fonts: the fonts in `media/textures/fonts/` with author and licence, from their `*-LICENSE.txt` / `*-OFL.txt`.
* Agents: the AI agents that built the game (Claude, by Anthropic).
* Game version: the content of `.version`.

## Plan

* Section switch on the credits screen (a second page, key and button, like the other sub-screens; see
  [ui.md](../ui.md)). Section 1 stays the PNG sheet.
* Version: embed `.version` at build time (the Makefile already reads it, if not: add a generated header);
  the screen appends it to the text from `content/credits`.
* Text source: the credits (music, fonts, agents) are read from `content/credits` (does not exist yet: create it,
  a plain-text file, or a folder of them if the implementer prefers; document the format in
  [ui.md](../ui.md)). The game loads it, no credit text in C++. Fill it from `media/sounds/music/CREDITS.md`,
  the font license files and the agent line. Keep `CREDITS.md` as is.
* Long list: fit the panel with the shared UI look (small font, scroll or two columns), no text outside the panel.
* Scenario test in `tests/scenarios/` that opens the credits, switches section, screenshots to `tests/out/`.
* Glossary row if a new term is added.

## Play-test

Play command: `bin/game` (main menu, Credits), or `bin/game --level 1` and Esc, Main Menu, Credits.

* `bin/game`, main menu, Credits: More (or Right) switches to the second section; Up / Down or the wheel scroll it.
* Music, font and agent names are listed, the version matches `.version`.
* The first section (sheet) is unchanged; Esc returns to the menu.
