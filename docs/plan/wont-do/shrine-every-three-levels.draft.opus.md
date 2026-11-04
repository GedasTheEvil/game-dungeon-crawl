# A Shrine of Isis every three levels

Status: draft 2026-11-04, needs nothing first. From the user.

## Idea (from the user)

At least one Shrine of Isis in every three levels, boss levels aside.

## Today

* 13 shrines ([shrine-of-isis](../solved/shrine-of-isis.md), placed prop `DECOR_SHRINE_ISIS` = 27 in the level's `decor`
  section): lvl9, 14, 18, 24, 29, 32, 38, 41, 45, 47, 50, 52, 54. That was the old rule, "rare, about one level in four, on
  the levels with the most curse sources" ([levels](../../levels.md), Shrines of Isis).
* Boss levels: 4, 9, 13, 18, 22, 27, 32, 37, 41, 45, 54. Six of them hold a shrine before the boss (9, 18, 32, 41,
  45, 54).
* Curses come from the first level on: rats, bats, giant scarabs, Khepri, worms, mummies, Anubis guards, cobras,
  trapped and cursed containers. The longest stretch with no shrine is lvl1-8.

## Rule

* Count only the non-boss levels. No three of them in a row lack a shrine. A shrine on a boss level counts too and
  stays where it is: boss fights are where curses hurt most.
* Boss levels need none.

## Plan

* Add a shrine on lvl3, 7, 12, 17, 21, 28, 35 and 44 (8 new, 21 in all). This is the fewest that meet the rule with
  the shrines already placed. Moving one a level up or down is fine as long as the rule still holds. Several of
  them sit just before a boss level (3 before Khepri, 12 before the Mother of Webs, 17 before the High Priest, 21
  before the scorpion queen, 44 before Babi): put those near the exit.
* Placement as before: a floor cell four or more cells from a monster, at a wing's end or off the path where the hall
  allows, with a preference for parts of the level that have curse sources. The placed-prop rules apply
  ([editor readme](../../../tools/editor/readme.md#decor-and-decal-layers)).
* lvl3 is the first shrine the player meets. Make it easy to find, so the player learns about it before Khepri
  (greater clumsiness).
* Hand-edit the level files. Do not regenerate them (AGENTS.md, Campaign levels).
* Guard the rule so it cannot break unnoticed, for example a unit test that reads `content/levels/lvl*` and checks the
  rule over the campaign, or a campaign-wide warning in `bin/levelcheck` when it is given every level. The
  implementer chooses.
* Docs: [levels](../../levels.md), Shrines of Isis (the new list and rule), and the glossary's Shrine of Isis row ("Rare:
  about one level in four" becomes the new rule).
* `bin/levelcheck content/levels/lvl*` shows no warnings. Screenshot scenario of a few of the new shrines in place.
