# Denser levels: use the whole grid

Status: draft 2026-10-06, from the user: every level is 40 x 47 cells, but 40-60% of it is solid wall, even lvl15.
Most levels use only the top 15-20 rows. The mazes could be larger on each level.

* No format change: the grid stays 40 x 47 (`LEVEL_WIDTH`, `LEVEL_HEIGHT`). Fill it: more halls, more floors, side
  branches, loops (two ways to the same place), vertical shafts across the whole height.
* The new levels 16-30 of the [longer campaign](longer-campaign.draft.md) are drawn this way from the start.
* Levels 1-15 get the same later, one at a time, keeping their character (the first levels stay short: a level 1 the
  size of level 30 is too much). Each must still pass `./levelcheck` with no warnings.
* Watch: the spawn window (monsters spawn when they come into view), `MAX_MONSTERS` (32 live slots), the draft map's
  size on screen, the load time.

Open:

* How big the early levels should grow (a size curve by level).
