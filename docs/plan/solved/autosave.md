# Autosave and the death screen

Status: solved 2026-10-20 (play-tested by the user: `bin/game --play tests/play/autosave.txt`). Draft 2026-10-19,
from the user's idea.

## Idea

Quality of life: a death no longer means "load a slot or start over". One autosave the game writes itself at safe
moments, and a death screen that offers to continue from it.

## The autosave

* **One file:** `saves/autosave.sav`, the same format as a slot save (`GameState::Save` / `LoadSave`). The player
  can't write it: it is not in the Save Game list. The Load Game list shows it first, read-only, named "Autosave"
  with its level and time (as `SaveSlots::Info`).
* **When:**
  * **Level start:** New Game's level 1 and every level reached by the exit (`Dungeon::LoadCampaignLevel`), after
    the level is in place and before the first tick. Not on a load (the save already exists).
  * **Riddle answered:** the right answer, after the reward is in (XP, journal note, run statistic). Not on a wrong
    answer or a walk away (Esc).
  * **Boss slain:** after its death is handled and the lair gate opens. The hardest fight of a level should not be
    replayed.
* **Never in danger:** skip a trigger (the old autosave stays) while the player is poisoned, burning, held (the hold
  of plan crocodile-hold-bite), climbing, falling or travelling. A save at 1 HP with poison running would be a death
  loop. Level start is always safe (full state as the player left the last level); a skipped riddle or boss trigger
  is not retried.
* **No other triggers:** no timer, no chest or pickup, no potion. Each would land mid-danger sooner or later, and a
  timer would make the autosave a hidden quicksave.
* **Feedback:** a toast "Autosaved" (as `ShowToast` after a slot save), small, no pause.
* **Dev and tests:** scenario tests never write `saves/autosave.sav` (a scenario's own `savegame` command stays as
  is). `--level N` and `--play` runs do write it: the user play-tests the death screen there.

## The death screen

Today a death draws `textures/ui/dead.png` over the dying player. Instead, a panel in the shared UI look
([ui.md](../../ui.md)), after the death clip has played (about a second, so the death is seen):

* Title "You died" (or the dead.png art inside the panel, implementer's choice).
* Two buttons:
  * **Retry from Autosave:** loads it. Greyed out, not hidden, when there is no autosave file. (Not "Continue":
    that word is the main menu's button, see below.)
  * **Main Menu:** as the in-game menu's Main Menu.
* Keys: arrows / mouse move the focus, Enter picks, Esc = Main Menu. Focus starts on Retry from Autosave when it is
  enabled.
* The world keeps drawing behind the panel (dimmed), no ticks.
* Dead state is checked where it is today (`Player::Alive`): the death screen is a screen state like the menu, so
  the in-game menu, inventory and journal can't open over it.

## Run statistics and leaderboard

A slot load already undoes a death, so the autosave adds no new cheat: `run.campaign` stays as saved, and a won run
after a retry still goes on the leaderboard.

**Deaths counter** (user, 2026-10-19): `RunStats::deaths`, written in its "RUN ..." save lines (an old save without
it reads 0). A load would put the old count back, so at the moment of death the game counts it and rewrites the
autosave file with the new count (only that number changes, the rest of the autosave stays as it was). The retry
and the main menu's Continue then read it from the file, also after the game was quit on the death screen. A slot
load keeps the slot's own count (known gap: dying and loading an older slot loses the deaths since; accepted).
Shown in the Hall of Records next to the run's other statistics. A clean run shows 0.

## Continue in the main menu

When `saves/autosave.sav` exists, the main menu's first button is "Continue" (loads it), New Game below it. Without
the file the button is not there (the menu looks as today). One click back into the run after a restart of the game.

## Tests

* Unit: the danger guard (a poisoned / held / burning player skips, a healthy one saves); the autosave is not in the
  slot list and can't be overwritten by `SaveSlots`.
* Scenario: die in a level, the death screen shows both buttons (screenshot), Retry from Autosave puts the player at the
  level start with the level-start HP and items; with no autosave file it is greyed out. The deaths
  counter survives the retry (1 after one death). The main menu shows Continue only with an autosave file.
* Play-test: `tests/play/autosave.txt`: a level with a riddle gate and a monster, low HP, so the user can answer,
  die and continue.

## Implemented (2026-10-20)

* `GameState::Autosave(AutosaveMoment)` writes `AutosavePath()` (a scenario's: `tests/out/<name>/autosave.sav`) and
  shows "Autosaved" top left over the game. The world asks for it with a `WorldEvent` (`AutosaveMoment::LevelStart`
  after the exit's `LoadCampaignLevel`, `Boss` when the boss's gates open); `NewGame` and the riddle (also a late
  answer from the journal) call it. The guard: `Dungeon::PlayerInDanger` (poisoned, burning, held, on a ladder,
  jumping or falling, travelling); a level start skips it, a dead player never saves.
* `--level` and `--play` write it once their setup has run. The scenario command `autosave` writes it as at a level
  start; `expect deaths`, `expect death_screen`.
* Death screen: `DeathScreen` (`ui/death_screen.*`), `Screen::Death`, opened `DEATH_SCREEN_DELAY_MS` (1.2 s) after
  the death, from the game only (a menu opened in the death clip delays it). Title "You died" as text; the old
  `dead.png` quad and the file are gone. The screen keys of a dying player do nothing.
* Deaths: `RunStats::deaths` in a "DEATHS n" line after "NAMES"; `GameState::CountDeath` patches that line in the
  autosave file (`RunStats::SetDeaths`). Leaderboard key `deaths`, the run's details "Deaths" under Fights.
* Load Game: the autosave across the top of a taller panel, the slots 12.5 high under it; "No autosave yet"
  greyed. The main menu with an autosave: seven buttons on a taller panel, Continue (lapis) on top.
* Tests: `tests/unit/autosave_test.cpp`, `tests/scenarios/solved/autosave.txt` (parked: screenshots of the screens; it reads the
  real `saves/` slots in Load Game, never clicks one).

