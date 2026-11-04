# Sub-cells in the campaign

Status: draft 2026-11-02. Needs first: [subcell-checker](solved/subcell-checker.md). From the user: the
sub-cell drafts use a test level only; the campaign gets steps and crawl spaces here.

* Pick levels where steps make the route more interesting: a stepped hall, a stair up to a ledge, a crawl space as a
  shortcut or as the escape from a large monster, a duck under an archer's line.
* Edit by hand in the editor; never regenerate a level (AGENTS.md, Campaign levels).
* Every touched level passes `bin/levelcheck` with no warnings.
* Report which levels changed and give a `bin/game --level N` for each.
* Open point: how many levels; start with a few in one chapter and play-test before the rest.
