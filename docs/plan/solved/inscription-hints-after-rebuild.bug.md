# Inscription hints and a notebook page that the campaign rebuild made wrong

Status: solved 2026-11-13 (play-tested by the user). Was: implemented 2026-11-13, waiting for the user's play-test. Was: draft 2026-11-13, size M, needs nothing first.

From the lore audit of 2026-11-13. The texts of `content/inscriptions/campaign.txt` were written for the old
35-level campaign; several carry a hint about the spot they stood on ("the green key is inside the pillar", "a hole
in the floor right here"). The rebuild ([denser-levels](denser-levels.md)) placed them again on other levels
where the spot is gone, so the text now points at something that is not there. The `# ---- level N` comments above
them still name the old levels. One expedition page miscounts the forks the same way.

Coordinates are `COL ROW` as `bin/levelcheck --map` prints them (row 0 at the bottom); the wall fixture lines in the
level files give the text id (`COL ROW 15 ID 0`).

## Repro

1. `bin/game --level 41`, read the inscription at (30, 39), id 16 "The djed of Osiris".
2. The text says "What is green rests within the pillar, in the house that has no door but the way around", the note
   "The green key is inside the pillar?". The green key (colour 3) lies at (2, 14), at the open west end of a hall,
   not in a pillar.

## Expected

Every campaign inscription that points at something on its level points at something that is there. A hint whose
spot is gone is rewritten to fit the level it stands on now, or to say nothing about the spot (keep the real source
lines and the notebook voice, [texts.md](../../texts.md)).

## Actual

Checked against the level files and `bin/levelcheck --map` (2026-11-13):

| id | title | now on | the text says | the level has |
|---|---|---|---|---|
| 8 | The Book of Gates | lvl38 (6, 2) | T: "The carnelian door opens first, then the turquoise, and the golden door last of all." N: "Red, then green, then gold." | no red at all: keys blue (2) and green (3), a gold (4) sun disc; gates blue, green, gold, green |
| 16 | The djed of Osiris | lvl41 (30, 39) | the green key is inside the pillar, reached round it | the green key at (2, 14), at a hall's open west end |
| 27 | The treasury in the pillar | lvl49 (21, 38) | T: "Go down where the floor is open"; N: "There is a hole in the floor right here." | solid rock under (21, 38) and (20, 38); lvl49 has no teleporter or hidden chamber |
| 30 | The overseer of works | lvl50 (12, 22) | T: "Kha, beneath this floor ... Go down, if you dare; the way up is by the ladder of the workmen." | solid rock under it; the nearest way down is the death trap at (15, 21) |
| 20 | The cistern treasury | lvl30 (17, 33) | "the gate of light at the end of this passage"; N: "A teleporter to a treasure room?" | no teleporter on row 33; the level's pair is at (13, 27) and (36, 29) (check whether either leads to a treasure room) |
| 22 | The instruction of Amenemhat | lvl25 (32, 44) | N: "Paranoid, but this hall has darts." | no dart plate on row 44 (the level's five are elsewhere) |
| 14 | The spitting wall | lvl24 (17, 19) | N: "A pressure plate ahead, darts in the wall." | no dart plate on row 19 (lvl24 has 8 elsewhere); soft, "ahead" may pass |
| 9 | A workmen's note | lvl30 (15, 27) | "The ceiling is cracked here, sand runs from it." | no rock fall on row 27 (4 elsewhere on lvl30); soft |

The other campaign texts with a hint fit their level (id 2 lvl29, 5 lvl9, 10 lvl15, 12, 17 lvl26, 23 lvl31,
31 lvl54, 48 lvl38, 49 and 50 lvl37).

The `# ---- level N: ...` comments in `campaign.txt` (lines 5 to 418, and line 439 "levels 6 to 35") name the old
levels: e.g. line 192 "level 19: Apep, the boss of level 20" (Apep is on lvl27), line 418 "level 35: the Anubis
boss" (lvl54), line 89 "level 12: red key, green lever, gold key" (id 8 is on lvl38 now).

`content/texts/expedition.txt:114`, page 22 (lvl42): "A second fork ahead: the right is shorter." It tells of the
lvl43 fork, the third: the first is lvl12 (page 8 itself, "A hall with two shafts side by side. I took the short
one", line 43, lies on it), the second lvl39 (page 19, line 99, "A fork ahead"). [levels.md](../../levels.md#campaign-order)
says so too ("lvl43: The third route fork").

## Fix

* Per row above: rewrite the T: hint line(s) and the N: line to fit the level the text stands on now (look at its
  map), or drop the spot-specific part. The real-source lines stay (the djed lines of id 16 are from Osiris lore;
  only "What is green rests within the pillar, in the house that has no door but the way around" is the hint).
  Alternative for one text: a small level edit that makes the hint true, only if trivial; never regenerate a level
  (AGENTS.md). Keep each text's `boon:`.
* Rewrite each `# ---- level N` comment to the level(s) the text is on now (`grep -n "15 ID 0" content/levels/lvl*`
  in the wall section, or the inscription list of [levels.md](../../levels.md#campaign-order)), or to a zone heading
  like `rebuild.txt` has.
* Page 22: "Another fork ahead" or "A third fork ahead".
* The game saves a copy of each read text: old saves keep the old wording (fine, AGENTS.md).

## Fixed when

* A test that names the plan fails before the fix and passes after it: e.g. a unit test that loads
  `campaign.txt` and checks id 8's text has no "carnelian" and id 16's no "within the pillar", and that
  `expedition.txt` page 22 has no "second fork".
* `bin/levelcheck content/levels/lvl*` still passes with no warnings.

## Play-test

* `bin/game --level 41`: read the inscription by the ladder at (30, 39); its words fit this level.
* `bin/game --level 38`, `--level 49`, `--level 50`: the Book of Gates text, the pillar text and Kha's text no longer
  promise a red door, a hole or a tomb that is not there.

## Implemented

Rewrote the hints of ids 8, 9, 14, 16, 20, 22, 27, 30 in `campaign.txt` (id 8 now lapis, turquoise, gold; the rest drop
the spot, keep the real source lines and the `boon:`). The `# ---- level N` comments are now `# ---- lvlN` above each
id, from the wall fixture lines. Page 22 says "A third fork ahead". Test: `inscription-hints-after-rebuild` in
`tests/unit/inscriptions_test.cpp`.
