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
    example (a shaft 1-2 cells from where the mirror stands).
* Prefer optional rooms (treasure, a shortcut, a shrine) for the harder variants, the critical path for the
  introduction only, so a puzzle the player misreads never blocks the level.
* Dragging needs the flat corridor (bare, dry, unshaped cells on a wall, `dragGround`): keep the run from the stone to
  the plate free of water, sand, ladders, slopes and traps.
* Every touched level passes `bin/levelcheck content/levels/lvl*` with no warnings (the plate warnings: no weight
  reaches it, a plate without a gate, a gate without a plate). Known limit: a plate shares its colour's checker bit
  with keys and levers; don't reuse a colour of a key or lever of the same level for a plate.
* Edit the level files by hand (`content/levels/` are the source of truth, never regenerate). Glyph and file format:
  [level-format.md](../level-format.md).
* Inscriptions or a notebook page may hint at the introduction ("The silver gates answer to weight, not to keys").
  Check the hint texts against [inscription-hints-after-rebuild](solved/inscription-hints-after-rebuild.bug.md)'s
  lesson: a hint must name a thing that is really there.

## Play-test

* `bin/game --level N` for the introduction level: the stone, the plate and its gate read at a glance; dragging the
  stone on opens it.
* One later level per variant (listed here when implemented).
