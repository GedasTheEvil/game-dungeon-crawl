# Pressure plate and drag puzzles in the campaign levels

Status: draft 2026-11-14, size L, needs nothing first.

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

## Play-test

* `bin/game --level N` for the introduction level: the stone, the plate and its gate read at a glance; dragging the
  stone on opens it.
* One later level per variant (listed here when implemented).
