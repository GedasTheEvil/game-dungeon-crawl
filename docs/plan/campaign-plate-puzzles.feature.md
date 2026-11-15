# Pressure plate and drag puzzles in the campaign levels

Status: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-14, size L, needs nothing first.

## Idea

[pressure-plate](solved/pressure-plate.feature.md) and [mirror-drag](solved/mirror-drag.feature.md) added the mechanism:
pressure plates (fixture 27, glyph `-`), silver plate gates (`Gate` with colours 6-9, glyphs `1`-`4`), stones to drag
(28, glyph `;`), and dragging of mirrors, emptied chests and bodies. Both plans left the campaign levels untouched and
promised a follow-up: this one. No campaign level has a plate, a plate gate, a stone or a mirror puzzle that needs
dragging yet.

## Plan

* Teach first, then combine. One plain introduction early (a plate a little after the first levers, lvl5-lvl9: a stone
  a few cells before a plate that needs 3, the gate in sight, so the player learns the player alone is too light), then
  a few later variants. One puzzle idea per level at most, and not in every level (about 6-10 levels in all):
  * a plate the player can hold (needs 2) whose gate closes as they step off: a stone or chest left on it instead;
  * two plates of one colour: the player on one, a stone on the other;
  * an emptied chest dragged onto a plate (the chest's loot taken first);
  * a monster's body as the weight (a heavy one, a giant or a boss, near a plate; bodies are lost on a load until
    save-live-monsters, so never the only way through: a stone too, or the puzzle off the critical path);
  * the mirror zone (lvl33-lvl40 have mirrors): a mirror that must be dragged under a light shaft, as mirror-drag's
    example (a shaft 1-2 cells from where the mirror stands);
  * a lured monster as the weight, for a player who would rather not kill it: a slow walker (pick by `MonsterKind::speed`,
    `src/level/monster_kinds.cpp`) follows the player onto the plate, the gate goes up, and the player runs for it before the monster steps off and
    the gate comes down again. A timing puzzle: the gate a few cells from the plate, off the critical path or with a
    stone too (a monster's path is not reliable). Not a flyer: a living bat, a flying or jumping monster, or a
    hidden (burrowed) one weighs nothing on a plate (`Dungeon::WeightOn`); only its body counts.
* Prefer optional rooms (treasure, a shortcut, a shrine) for the harder variants, the critical path for the
  introduction only, so a puzzle the player misreads never blocks the level.
* Dragging needs the flat corridor (bare, dry, unshaped cells on a wall, `dragGround`): keep the run from the stone to
  the plate free of water, sand, ladders, slopes and traps. The stone stands on the plate's own row: the player drags
  only along a row, and `checkPlates` counts only the plate's flat floor run left and right.
* The checker's weight is optimistic. It counts every stone, mirror, chest, rock fall (its rock lands as a stone) and
  monster spawn (its body, once slain) on the plate's run, wherever the gate stands. Check by hand that the weight
  is on the player's side of the gate: a stone behind it soft-locks the introduction on the critical path.
* Stray weight: a monster that walks onto a plate opens its gates (a rat weighs 1), and a gate does not come down on
  anyone in its cell. Keep walkers off plate rooms unless the puzzle wants them (the lure variant). A rock fall on the
  plate's row gives a stone too: place one only on purpose.
* Every touched level passes `bin/levelcheck content/levels/lvl*` with no warnings (the plate warnings: no weight
  reaches it, a plate without a gate, a gate without a plate). Known limit: a plate shares its colour's checker bit
  with keys and levers; don't reuse a colour of a key or lever of the same level for a plate.
* Edit the level files by hand (`content/levels/` are the source of truth, never regenerate). Glyph and file format:
  [level-format.md](../level-format.md).
* Inscriptions or a notebook page may hint at the introduction ("The silver gates answer to weight, not to keys").
  Check the hint texts against [inscription-hints-after-rebuild](solved/inscription-hints-after-rebuild.bug.md)'s
  lesson: a hint must name a thing that is really there. The field note `note.pressure-plates`
  (`content/texts/journal.txt`) already explains plates: check that its words fit the introduction level.
* A scenario test (`tests/scenarios/`) for the introduction level: drag the stone onto the plate, the gate goes up.
  It catches a later level edit that breaks the puzzle.

## Implementation

| Level | Where | Variant | Scenario |
|---|---|---|---|
| lvl8 | row 26, cols 24-30, the critical path | introduction: a stone, the red plate (needs 3), its gate in sight | `plate_intro` |
| lvl12 | row 4, cols 26-34, the treasure branch | the blue plate needs 2: the player opens it but cannot pass; the stone past it pulled back | `plate_hold` |
| lvl15 | row 41, cols 7-25, the upper branch | lure: a mummy (col 25) onto the blue plate (col 24); or its body. A new drop (col 12) leads back to row 38: no soft-lock behind a gate that came down | `plate_lure` |
| lvl18 | row 2, the boss arena | the High Priest's body (weight 4) on the blue plate (col 25); a new alcove (cols 18-19) by the boss gate | `plate_boss_body` |
| lvl24 | row 6, cols 27-38, an empty dead end | two green plates (needs 3 and 2): the rock fall's stone and an emptied chest (placed as a chest); the dart plate moved to col 27 | `plate_two_plates` |
| lvl39 | row 18, cols 23-28 | a mirror 2 cells off a new light shaft's beam: dragged under it, turned, it lights the red disc; a minor amulet of fire guard behind the gate | `plate_mirror_drag` |

* Colours: none shares a colour with a key or lever of its level. Walkers that would stray onto a plate moved off
  (lvl12's giant scarab, lvl15's giant scarab, lvl39's giant scorpion off the mirror's run).
* The field note `note.pressure-plates` now fits the introduction (the player alone too light; the stone holds it).
* Checker: a treasure placed as another container than a chest weighs nothing on a plate; the mirror the player
  stands at may be dragged along its drag run before it is turned (`sunlitCellsDragging`, `dragRun`), for the sun
  discs and the path. Path scripts (`levelcheck --script`, `make paths`) drag a stone onto a plate on the path and the
  checker's dragged mirror under its beam (unit tests in `subcell_mismatches_test.cpp`).
* `tools/run_scenarios.sh`: a batch of one script ran as a batch and counted as failed (the game prints batch lines
  for two or more); it runs alone now.
* Not done: no new inscription hint (the field note explains plates; inscriptions need the hieroglyph sheet); a
  late-game (lvl46-54) puzzle, as six levels cover every variant.

## Play-test

`bin/game --play tests/play/campaign-plate-puzzles.txt` (lvl8's introduction; the comment lists the others):

* lvl8: off the ladder, the stone, the plate and the silver gate read at a glance; standing on the plate does
  nothing; pushing the stone onto it opens the gate.
* lvl12 (`level 12 at 26 4`): on the plate the gate rises, off it it comes down before you reach it; pull the stone
  back onto the plate.
* lvl15 (`level 15 at 27 41`): wait past the plate for the mummy; while it stands on the plate, slip through; the drop
  at col 12 leads back down.
* lvl18 (`level 18 at 31 2`), lvl24 (`level 24 at 28 6`), lvl39 (`level 39 at 25 18`): the boss's body, the two
  plates (the fallen rock and the emptied chest), the mirror dragged under the shaft and turned.
