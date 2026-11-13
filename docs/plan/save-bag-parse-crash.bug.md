# A save cut in the bag, or with an unknown bag tag, quits the game

Status: implemented 2026-11-13, waiting for the user's play-test. Was: draft 2026-11-13, size S, needs nothing first.

## Problem

`ItemBag::Load` (`src/level/item_bag.cpp:466`) reads the bag's first token and, when it is none of `INV2` ... `INV8`,
takes the oldest format: `counts[...] = std::stoi(tok)` (`item_bag.cpp:527`). `std::stoi` throws
`std::invalid_argument` on a token that is not a number, and on the empty string a failed read leaves. Nothing on the
way catches it: `readSaveGame` (`src/sim/dungeon_save.cpp:93`) wraps only its own first `std::stoi` in a `try`;
`loadSaveGame` and `GameState::LoadSave` (`src/state/game_state.cpp:503`) have none. The exception leaves a GLUT
callback through `glutMainLoop` and ends in the `catch` round the whole game in `main` (`src/game/game.cpp:201`, "Fatal
error") or in `std::terminate`: the game quits.

The same holds for `item_bag.cpp:536` (`type = std::stoi(tok)` on a token without a dot).

plan save-load-safety promised that a broken save leaves the game as it was (`loadSaveGame`, its test
`tests/unit/save_load_safety_test.cpp:48`). That test cuts the save in the map and before the end tag, never in the
player's part or the bag, so it misses this.

## Repro

1. Take any save (`saves/autosave.sav`) and cut it right after the stats (the line `OFFERINGS N`), or change the bag's
   tag `INV8` to `INV9` (a save from a newer build, after a `git checkout` of an older one).
2. Start the game, Continue (or Load that slot, or Retry on the death screen).

## Expected

"Could not load" as for any other broken save; the game stays on the menu as it was.

## Actual

The game quits: `std::invalid_argument` from `std::stoi` in `ItemBag::Load`.

## Fix

* `ItemBag::Load`: parse the legacy count and the legacy weapon type without throwing (`std::from_chars`, or
  `std::istringstream`), and set the stream's failbit when the token is neither a known tag nor a number, so
  `readSaveGame`'s `if (!in || ...)` reports the broken save.
* grep the save readers (`src/level/*.cpp`, `src/sim/dungeon_*.cpp`) for any other `std::stoi` / `std::stof`: today
  only `dungeon_save.cpp:98` (caught) and the two above.

## Fixed when

* A test that names the plan fails before the fix and passes after it: in `tests/unit/save_load_safety_test.cpp`,
  `loadSaveGame` on a good save cut right after the stats, and on one with `INV8` replaced by `INV9`, returns false
  (no exception) and leaves `saved(world)` as it was.

## Play-test

* Play command: `bin/game --level 1` (save once, then cut the copy).
* Copy a save, cut it after the `OFFERINGS` line (or change `INV8` to `INV9`), Load it: a "could not load" message, the
  game keeps running.
* Implemented: `ItemBag::Load` parses the legacy count and type with `std::from_chars` and sets the failbit on a bad
  token; no other `std::stoi` reads a save (only `dungeon_save.cpp:98`, caught). Test: `tests/unit/save_load_safety_test.cpp`.
