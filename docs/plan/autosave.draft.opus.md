# Autosave and the death screen

Status: draft 2026-10-19, from the user's idea. Needs nothing first.

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
([ui.md](../ui.md)), after the death clip has played (about a second, so the death is seen):

* Title "You died" (or the dead.png art inside the panel, implementer's choice).
* Two buttons:
  * **Continue** (from the autosave): loads it. Greyed out, not hidden, when there is no autosave file.
  * **Main Menu:** as the in-game menu's Main Menu.
* Keys: arrows / mouse move the focus, Enter picks, Esc = Main Menu. Focus starts on Continue when it is enabled.
* The world keeps drawing behind the panel (dimmed), no ticks.
* Dead state is checked where it is today (`Player::Alive`): the death screen is a screen state like the menu, so
  the in-game menu, inventory and journal can't open over it.

## Run statistics and leaderboard

A slot load already undoes a death, so the autosave adds no new cheat: `run.campaign` stays as saved, and a won run
after a continue still goes on the leaderboard. Optional, the implementer's choice: a `deaths` counter in
`RunStats` (kept across the load: the death is counted before the load, then written back), shown in the Hall of
Records. Without it, the record can't tell a clean run from one with ten continues.

## Optional extra

The main menu's first button "Continue" when an autosave exists (loads it), New Game below it. Common in games, one
click back into the run after a restart of the game.

## Tests

* Unit: the danger guard (a poisoned / held / burning player skips, a healthy one saves); the autosave is not in the
  slot list and can't be overwritten by `SaveSlots`.
* Scenario: die in a level, the death screen shows both buttons (screenshot), Continue puts the player at the level
  start with the level-start HP and items; with no autosave file Continue is greyed out.
* Play-test: `tests/play/autosave.txt`: a level with a riddle gate and a monster, low HP, so the user can answer,
  die and continue.
