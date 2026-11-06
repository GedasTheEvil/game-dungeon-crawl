# Native terrain: monsters stronger in their own ground

Status: draft 2026-11-06, size L, Won't do 2026-11-06: superseded by [terrain-affinity](../terrain-affinity.feature.draft.opus.md) (hostile terrains added), needs nothing first.

## Idea

A monster can have native terrains. Standing in one, it moves faster, attacks more often and hits harder. Most
monsters have none. Each terrain has one set of multipliers, the same for every monster native to it, bosses
included: no per-monster numbers. Balance (lowering a boss's base stats, if it turns out too hard) is not part of
this plan: separate follow-up drafts after the play-test, maybe not needed at all. It replaces the worm-only draft ([worm-sand-frenzy](worm-sand-frenzy.feature.md)) and the
swimmers' per-kind `waterSpeed`. More terrains can be added later as one row in the table.

## Plan

* **Terrains** (`enum class NativeTerrain`, a cell test each, read on the monster's own cell every tick, so it
  drops back at once when it leaves):
  * Water: half water (`inHalfWater`, deep water where a swimmer goes).
  * Sand: sand and quicksand alike (`inSand`). Quicksand is sand here, not a terrain of its own.
  * Darkness: a cell that is not lit (not `Dungeon::Lit`, no sunbeam). A doused torch makes darkness.
  * Web: a cell with a floor web (the Mother of Webs' webs).
  * Fire: a lit cell (`Dungeon::Lit`: a torch, a brazier, an oil lamp within `LIGHT_SHUN_CELLS`) or a fire vent's
    cell. The opposite of darkness, on purpose.
* **The table** (gameplay_config.h, one row per terrain: speed, attack rate, damage). Starting values, tune by
  play-test:

  | terrain | speed | attack interval | damage |
  |---|---|---|---|
  | water | x2 | x0.8 | x1.5 |
  | sand | x2 | x0.8 | x1.5 |
  | darkness | x1.5 | x0.8 | x1.25 |
  | web | x2 | x0.8 | x1.25 |
  | fire | x1.5 | x0.8 | x1.5 |

  The attack interval is `attackMs` times the factor (x0.8 = 25% more blows).
* **Who is native** (`MonsterKind::native`, a bit set: a monster can have several; in two at once, each factor is
  the highest of the two, they do not stack):
  * water: crocodile, Sobek, hippo, Seth, cobra, giant cobra, Apep (today's `Wading::Swimmer`s);
  * sand: worm, Rerek, scorpion, giant scorpion, the scorpion queen (the user's call; not the egg cluster: it does not move or bite);
  * darkness: bat, giant bat, Bat of Shezmu, camel spider, tiny spider, Mother of Webs;
  * web: camel spider, tiny spider, Mother of Webs;
  * fire: baboon of the Lake of Fire, Babi.
* **Bosses**: a boss has its kin's native terrains (the boss rule, AGENTS.md). Extend the boss-rule check in
  monster_kinds.cpp (~1417, now for `sandBurrow`) to fail a boss without them. The same multipliers as everyone.
* **Water today**: `waterSpeed` per kind (cobra x1.25 ... hippo x3) goes away. `Wading` keeps only how it moves in
  water: `Slowed` (non-natives, `WADE_SPEED_FACTOR`), `Unaffected`, `Swimmer` (floats, `swimLift`). A swimmer is
  native to water; check the two match. The one water speed changes every swimmer (cobras and Apep faster, the
  hippo, Seth, the crocodile and Sobek slower in water): accepted, no retune here.
* **The tell**: the only visible sign is the walk (and swim) clip playing slightly faster in a native terrain
  (e.g. 1.25x, one constant for all). No trail, no glow, no text in the HUD.
* **The level checker**: one threat factor per terrain, derived from its row (speed and damage per second), for a
  monster spawned in its native terrain. It replaces `SWIMMER_IN_WATER_THREAT_FACTOR` (level_check.cpp ~1319).
  Darkness and fire depend on light, which the checker knows from the level's flames. Run
  `bin/levelcheck content/levels/lvl*`; every level it now flags gets fixed (AGENTS.md, Campaign levels).
* **Texts**: the journal notes that the change makes wrong (`monster.worm`: "Slow as sand..."), the `description`
  of each native kind where it helps (the worm: quick in the sand), a glossary row "native terrain", and the
  glossary rows "sand burrower", "light shunner", "web" where they need it.
* **Tests**: unit tests: a native monster in and out of its terrain (speed, attack interval, damage), quicksand
  same as sand, two terrains at once take the highest factor, a boss gets its kin's terrain, the checker factor.

## Steps

The plan can land in steps, each one play-testable on its own:

1. The table, `native`, sand and water (`waterSpeed` gone), the clip rate, the checker.
2. Darkness and web.
3. Fire.

## Balance notes

* In sand the player walks at 3/4 with no sprint: a native at x2 is about 2.7x the player's speed there. No running
  away in sand. The same holds in half water (the player wades at `WADE_SPEED_FACTOR`).
* A x1.25 attack rate and x1.5 damage is about x1.9 damage per second.
* The bosses fight in their terrain almost all the time (Rerek's hall is sand, Sobek's and Seth's have water): their
  fights get much harder. Any nerf is a separate follow-up draft, after the play-test.
* Darkness: the player lighting braziers and torches now weakens bats and spiders, and the Mother of Webs dousing
  torches makes her hall stronger for her. Wanted.

## Open questions

* The starting numbers in the table and the clip rate.
* Fire: is "lit cell" too wide (every torch in a level)? A narrower choice: only braziers and fire vents.

## Play-test

* 
