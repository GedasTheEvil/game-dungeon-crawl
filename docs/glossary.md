# Glossary

The words used in the game, the code and the docs, and what they mean. One row per term, so a grep returns the whole
entry: `grep -i '| hold ' docs/glossary.md`.

* **In game:** the word the player sees, empty for dev-only terms.
* **Code:** where the term lives (type, constant, function, file). `tests/unit/docs_test.cpp` checks that every name
  in this column still exists in `src/`, and every path still exists.
* A term with more than one meaning (level, range, gap, threat) says so in its row.
* New term, or unsure of one: grep here first, add a row when you name a new thing.

Sections: [Map and levels](#map-and-levels), [Monsters](#monsters), [Combat](#combat), [Items](#items),
[Tooling](#tooling).

## Map and levels

| Term | In game | Meaning | Code |
|---|---|---|---|
| boss level | | a map level with a boss (every 5th: 5, 10, 15, ...); its boss gates open when the boss dies | `BOSS_LOCK` |
| boss room | | the part of a boss level where the boss and its minions fight; drafts also say "arena" | `BossRules` |
| campaign | | the 30 map levels in order, `levels/lvl1` .. `lvl30`; the last holds the ankh | `CAMPAIGN_LEVELS`, `campaignLevelFile` |
| cell | | one square of the level grid: a structure (wall, empty, water) and at most one object. Not a tile (see tile) | `Tile`, `LevelGrid` |
| column | | a vertical line of cells, counted from the left | `LevelGrid` |
| deep water | | a cell full of water: solid, nobody walks or swims into it. "Full water" in some drafts | `Structure::DeepWater`, `isSolidStructure` |
| flooded cell | | a cell with water in it: half water or deep water | `Structure` |
| floor | | a solid cell under an open one, what the player stands on. Also a storey of the map ("the upper floor") | `isSolidStructure` |
| foot of a ladder | | the lowest ladder cell, standing on a floor; the player can jump from it | `LADDER_BOTTOM` |
| gap | | (map) an open cell in a floor, one cell wide, jumped over. Not the gap between hitboxes (see Combat) | `src/world/level_check.h` |
| half water | | an open cell, half full: the player wades (half speed, no sprint, no jump), swimmers swim | `Structure::HalfWater`, `inHalfWater`, `WADE_SPEED_FACTOR` |
| ladder | Ladder | a ladder cell; the player climbs between ladder cells above each other | `Ladder`, `Dungeon::PlayerOnLadder` |
| ladder top | | the highest cell of a ladder shaft | `LADDER_TOP` |
| level | Level (save slots) | (map) one of the campaign's maps, "lvl5" in the docs. Not the player's level or a weapon's level (see Items) | `Dungeon::LevelNumber` |
| level grid | | every level is 40 x 47 cells, row 0 at the bottom | `LEVEL_WIDTH`, `LEVEL_HEIGHT` |
| pit | | a drop with no floor; a spike pit kills (the checker does not count it as a softlock) | `src/world/level_check.h` |
| riddle gate | | a gate that opens on the right answer to a riddle | `GateRiddle` |
| row | | a horizontal line of cells, row 0 at the bottom. Walkers follow the player along their row | `LevelGrid` |
| slain marker | | a monster's spawn cell remembers the monster killed there, so it does not come back after a load | `slainMonster`, `slainObject` |
| softlock | | a reachable cell from which the exit can no longer be reached; the checker warns | `LevelReport` |
| tile | | (render) a cell's size in the 3D world, 40 GL units; distances in docs are in tiles ("1.4 tiles"). Often used for "cell" | `TILE_SIZE` |
| wade | | walk through half water: half speed, no sprint, no jump ("too deep to jump") | `Dungeon::PlayerWading`, `Dungeon::JumpAllowed` |
| water slab | | (drawing) the thin rock a deep water cell sits on, and stands against at its sides, where it meets an open cell | `Dungeon::deepWaterBox`, `WATER_SLAB` |

## Monsters

| Term | In game | Meaning | Code |
|---|---|---|---|
| alerted | | a monster that has noticed the player; shows its health bar | `Monster::Alerted` |
| ambush | | rooted and still, a fake chest until the player comes near (the mimic) | `Locomotion::Ambush` |
| boss | | one strong monster per boss level; summons minions, its death opens the boss gates | `MonsterKind::isBoss`, `BossRules` |
| burrower | | dives into the floor and comes up elsewhere on its row (Apep) | `Locomotion::Burrow`, `Burrow` |
| charge | | a rush along its row through the player after a wind-up (Sobek) | `Charge`, `MonsterKind::charges` |
| climber | | (planned, [monster-climbers](plan/monster-climbers.draft.md)) a monster that follows the player up and down ladders | |
| coiled | | lies coiled until the player comes near, then rears up (the cobra) | `Locomotion::Coiled` |
| courage | | whether a monster sets foot on a trap: coward or reckless | `Courage` |
| coward | | afraid of traps: a walker stops at their edge, a walk-jumper leaps over them | `Courage::Coward` |
| creature | Creatures | the player's word for a monster; the journal's section | `Journal` |
| entombed | | lies in its coffin until the player comes near, then climbs out (the mummy) | `Locomotion::Entombed` |
| flee | | a coward that cannot reach the player runs along its row out of the bow's range ([coward-flee-ranged](plan/coward-flee-ranged.md)) | `Monster::flees`, `Monster::Flee`, `Dungeon::fleeFrom` |
| flyer | | flies along the row and crosses pits (bats) | `Locomotion::Fly`, `Flight` |
| hold | | (planned, [crocodile-hold-bite](plan/crocodile-hold-bite.draft.md)) the crocodile grabs the player in water and rolls | |
| kin | | a boss's common monster (the Anubis boss: the Anubis guard); a new ability for the kin goes to its boss too | `MonsterKind::kin` |
| kind | | a monster type: one row of stats, model and rules | `MonsterKind`, `monsterKind` |
| leap | | a walk-jumper's jump over a pit or a trap | `Leap` |
| locomotion | | how a monster gets around: stationary, ambush, walk, walk-jump, fly, ... | `Locomotion` |
| lurking | | ambush, entombed, submerged or coiled, before it wakes | `Monster::Lurk` |
| minion | | a monster a boss summons; gives less XP | `Monster::Minion`, `Dungeon::MinionXP` |
| reckless | | walks straight through traps and takes their damage | `Courage::Reckless` |
| spit | | a ranged venom glob, from afar along its row (the cobras); range in tiles | `SpitRules`, `Dungeon::Venom` |
| stationary | | rooted to its spawn cell, bites when the player is next to it (the plant) | `Locomotion::Stationary` |
| submerged | | lies under the water until the player comes near (the crocodile) | `Locomotion::Submerged` |
| swimmer | | faster in half water, floats at the surface (the crocodile) | `Wading::Swimmer` |
| threat | | (monster) one monster's weight in the checker's difficulty score. Not how dangerous it plays | `MonsterKind::threat`, `monsterThreat` |
| walker | | follows the player along its row, stops at pits | `Locomotion::Walk` |
| walk-jumper | | a walker that leaps pits and traps (giant rat, giant scarab) | `Locomotion::WalkJump` |

## Combat

| Term | In game | Meaning | Code |
|---|---|---|---|
| armour | Armor | taken off every hit on the player, at least 1 HP gets through; the game shows "Armor" | `PlayerStats::Armor` |
| bite reach | | how close a walker comes before it bites: 0.1 tiles between hitboxes (0.25 for the rooted) | `MONSTER_BITE_REACH`, `ROOTED_BITE_REACH` |
| damage mix | | a weapon's or a monster's damage split into blunt, slash and pierce, in percent | `DamageMix` |
| damage type | blunt, slash, pierce | one of the three kinds of damage | `DamageType` |
| gap | | (combat) the distance between two hitboxes' edges, in tiles. Not a gap in the floor (see Map) | `Monster::MeleeGap` |
| hitbox | | a body's box: half width from its model, used for reach and hits; F3 / `hitboxes on` shows them | `HalfWidth` |
| might | Might | added to every hit the player deals | `PlayerStats::Might` |
| player level | Level (stats) | the player's level from XP; raises HP, stamina, armour, might. Not the map level | `PlayerStats::CurrentLevel`, `levelXP` |
| poison tier | | weak, medium or strong poison; ticks damage until it runs out | `PoisonTier` |
| reach | | how far a melee weapon hits, from the player's box edge, in tiles | `weaponReach` |
| range | | how far a weapon or spit reaches, between the hitboxes; weapons in tenths of a tile, spit in tiles | `WeaponDef::range`, `SpitRules::range` |
| resistance | resists, tough | the share of a damage type a monster (or the player) takes: weak 200%, normal 100%, resists 50%, tough 25%. Also the resistance potion, against poison | `Resistances`, `resistedDamage` |
| weakness | weak | a damage type a monster takes double from; no boss has one | `WEAK` |

## Items

| Term | In game | Meaning | Code |
|---|---|---|---|
| amulet | Amulet | worn one at a time, gives its bonus while on; four tiers per type | `AmuletType`, `AmuletTier` |
| bag | Inventory | what the player carries: counts, weapon levels, the worn amulet | `ItemBag` |
| group | Weapons, Potions, Amulets, Rings | an inventory tab | `ItemGroup` |
| item kind | | one item: a weapon, a potion, an amulet tier; the save ids follow it | `ItemKind` |
| journal | Journal | the archaeologist's notebook: creatures, riddles, field notes | `Journal` |
| level | Lv | (weapon) a weapon's level, raised by copies found. Not the map level or the player's level | `ItemBag::Level`, `upgradeCost` |
| note | Field notes | a journal entry on a game rule, written the first time it matters | `FieldNote` |
| potion | | drunk from the inventory or a quick key | `PotionDef` |
| quick-drink | | the quick keys: drink the best fitting health or stamina potion | `Inventory::QuickDrink`, `quickPotion` |
| tier | Lesser, Minor, Grand | an amulet's strength: lesser, minor, normal, grand ("Lesser Amulet of Armor", "Amulet of Minor Armor") | `AmuletTier` |

## Tooling

| Term | In game | Meaning | Code |
|---|---|---|---|
| checker | | `./levelcheck`: can a level be finished, how big and hard is it; campaign levels pass with no warnings | `src/world/level_check.h` |
| draft plan | | an idea for later, `docs/plan/<slug>.draft.md` | `docs/plan` |
| effects stream | | the random stream for what only shows (a clip's start frame); never shifts the game's rolls | `GameRandom` |
| gameplay stream | | the random stream for what changes the game: loot, riddles, poison rolls | `GameRandom` |
| implemented plan | | a plan done but not confirmed by the user, `docs/plan/<slug>.md` | `docs/plan` |
| scenario | | a script in `tests/scenarios/` that drives the game and takes screenshots | `Scenario`, `docs/testing.md` |
| sim | | the sim library: the dungeon, the monsters and the player without GL, unit tested | `SimLinks`, `tests/unit/sim_world.h` |
| solved plan | | a plan the user confirmed, `docs/plan/solved/` | `docs/plan/solved` |
| threat | | (checker) the level's difficulty score, built from its monsters' threats. See threat under Monsters | `LevelReport::difficulty` |
| tick | | one fixed step of the game's update, 16 ms | `UPDATE_TICK_MS` |
| unit test | | a doctest case in `tests/unit/`, run by `make unit` | `tests/unit/main.cpp` |
