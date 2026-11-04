# Sub-cells in the campaign

Status: draft 2026-11-02. On hold 2026-11-04: the first batch (the bat galleries, lvl5 to lvl7) is in, waiting for the
user's play-test before the rest. Needed [subcell-checker](solved/subcell-checker.md) first. From the user: the
sub-cell drafts use a test level only; the campaign gets steps and crawl spaces here.

First batch (2026-11-04), the bat galleries: plain walkers (rats, worms) stop at a quarter step, giant rats hop onto
it, giant bats and crocodiles do not fit a crawl space. Shapes set with `tools/level/shape.py` (the editor's brushes by
name, the shaped glyph kept in step); each level passes `bin/levelcheck` and its path plays in the game.

* lvl5 (`bin/game --level 5`): the ledge's ladder (col 11, rows 33-34) is now a stair of three quarter steps (cols
  12-14, row 33; col 11 rock, its torch moved onto the ledge); a dais in the lever hall on the path (row 37, cols
  15-18: a step, two half steps, a step); a crawl space before the exit (row 21, cols 17-18) the giant rat there
  cannot follow through.
* lvl6 (`bin/game --level 6`): the treasure at the end of row 41 behind a crawl space (cols 28-29) the giant rat
  cannot pass (the rock fall moved from col 27 to 26, off the crawl's end); a raised block on the way to the blue key
  (row 30, cols 12-15).
* lvl7 (`bin/game --level 7`): a crawl space on the exit run (row 45, cols 11-12; the rock fall moved from col 13 to
  14), the giant bat before the exit cannot follow; raised take-offs either side of the death-trap pit (row 28: steps
  at cols 15 and 19, half steps at 16 and 18).

Next, after the play-test: the other chapters; a duck under an archer's line where the archers are (Anubis, cobras).

* Pick levels where steps make the route more interesting: a stepped hall, a stair up to a ledge, a crawl space as a
  shortcut or as the escape from a large monster, a duck under an archer's line.
* Edit by hand in the editor; never regenerate a level (AGENTS.md, Campaign levels).
* Every touched level passes `bin/levelcheck` with no warnings.
* Report which levels changed and give a `bin/game --level N` for each.
* Open point: how many levels; start with a few in one chapter and play-test before the rest.
