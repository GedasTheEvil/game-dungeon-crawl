# Level content gaps

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: level edits. Every level must still pass `bin/levelcheck` with no warnings. Edit the level files by hand; never
regenerate (AGENTS.md, Campaign levels).

## Problems

* Inscription id 5, "Shezmu, lord of the press" (`content/inscriptions/campaign.txt`), is never placed. It is the
  hint for the Bat of Shezmu fight (it heals by biting). lvl9, the Shezmu level, has "The wheel of the day" instead.
  Every other id 1-137 is placed.
* Sunlight mirrors: only 4 mirrors and 4 sun discs, one each on lvl33, 34, 36, 37
  ([sunlight-mirrors](solved/sunlight-mirrors.md)). All 31 light shafts beam down; left, up and right never appear.
  The shafts on lvl35 and 38-41 are only daylight.
* Egg clusters only on lvl22 (2).

## Plan

* Place id 5 on lvl9 before the boss room (or swap it with "The wheel of the day" if that one fits elsewhere).
* Two to four more mirror puzzles in the desert zone and later levels, some with sideways shafts, a few with two
  mirrors. Keep the early ones simple.
* Egg clusters in one or two more spider / camel-spider levels, if the zone fits.
