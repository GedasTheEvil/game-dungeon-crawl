# Sub-cells in the campaign

Status: implemented 2026-11-04, waiting for the user's play-test (`bin/game --level N`, any level 1 to 54). Was: draft
2026-11-02, needed [subcell-checker](solved/subcell-checker.md) first. From the user: the sub-cell drafts use a test
level only; the campaign gets steps and crawl spaces here.

Implementer's choices:

* Every campaign level got 2 to 4 features (from the user: all levels, the play-test at the end): a stair of three
  quarter steps in place of a ledge's ladder (the ladder's foot cell turned to rock), a dais or raised block on the
  path (a step, half steps, a step: plain walkers and mummies stop at it), steps either side of a death-trap pit
  (raised take-offs, in halls of 2 rows or more), a crawl space to a treasure or on the route that the large monsters
  near it do not fit (giant rats and bats, giant scarabs, camel spiders, crocodiles, giant scorpions, mummies,
  Anubis), and cover against shooters: a step or half step to crouch behind (Anubis's bolt, the cobras' spit stop at
  a solid sub-cell), a crawl space between cobras. Boss arenas and the stargate room are untouched.
* Objects moved off a shaped cell or a crawl space's end (rock falls, spikes, spawns, torches) where needed; each
  level passes `bin/levelcheck` with no warnings and its checker path plays in the game (`make paths`, 54/54).
* Shapes set with `tools/level/shape.py` (the editor's brushes by name, the shaped glyph kept in step).
* Left out, the game does not allow them: stepping stones in half water (no jump while wading: one way only), steps
  by a pit in a 1-row hall (no head room for the jump), a crawl space starting in a ladder's cell (down climbs
  there). Where the checker and the game disagree: [subcell-checker-mismatches](subcell-checker-mismatches.draft.opus.md).
* Not checked in a game run: that a crouch behind a cover step dodges the bolt or the spit (it follows from the shot
  code: aimed at the crouched chest, stopped by a solid sub-cell).
* Tests: the v2/v3 conversion tests drop the shapes first (old formats have none); decor goldens updated (no random
  decor in a shaped cell); the lvl4 walkthrough (`world_rules_test.cpp`) goes over the new dais.

Per level (row, cols):

* Tomb entrance: lvl1 a quarter step (35, 24), a platform the rat stops at (38, 25-26); lvl2 pit steps (36, 15/17),
  a crawl on the exit run (45, 31-32); lvl3 a dais before the pit (44, 24-26), a crawl on the route (31, 25-26); lvl4 a
  dais on the entrance run (43, 17-19), a crawl to a treasure (34, 29-30).
* Bat galleries: lvl5 a stair to the ledge (33, 12-14), a dais (37, 15-18), a crawl before the exit (21, 17-18);
  lvl6 a crawl to a treasure (41, 28-29), a raised block (30, 12-15); lvl7 a crawl on the exit run (45, 11-12), pit
  take-offs (28, 15-19); lvl8 a stair to the ledge (44, 7-9), a crawl to a treasure (41, 25-26), a dais (33, 13-15);
  lvl9 a stair to the ledge (41, 31-33), a raised block (19, 21-23).
* Web halls: lvl10 crawls (5, 6-7; 36, 36-37), a raised block (28, 15-17), a stair (39, 18-20); lvl11 crawls (15,
  13-14; 34, 26-27), pit steps (30, 25-29), a raised block (22, 11-13); lvl12 crawls (45, 27-28; 38, 31-32), a step
  (17, 17-18), a dais (7, 12-14); lvl13 crawls (15, 4-5; 7, 26-27), pit steps (26, 18-22).
* Priests' quarter: lvl14 cover steps (5, 23/27), a crawl to the blue key (41, 33-34); lvl15 cover steps (43,
  19/24), pit steps (7, 18/20), a crawl (12, 11); lvl16 a crawl (31, 17-18), cover steps (24, 12/20), pit steps (14,
  16-19); lvl17 a stair (11, 30-32), cover steps (20, 21/23; 31, 25; 1, 35); lvl18 a crawl (45, 2-3), cover steps
  (16, 17/22), a dais (11, 32-34), pit steps (24, 22-25), a stair (24, 30-32).
* Scorpion nest: lvl19 a stair (15, 5-7), a crawl (37, 33-34), pit steps (19, 29/31); lvl20 a stair (33, 27-29), a
  crawl to the page (23, 36-37), pit steps (44, 13/15); lvl21 a stair (41, 7-9), pit steps (27, 20/22), a crawl (21,
  36-37); lvl22 a stair (43, 6-8), pit steps (43, 29/31), a raised block (15, 13-15).
* Serpent temple: lvl23 a crawl between cobras (42, 22-23), a dais (5, 12-14), pit steps (9, 9/11); lvl24 a stair (42,
  32-34), a crawl (9, 28-29), a dais (14, 18-20), pit steps (31, 28/30); lvl25 pit steps (44, 18/20), crawls (41,
  18-19; 25, 22-23); lvl26 a dais (41, 10-12), a crawl between cobras (27, 25-26), pit steps (22, 24/26); lvl27 a step
  (41, 6), a dais (29, 12-14), a crawl (5, 15-16).
* Cisterns of Sobek: lvl28 a dais as cover (29, 19-21), a tunnel (7, 27-28, roofed); lvl29 a dais (36, 18-21), a crawl
  (41, 30-31); lvl30 a cover block (33, 22-24), a crawl (16, 31); lvl31 a raised run (9, 25-27), a crawl (19, 23-24);
  lvl32 pit steps (43, 5/7), a dais (10, 19-22).
* Seth's desert: lvl33 a stair (14, 31-33), a crawl (31, 36-37), a block (34, 8-9); lvl34 a crawl (4, 6-7), a block
  (41, 18-20), a stone in half water (16, 18); lvl35 a dais (44, 13-15), crawls (2, 31-32; 32, 36-37); lvl36 a crawl in
  sand (7, 35-36), pit steps (25, 15/17), a dais (31, 11-13); lvl37 a crawl (16, 7-8), raised blocks (25, 18-20; 11,
  11-12).
* Land of Sokar: lvl38 crawls (45, 15-16; 4, 3-4), a dais (34, 26-28); lvl39 a crawl (45, 25-26), daises (36, 34-37;
  13, 30-32); lvl40 a crawl (39, 25-26), pit steps (16, 13/15), a dais (5, 15-17); lvl41 a crawl (45, 12-13), pit steps
  (17, 18/20), a stair (20, 7-9).
* Lake of fire: lvl42 a dais (43, 26-28), a crawl as cover (30, 25-26), pit steps (2, 15/17); lvl43 a dais (30,
  8-10), pit steps (8, 15/17), a crawl between baboons (11, 22-23); lvl44 a dais as cover (36, 13-15), a crawl (27,
  6-7), pit steps (3, 12/14); lvl45 a dais as cover (41, 29-31), a crawl to a treasure (38, 27-28), a dais (23, 23-25).
* Necropolis: lvl46 a crawl (45, 30-31), a dais (20, 12-14), pit steps (28, 14/16); lvl47 a stair (38, 7-9), a crawl
  (3, 27), pit steps (9, 21/23); lvl48 a stair (10, 7-9), a crawl (36, 17-18), a dais (14, 15-18); lvl49 a crawl (36,
  21-22), a dais (15, 14-17), a raised block (28, 23-25); lvl50 pit steps (44, 13/15), a crawl (22, 17-18), a cover
  block between two Anubis (30, 20-23).
* Hall of judgment: lvl51 a crawl (6, 13-14), a cover block (37, 21-24), a step (12, 27); lvl52 a cover block (43,
  29-32), a crawl (17, 15-16); lvl53 a crawl (45, 13-14), a raised block (28, 22-24), a dais (6, 15-18); lvl54 a crawl
  between Anubis and a mummy (10, 22-23), a raised block (24, 25-27).

* Pick levels where steps make the route more interesting: a stepped hall, a stair up to a ledge, a crawl space as a
  shortcut or as the escape from a large monster, a duck under an archer's line.
* Edit by hand in the editor; never regenerate a level (AGENTS.md, Campaign levels).
* Every touched level passes `bin/levelcheck` with no warnings.
* Report which levels changed and give a `bin/game --level N` for each.
* Open point: how many levels; start with a few in one chapter and play-test before the rest.
