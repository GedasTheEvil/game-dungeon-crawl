# Terrain affinity: monsters stronger in native terrain, weaker in hostile terrain

Status: solved 2026-11-06 (play-tested by the user). Was: implemented 2026-11-06, waiting for the user's play-test. Was: draft 2026-11-06, size L, needs nothing first.

## Idea

A monster can have native terrains and hostile terrains (its terrain affinity). In a native one it moves faster,
attacks more often and hits harder; in a hostile one it is slower, attacks less often and hits softer, and it avoids
the terrain when it can. Most monsters have neither. Each terrain has one native row and one hostile row of
multipliers, the same for every monster, bosses included: no per-monster numbers. Balance (lowering a boss's base
stats, if it turns out too hard) is not part of this plan: separate follow-up drafts after the play-test, maybe not
needed at all. It replaces the native-only draft ([native-terrain](../wont-do/native-terrain.feature.md)), the
worm-only draft ([worm-sand-frenzy](../wont-do/worm-sand-frenzy.feature.md)) and the swimmers' per-kind `waterSpeed`.
More terrains can be added later as one row pair in the table.

## Plan

* **Terrains** (`enum class Terrain`, a cell test each, read on the monster's own cell every tick, so the effect
  ends at once when it leaves). One cell can be in several terrains (a brazier's cell is fire and light):
  * Water: half water (`inHalfWater`, deep water where a swimmer goes).
  * Sand: sand and quicksand alike (`inSand`). Quicksand is sand here, not a terrain of its own.
  * Light: a lit cell (`Dungeon::Lit`: a wall torch, a brazier, an oil lamp within `LIGHT_SHUN_CELLS`) or a sunbeam
    (`Dungeon::Sunlit`).
  * Darkness: every cell that is not light. A doused torch makes darkness.
  * Fire: a real flame's own cell only, not its light: a brazier's or an oil lamp's cell, a fire vent's cell, a
    blaze. Wall torches and sunbeams are light, not fire. A fire vent counts only while its jet burns (for the
    multipliers); for the avoidance it counts always (monsters know the trap).
  * Web: a cell with a floor web (`Dungeon::WebAt`, the Mother of Webs' webs).
* **The table** (gameplay_config.h, a native and a hostile row per terrain: speed, attack interval, damage).
  Starting values, tune by play-test:

  | terrain | native speed | native interval | native damage | hostile speed | hostile interval | hostile damage |
  |---|---|---|---|---|---|---|
  | water | x2 | x0.8 | x1.5 | x0.6 | x1.25 | x0.75 |
  | sand | x2 | x0.8 | x1.5 | x0.6 | x1.25 | x0.75 |
  | darkness | x1.5 | x0.8 | x1.25 | x0.6 | x1.25 | x0.75 |
  | light | x1.5 | x0.8 | x1.25 | x0.6 | x1.25 | x0.75 |
  | web | x2 | x0.8 | x1.25 | x0.6 | x1.25 | x0.75 |
  | fire | x1.5 | x0.8 | x1.5 | x0.6 | x1.25 | x0.75 |

  The attack interval is `attackMs` times the factor (x0.8 = 25% more blows, x1.25 = 20% fewer). Darkness hostile and
  light native have no user yet: rows for later.
* **Several terrains at once** (the user's rules):
  * Native in several: per factor, the strongest bonus wins (fastest speed, shortest interval, highest damage). No
    stacking.
  * Hostile in several: per factor, the strongest penalty wins (slowest, longest interval, lowest damage). No
    stacking.
  * Native and hostile at once (e.g. a spider in a web in a lit cell): they cancel out, the monster has its normal
    stats; for the avoidance the cell is neutral.
* **Who** (`MonsterKind::native` and `MonsterKind::hostile`, bit sets):

  | monster | native | hostile |
  |---|---|---|
  | crocodile, Sobek, cobra, giant cobra, Apep (today's `Wading::Swimmer`s) | water | |
  | hippo, Seth (swimmers too) | water | sand |
  | worm, Rerek | sand | water |
  | scorpion, giant scorpion, scorpion queen (not the egg cluster: it does not move or bite) | sand | water |
  | bat, giant bat, Bat of Shezmu | darkness | fire |
  | camel spider, tiny spider, Mother of Webs | darkness, web | light, fire |
  | mummy, High Priest of Anubis | | fire |
  | baboon of the Lake of Fire, Babi | fire | water |
  | living ushabti (clay) | | water |

  Checks (unit test or a static check in monster_kinds.cpp): no kind is native and hostile to the same terrain; none
  is native to both light and darkness, nor hostile to both.
* **Avoidance** (generalises the light shunner's `Dungeon::wayOutOfLight`). The preference per cell: native, then
  neutral, then hostile.
  * Not chasing the player (idle, wandering): a hostile cell is a wall to it; it never walks into one by itself.
    One already in a hostile cell (spawned there, the terrain changed under it) walks out to the nearest better cell.
  * Chasing: it still follows the player into hostile terrain (no refuge for the player), but prefers a way and an
    attack cell outside it when one is about as short.
  * Hurt or fighting while it stands in hostile terrain: it steps to the nearest better cell within 2 cells that
    still lets it reach the player, and fights from there. None: it fights where it stands, with the penalties. The
    2 cells and the reach keep the player from pushing it around.
  * The light shunner (`shunsLight`, full HP only) stays as is, the stricter form on top: lit cells are a wall to it
    even while chasing. Merging the two is a later cleanup, not this plan.
* **Bosses**: a boss has its kin's native and hostile terrains (the boss rule, AGENTS.md). Extend the boss-rule check
  in monster_kinds.cpp (~1417, now for `sandBurrow` and `shunsLight`) to fail a boss without them. The same
  multipliers as everyone.
* **Water today**: `waterSpeed` per kind (cobra x1.25 ... hippo x3) goes away. `Wading` keeps only how it moves in
  water: `Slowed` (non-natives, `WADE_SPEED_FACTOR`), `Unaffected`, `Swimmer` (floats, `swimLift`). A swimmer is
  native to water; check the two match. The one water speed changes every swimmer (cobras and Apep faster, the
  hippo, Seth, the crocodile and Sobek slower in water): accepted, no retune here. A `Slowed` wader hostile to water
  takes the lower of `WADE_SPEED_FACTOR` and the hostile speed, not their product.
* **The tell**: the walk (and swim) clip plays faster in a native terrain (1.1x) and slower in a hostile one
  (0.9x, the user's call), one constant each for all. No trail, no glow, no text in the HUD. A journal line
  (`CreatureMove::AvoidTerrain`, like `ShunLight`) the first time the player sees a monster step out of hostile
  terrain.
* **The level checker**:
  * Native: one threat factor per terrain, derived from its row (speed and damage per second), for a monster spawned
    in its native terrain. It replaces `SWIMMER_IN_WATER_THREAT_FACTOR` (level_check.cpp ~1319). Darkness, light and
    fire depend on the level's flames, which the checker knows.
  * Hostile: no lower threat (the estimate stays on the safe side). A warning for a monster spawned in a hostile cell
    (a fire vent's always counts): a placement mistake.
  * Run `bin/levelcheck content/levels/lvl*`; every level it now flags gets fixed (AGENTS.md, Campaign levels).
* **Texts**: the journal notes that the change makes wrong (`monster.worm`: "Slow as sand..."), a line in the
  `description` of each kind with an affinity where it helps (the worm: quick in the sand, hates water; the mummy:
  keeps away from flames), glossary rows "terrain affinity", "native terrain", "hostile terrain", and the rows "sand
  burrower", "light shunner", "lit cell", "web" where they need it.
* **Tests**: unit tests: a native monster in and out of its terrain (speed, attack interval, damage), the same for a
  hostile one, quicksand same as sand, two native (two hostile) terrains take the strongest factor, native and
  hostile at once cancel, the wade and hostile water take the lower speed, a boss gets its kin's terrains, the
  avoidance (an idle monster does not step into a hostile cell, a hurt one steps out, a chasing one follows), a torch
  is light but not fire, the checker factor and warning.

## Steps

The plan can land in steps, each one play-testable on its own:

1. The table, `native` and `hostile`, the combining rules, water and sand (`waterSpeed` gone), the avoidance, the
   clip rate, the checker.
2. Darkness, light and web.
3. Fire.

## Balance notes

* In sand the player walks at 3/4 with no sprint: a native at x2 is about 2.7x the player's speed there. No running
  away in sand. The same holds in half water (the player wades at `WADE_SPEED_FACTOR`).
* A x1.25 attack rate and x1.5 damage is about x1.9 damage per second; the hostile row (x0.8 rate, x0.75 damage) is
  about x0.6.
* The bosses fight in their terrain almost all the time (Rerek's hall is sand, Sobek's and Seth's have water): their
  fights get much harder. Any nerf is a separate follow-up draft, after the play-test.
* Darkness and light: the player lighting braziers and torches now weakens bats and spiders, and the Mother of Webs
  dousing torches makes her hall stronger for her. Wanted.
* Hostile terrain is a tool for the player (fight a worm by the water, a mummy by a brazier), not a refuge: the
  wading player is slow too, and a chasing monster still follows.

## Open questions

* The starting numbers in the table.
* The avoidance's "about as short" detour for a chasing monster (e.g. at most 2 cells longer).

## Play-test

`bin/game --play tests/play/terrain-affinity.txt` (the scorpion), then the campaign levels to taste: `bin/game --level 10`
(camel spiders in the dark), `--level 4` (bats), a scorpion or worm level with water.

* Stand in the sand by the scorpion: it bites quick and hard, its legs run fast. Walk west onto the dry floor: it
  follows at its usual pace.
* Walk on into the water: it follows you in, but slowly, its legs slow; its bites are fewer and softer.
* Light a brazier or stand by a torch near a camel spider or a bat: it is slow and weak there; in the dark it is fast.
* A baboon backing off from you stops at the water's edge instead of stepping in.
* The journal notes "Hates the water..." (or the light, the fire) the first time you see one walk out of it.

## Notes

* The checker: a native in its terrain scores x(1 + `TERRAIN_THREAT_WEIGHT` x its gain); it knows only the placed
  flames (torches, braziers, lamps), every other cell counts as dark. 31 campaign spawns in hostile cells moved 1-6
  cells along their row (23 camel spiders by placed torches, 7 baboons and a worm in water); levelgen puts no monster
  in a hostile cell and no torch or brazier by a light-hater.
* Fighting in a hostile cell, it steps to a better cell up to 2 away only if it still bites the player from there:
  with the short bite reach (0.1 tile) that is a small step at a terrain's edge. Chasing, it takes the row's one way;
  climbers' paths are as before. Flyers get the factors, not the avoidance.
* Damage factors act on every hit (bite, grab, death roll, sting, charge, shot); a shot keeps its thrower's factor.
  Charges keep their own speed.

