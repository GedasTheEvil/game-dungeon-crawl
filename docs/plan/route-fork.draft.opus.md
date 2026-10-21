# Route fork: left or right

Status: draft 2026-10-19, from the user's review. Needs first: [expedition-notes](expedition-notes.md)
(the page that warns before the fork).

## Idea

One level splits into two one-way routes that both reach the exit. The player picks one, warned before the choice.
It pays off the expedition's last page: "If you read this, take the left gate. I took the right."

* **Never a trap:** both routes reach the exit hall. The choice is cost and reward, not survival. The map stays as
  it is.
* **One-way:** the fork is two drop shafts (or two one-way gates) side by side in one hall. Once down, no way back
  up to the other route; the routes join again in the exit hall.
* **Left (the safe one):** longer, a crocodile wade or a monster hall of the level's set, little loot.
* **Right (the one that killed them):** shorter and harder: a trap gauntlet (rock falls, dart plates, rhythm spikes),
  then a treasure tomb and the earlier archaeologist's remains: a skeleton with their gear (a good weapon or
  amulet of the depth) and their journal's end. A brave player takes it for the loot.

## Known before the choice

The player must know what the two routes are before they drop. Both hints, on the way to the fork:

* **The expedition page** (always): the last page lies on the path to the fork, on a skeleton or a floor spot, never
  in a chest (no mimic, no trapped chest, no chance to miss it). Its text names the two gates and what the writer
  found behind the right one. This page is the exception to expedition-notes' "pages are optional, not on the exit
  path".
* **An inscription at the fork** (for readers): on the back wall between the shafts, a warning in the priests' words
  ("To the left, the long road of the living. To the right, the short road of the dead."). Without the
  [Words of Thoth](solved/skill-hieroglyphs.md) skill it is copied as unread, as today.

## Placement

One fork in the campaign: the level of the expedition's last page, today lvl28 or lvl29 (the necropolis; lvl33 or
lvl34 with [desert-chapter](solved/desert-chapter.md)'s renumbering). Implementer's choice: a second, smaller
fork earlier, with an earlier page.

## levelcheck

Must prove the fork is fair: from the fork, each route alone reaches the exit (keys, levers and gates on each
route, not one key on the other side). A new warning: a one-way drop whose landing cannot reach the exit. Check
whether today's checker already finds that softlock; if so, this needs only a test on the fork level.

## Tests

* Unit: the checker passes the fork level and warns about a copy where the right route's gate key lies on the left.
* Scenario with screenshots: the fork hall, the page, the inscription.
