# The twelve hours of the Duat

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user.

## Idea

The campaign is Ra's night journey through the Duat, the underworld of the Book of Gates: twelve hours, each behind
a gate with its guardian. Give the 30 levels names and group them into hours, for story and a sense of progress.

* Each level gets a title, shown on level start (a short banner, `docs/ui.md` look) and in the map and save slots:
  e.g. "The Fourth Hour: the Gate of Sand".
* Grouping: 12 hours over 30 levels; the boss levels (5, 10, ..., 30) end an hour each. A split like 2-3 levels per
  hour is the implementer's choice; the twelfth hour is lvl30 with the ankh (Ra reborn at dawn).
* Guardians: the Book of Gates names a guardian serpent per gate (e.g. Saa-set, Aqebi, Teka-hra). The boss of a boss
  level is that hour's guardian in the banner text; non-boss hours just name the gate.
* Titles live in the level files (a new header field, `docs/levels.md`) so `levelcheck` can check every campaign level
  has one.
* Journal: a field note per new hour, in the archaeologist's voice ("The Seventh Hour. Apep waits here, says the
  wall. I wish the wall were wrong.").

## Tests

* Unit: every campaign level has a title (checker rule, no warnings).
* Scenario with a screenshot of the banner.
