# Leaderboard

Status: implemented 2026-10-13, waits for the user's play-test: `./game --play tests/play/leaderboard.txt` (a short
level marked as a campaign run; its board goes to `tests/out/leaderboard/`, not `saves/`). Draft 2026-10-12, refined
2026-10-13. From the user. What was built: [Implemented](#implemented-2026-10-13).

Opus: a run statistics system (counters in the player state and the save file), a name entry on the win screen, a
leaderboard file and a new menu sub-screen.

## Idea

The player wins the game (takes the ankh, `Dungeon::won`, `EndScreens::DrawWin`), types a name, and the run is added
to a leaderboard. The leaderboard is a file in `saves/` and has its own screen in the main menu.

## Run statistics

Counted from New Game to the win. The current run's counters ride in the save game (not the leaderboard itself), so
a run survives quitting, and a load puts them back to the saved values: a fight replayed after a load counts once.

From the user:

* Monsters slain, total and per kind (bosses too).
* Potions drunk, total and per kind.
* Player stats at the end: player level, XP, max HP, stamina, armour, might.
* Skills learned at the end.
* Play time (user, 2026-10-13: recorded). Time in play only: paused, menus, the Heka screen and the inventory do
  not count. It is kept in the save, so it adds up over sessions. A cut-scene
  ([cinematic-effects](cinematic-effects.draft.opus.md)) pauses the world: it does not count either.
* Exploration (user, 2026-10-13): inscriptions read (the cartouches) and treasures found (treasure cells picked
  up), each counted once per cell. Riddles solved count here too.

Added (implementer picks; the first ones are cheap):

* Damage dealt and damage taken.
* Biggest single hit dealt.
* Secrets found (if the level data marks them).
* Time per level (splits), shown on the run's detail view.
* Date of the win and the game version (git hash at build time, or a version number). Balance changes make old
  runs not comparable; the version tells.

## Leaderboard file

* Its own file, `saves/leaderboard.txt` (or `.ini`, like `saves/settings.ini`), never part of a save game (user,
  2026-10-12): it outlives every save slot and a new game. Append only: one entry per win. A broken line logs a
  warning and is skipped, the rest still loads.
* An entry holds the name, the date, the version and every statistic above. A small `key=value` per entry keeps
  old entries readable when new statistics arrive.
* No cap on entries in the file; the screen shows the best 10 (or a scroll list).

## Win flow

1. The win screen shows a run summary (time, slain, potions, player level).
2. A name field, the same text entry as save slot names (`SaveSlots`, `src/ui/menu.cpp`). Default: the last name
   used. Esc skips the entry (no record).
3. Enter writes the entry and opens the leaderboard with the new row marked.

## Leaderboard screen

* A main menu sub-screen in the shared look (docs/ui.md), e.g. "Hall of Records" (Egyptian flavour; the name is the
  implementer's choice). Add the term to docs/glossary.md.
* The list (user, 2026-10-13): three columns only, rank, name and score. The score is the chosen sort order's value.
* Sort buttons above the list, one per order; the rank and the score follow the chosen order:
  * Time: fastest first (the default). The score is the play time (`h:mm:ss`), lower is better.
  * XP: most XP at the end first. The score is the XP.
  * Explored: highest first. The score is a weighted sum: inscription x10, treasure x10, riddle x30 (riddles are
    rarer and harder; user, 2026-10-13). The weights sit in one place.
  Ties in XP or exploration: the faster run first. More buttons (slain, ...) are the implementer's choice.
* Clicking a row (or Enter on it) opens the run's detail screen: every statistic of the run (time, XP, player level
  and end stats, skills, slain and potions per kind, inscriptions / treasures / riddles, splits, date, version). Esc
  goes back to the list, the sort kept.
* Empty board (user, 2026-10-13): when the file has no entry (or is missing), the list shows one placeholder row,
  "Spirit of Exploration", player level 1 (on its detail screen), score "-". It is not written to the file and is
  gone once a real run is recorded.

## Eligibility

* Only a run from New Game on level 1 counts. A scenario, a level picked by a dev flag or a run started on a later
  level is marked in the stats (a `campaign` flag) and the win screen offers no entry.

## Save format

The counters are new fields in the save (`GameState::Save`, `src/state/game_state.cpp`). Old saves do not matter
(user, 2026-10-12): no fallback, no version number needed; old saves may break.

## Decided (2026-10-13)

* Play time is recorded; the board sorts by it by default, with sort buttons for XP and exploration.
* The list shows rank, name and the chosen order's score; everything else is on the row's detail screen.
* Scores: time as `h:mm:ss` (not converted to points); exploration weights 10 / 10 / 30.
* The leaderboard is a main menu item from the start, with the placeholder row while no run has won.
* Saves, loads and deaths are not recorded (a death is undone by a load, so it would only count loads).

## Tests

* Unit tests: the counters (a kill, a potion, a load restores the saved counts), the leaderboard file (write, read
  back, a broken line skipped, each sort order and its score, the placeholder row on an empty or missing file).
* A scenario: win a small level, type a name, screenshot the leaderboard screen.

## Implemented (2026-10-13)

* Counters: `RunStats` (`src/world/run_stats.*`), a member of `PlayerStats` (`run`), so New Game resets it and the
  save carries it (a `RUN` line). Kills in `Dungeon::rewardKill` (every kill that gives XP: weapon, poison, trap),
  damage dealt and the biggest hit in `Dungeon::playerHit`, damage taken in `Player::TakeHit` (hits and traps, not
  poison), potions in `Inventory::DrinkPotion`, treasures in `Dungeon::PickUp`, inscriptions on their first reading,
  riddles on a right answer (also late, from the journal). Times poisoned (user, 2026-10-13) in `Player::Poison` when
  it takes (a warded one does not count), times cursed in `PlayerStats::LayCurse` (also one laid again while it runs);
  both on the detail screen under Fights. Play time in the game loop's tick, only while it runs in
  play (alive, not won): menus, the UI screens and the riddle do not count; kept per level (the detail screen's
  "Levels").
* Campaign flag: set by New Game only; scenarios and `--level` / `--play` runs are not campaign runs, the scenario
  command `campaign` marks one (for tests and the play-test).
* File: `src/world/leaderboard.*`, `saves/leaderboard.txt`, one `[run]` block of `key=value` lines per record,
  appended. A broken line is skipped with a log warning, a record without a name too; unknown keys are ignored.
  Scenarios and play runs write to their output directory instead.
* Version: the git hash (`git describe --always --dirty`), passed by the makefile to `src/ui/win_entry.cpp` only.
* Win: `WinEntry` (`src/ui/win_entry.*`), a panel under the win picture's text (the HUD hidden meanwhile): time,
  slain, potions, player level, the name field (default: the latest record's name). Enter records and opens the Hall
  of Records with the row marked (the menu is the main menu then: the run is over); Esc skips.
* Screen: "Hall of Records", the main menu's third button, `RecordsView` (`src/ui/records_view.*`). Ten rows visible,
  the wheel or Up / Down scroll; Enter or a click opens the details (the run's date and version in its footer).
* Not done: secrets found (the level data marks none).
* Fixed on the way: Load Game after a win kept the game won (`GameState::LoadSave` clears it now).
* Tests: `tests/unit/leaderboard_test.cpp`; scenarios `leaderboard` (win, name, list, XP order, details) and
  `leaderboard_empty` (the menu button, the placeholder row and its details), level `tests/levels/win_run`.
