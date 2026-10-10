# A previous expedition's notes

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user.

## Idea

Another archaeologist came here before the player and did not come back. Their torn notebook pages lie around the
tomb: found as loot (in a chest or on a skeleton), read in the journal in a new section ("Expedition", a ribbon of its
own or inside Field notes: implementer's choice).

* ~15-20 pages over the campaign, in order of depth; each a few lines in a different handwriting (a second hand font,
  or the same font in another ink colour).
* Content: humour and hints. A story arc: confident at the start, doubts, the last page near lvl28-29 ("If you read
  this, take the left gate. I took the right.").
  Hints point at secrets: the [Stargate chamber](stargate-chamber.draft.md), a mimic, a weak spot of a boss.
* Stargate references welcome (the user loves the show): the missing archaeologist could be a nod to Daniel Jackson.
* Placement: a new object or a treasure with a "page" item; picked up on touch, no inventory slot. Save game keeps
  which pages were found.
* `levelcheck`: pages are optional, not on the exit path.

## Tests

* Unit: picking a page adds it to the journal, saved and loaded.
* Scenario with a screenshot of a journal page.
