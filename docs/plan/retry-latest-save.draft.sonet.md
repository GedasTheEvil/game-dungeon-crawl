# Retry from the latest save

Status: draft 2026-10-29, from the user's idea. Needs nothing first. Works with
[quick-save](quick-save.draft.sonet.md) in either order.

## Idea

Today a death (and the main menu's Continue, plan autosave) offers only the autosave. A player who saved by hand
after the last autosave loses that progress. Offer the newest save instead, by file time, whatever its kind.

## Rules

* **Candidates:** the autosave, the six slots (`SaveSlots::FileName`) and the quick save once it exists (plan
  quick-save). Newest by the file's modification time wins. Missing files don't count.
* **Death screen** (`src/ui/death_screen.cpp`): the first button loads that save. Its label names it:
  "Retry from Autosave", "Retry from Slot 3", "Retry from Quick Save". Greyed out with no save at all ("No save
  yet" in the hint line).
* **Main menu Continue** (the user's call, 2026-10-29): loads the same newest save (autosave, quick save or a slot);
  shown when any save exists (now: only with an autosave). The Load Game list stays as is.
* One helper picks the save (e.g. `GameState::LatestSave()` returning path + kind/slot, or none), used by both
  screens. Save loading goes through `GameState::LoadSave` as now; the autosave path keeps its scenario override
  (`AutosavePath`).

## Tests

* Unit test for the pick: autosave older than a slot save → the slot; no files → none; quick save newest → it.
* Scenario or update `tests/play/autosave.txt` users: die after a manual save made later than the autosave, the
  death screen names the slot.
