# Glossary

The words used in the game, the code and the docs, and what they mean. One row per term, so a grep returns the whole
entry: `grep -i '| hold ' docs/glossary.md`.

* **In game:** the word the player sees, empty for dev-only terms.
* **Code:** where the term lives (type, constant, function, file). `tests/unit/docs_test.cpp` checks that every name
  in this column still exists in `src/`, and every path still exists.
* A term with more than one meaning (level, range, gap, threat) says so in its row.
* New term, or unsure of one: grep here first, add a row when you name a new thing.

Sections: [Map and levels](#map-and-levels), [Monsters](#monsters), [Combat](#combat), [Items](#items),
[Runs](#runs), [Tooling](#tooling).

## Map and levels

| Term | In game | Meaning | Code |
|---|---|---|---|
| boss level | | a map level with a boss (every 5th: 5, 10, 15, ...); its boss gates open when the boss dies | `BOSS_LOCK` |
| boss room | | the part of a boss level where the boss and its minions fight; drafts also say "arena" | `BossRules` |
| campaign | | the 30 map levels in order, `content/levels/lvl1` .. `lvl30`; the last holds the ankh | `CAMPAIGN_LEVELS`, `campaignLevelFile` |
| cell | | one square of the level grid: a structure (wall, empty, water), at most one fixture and at most one object. Not a tile (see tile) | `Tile`, `LevelGrid` |
| column | | a vertical line of cells, counted from the left | `LevelGrid` |
| deep water | | a cell full of water: solid, nobody walks or swims into it. "Full water" in some drafts | `Structure::DeepWater`, `isSolidStructure` |
| fixture | | a thing on a cell's back wall, in the wall layer: a lever, an inscription, wall spikes. Drafts also say "wall thing" | `Fixture`, `Tile::fixture`, `isWallType` |
| flooded cell | | a cell with water in it: half water or deep water | `Structure` |
| floor | | a solid cell under an open one, what the player stands on. Also a storey of the map ("the upper floor") | `isSolidStructure` |
| foot of a ladder | | the lowest ladder cell, standing on a floor; the player can jump from it | `LADDER_BOTTOM` |
| gap | | (map) an open cell in a floor, one cell wide, jumped over. Not the gap between hitboxes (see Combat) | `src/world/level_check.h` |
| half water | | an open cell, half full: the player wades (half speed, no sprint, no jump), swimmers swim | `Structure::HalfWater`, `inHalfWater`, `WADE_SPEED_FACTOR` |
| ladder | Ladder | a ladder cell; the player climbs between ladder cells above each other | `Ladder`, `Dungeon::PlayerOnLadder` |
| ladder top | | the highest cell of a ladder shaft | `LADDER_TOP` |
| inscription | Inscriptions | hieroglyphs on a cell's back wall the player reads; the text goes into the journal ([inscriptions](inscriptions.md)). Drafts also say "cartouche" | `Inscription`, `InscriptionBook`, `JournalInscription` |
| glint | | the gold band sweeping now and then over an inscription not read yet; dimmer over a copy not translated ([interact-prompt](plan/interact-prompt.md)) | `Dungeon::InscriptionGlint`, `glintQuad` |
| interact verb | "[E] Open" | what the interact key does in the player's cell (Open, Pull, Read, Copy, Light arrows, Enter, Answer, Take), shown by the interact prompt at the bottom right ([interact-prompt](plan/interact-prompt.md)) | `Interaction`, `Dungeon::InteractVerb`, `InteractPrompt` |
| boon | Gift | a permanent stat an inscription gives when it is translated: Might, Armor, max health % or max stamina % ([inscription-boons](plan/solved/inscription-boons.md)) | `Boon`, `InscriptionText::boon`, `JournalInscription::boon`, `PlayerStats::Grant` |
| level | Level (save slots) | (map) one of the campaign's maps, "lvl5" in the docs. Not the player's level or a weapon's level (see Items) | `Dungeon::LevelNumber` |
| level grid | | every level is 40 x 47 cells, row 0 at the bottom | `LEVEL_WIDTH`, `LEVEL_HEIGHT` |
| pit | | a drop with no floor; a spike pit kills (the checker does not count it as a softlock) | `src/world/level_check.h` |
| riddle gate | | a gate that opens on the right answer to a riddle | `GateRiddle` |
| row | | a horizontal line of cells, row 0 at the bottom. Walkers follow the player along their row | `LevelGrid` |
| slain marker | | a monster's spawn cell remembers the monster killed there, so it does not come back after a load | `slainMonster`, `slainObject` |
| softlock | | a reachable cell from which the exit can no longer be reached; the checker warns | `LevelReport` |
| tile | | (render) a cell's size in the 3D world, 40 GL units; distances in docs are in tiles ("1.4 tiles"). Often used for "cell" | `TILE_SIZE` |
| wade | | walk through half water: half speed, no sprint, no jump ("too deep to jump") | `Dungeon::PlayerWading`, `Dungeon::JumpAllowed` |
| wall layer | | the cell's layer between the structure and the object: its fixture ([wall-layer](plan/solved/wall-layer.md)). Not the structure `Wall` | `Tile::fixture`, `setFixture`, `setThing` |
| wall spikes | | a rhythm trap ([rhythm-traps](plan/solved/rhythm-traps.md)): poison spikes out of a cell's back wall on a fixed cycle; out, they block like a gate and stab whoever stands in the cell. Drafts also say "rhythm trap" | `WallSpikes`, `spikeStateAt`, `Dungeon::updateWallSpikes` |
| water slab | | (drawing) the thin rock a deep water cell sits on, and stands against at its sides, where it meets an open cell | `Dungeon::deepWaterBox`, `WATER_SLAB` |
| water prop | | ([water-decorations](plan/solved/water-decorations.md)) a prop scattered in a free half water cell: papyrus reeds (at the bank), weed (under the surface), lotus leaves, a blue lotus. No function; the reeds and weed sway, the leaves bob | `isWaterProp`, `DECOR_WATER_FIRST` |

## Monsters

| Term | In game | Meaning | Code |
|---|---|---|---|
| alerted | | a monster that has noticed the player; shows its health bar | `Monster::Alerted` |
| ambush | | rooted and still, a fake chest until the player comes near (the mimic) | `Locomotion::Ambush` |
| boss | | one strong monster per boss level; summons minions, its death opens the boss gates | `MonsterKind::isBoss`, `BossRules` |
| burrower | | dives into the floor and comes up elsewhere on its row (Apep) | `Locomotion::Burrow`, `Burrow` |
| charge | | a rush along its row through the player after a wind-up (Sobek) | `Charge`, `MonsterKind::charges` |
| climber | | a monster that follows the player up and down ladders once it has seen them ([monster-climbers](plan/solved/monster-climbers.md)) | `MonsterKind::climbs`, `ClimbMap`, `Dungeon::followPath` |
| coiled | | lies coiled until the player comes near, then rears up (the cobra) | `Locomotion::Coiled` |
| courage | | whether a monster sets foot on a trap: coward or reckless | `Courage` |
| coward | | afraid of traps: a walker stops at their edge, a walk-jumper leaps over them | `Courage::Coward` |
| creature | Creatures | the player's word for a monster; the journal's section | `Journal` |
| entombed | | lies in its coffin until the player comes near, then climbs out (the mummy) | `Locomotion::Entombed` |
| floating | | rooted in half water, a flower on the surface until the player comes near, then rears up and bites (the blue lotus, [water-lotus](plan/solved/water-lotus.md)) | `Locomotion::Floating`, `Monster::floatLift` |
| flee | | a coward that cannot reach the player runs along its row out of the bow's range ([coward-flee-ranged](plan/solved/coward-flee-ranged.md)) | `Monster::flees`, `Monster::Flee`, `Dungeon::fleeFrom` |
| flyer | | flies along the row and crosses pits (bats); follows the player up or down to another row through open cells ([bats-high-ceiling](plan/solved/bats-high-ceiling.md)) | `Locomotion::Fly`, `Flight`, `FlightGoal`, `Dungeon::flightGoal` |
| hold | | (planned, [crocodile-hold-bite](plan/crocodile-hold-bite.draft.opus.md)) the crocodile grabs the player in water and rolls | |
| kin | | a boss's common monster (the Anubis boss: the Anubis guard, in the game "Anubis living statue"); a new ability for the kin goes to its boss too | `MonsterKind::kin` |
| Egyptian names | | the game shows Egyptian names, the code and the editor keep the old ids and labels: guard = Anubis living statue, Anubis boss = Anubis, boss scarab = Khepri, vampire bat = Bat of Shezmu, mimic = Chest of Set, worm = Sand worm, plant = Thorn acacia; amulets are named by their object (Jackal Tooth, Eye of Horus), labels by their effect | `MonsterKind::name`, `MonsterKind::label` |
| kind | | a monster type: one row of stats, model and rules | `MonsterKind`, `monsterKind` |
| leap | | a walk-jumper's jump over a pit or a trap | `Leap` |
| locomotion | | how a monster gets around: stationary, ambush, walk, walk-jump, fly, ... | `Locomotion` |
| lurking | | ambush, entombed, submerged, coiled or floating, before it wakes | `Monster::Lurk` |
| minion | | a monster a boss summons; gives less XP | `Monster::Minion`, `Dungeon::MinionXP` |
| reckless | | walks straight through traps and takes their damage | `Courage::Reckless` |
| ranged attack | | a monster's shot from afar along its row: the cobras' spit, the Anubis's bolt; range in tiles; clip and sound suffix `_att_ranged` | `RangedRules`, `MonsterKind::ranged`, `Dungeon::MonsterShot`, `ModelState::AttackRanged` |
| spit | | the cobras' ranged attack: a poisoned venom glob | `RangedRules::poison` |
| fire ramp | | the colours of a fire over a particle's life, one per `FireStyle` (flame, venom green) | `FireRamp`, `Fire::VENOM_RAMP` |
| glob with flames | | a spat glob drawn as a cloud of fire sprites with smaller clouds trailing, no ball; ramp per projectile (venom: green). Without flames: a plain ball | `drawGlobWithFlames` |
| stamina drain | | a bite that takes a share of the player's max stamina (the blue lotus's sedative, 25%); the journal's "drowsy" note | `MonsterKind::staminaDrainPct`, `CreatureMove::Sedate` |
| stationary | | rooted to its spawn cell, bites when the player is next to it (the plant) | `Locomotion::Stationary` |
| submerged | | lies under the water until the player comes near (the crocodile) | `Locomotion::Submerged` |
| swimmer | | faster in half water, floats at the surface (the crocodile) | `Wading::Swimmer` |
| threat | | (monster) one monster's weight in the checker's difficulty score. Not how dangerous it plays | `MonsterKind::threat`, `monsterThreat` |
| walker | | follows the player along its row, stops at pits | `Locomotion::Walk` |
| walk-jumper | | a walker that leaps pits and traps (giant rat, giant scarab) | `Locomotion::WalkJump` |

## Combat

| Term | In game | Meaning | Code |
|---|---|---|---|
| armour | Armor | taken off every hit on the player (its non-fire shares), at least 1 HP gets through; the game shows "Armor" | `PlayerStats::Armor` |
| burning | You are on fire!, catches fire | a fire hit sets its target burning: 3 s at a fifth of the hit's fire share a second, armour or not; half water puts it out ([fire-damage](plan/solved/fire-damage.md)) | `Burning`, `BURN_MS`, `Fire::body` |
| bite reach | | how close a walker comes before it bites: 0.1 tiles between hitboxes (0.25 for the rooted) | `MONSTER_BITE_REACH`, `ROOTED_BITE_REACH` |
| damage mix | | a weapon's or a monster's damage split into blunt, slash, pierce and fire, in percent | `DamageMix` |
| damage type | blunt, slash, pierce, fire (player reads: smash, cut, stab, fire) | one of the four kinds of damage | `DamageType` |
| fire | fire | the fourth damage type: goes through armour, sets the target burning; only the fire guard cuts it on the player | `DamageType::Fire`, `fireDamage` |
| fire arrow | Dipped 5 arrows in the brazier's fire | interact by a brazier with a bow: the next 5 arrows deal 60% of their damage as fire | `Player::fireArrows`, `Dungeon::lightArrows`, `FIRE_ARROW_SHOTS` |
| gap | | (combat) the distance between two hitboxes' edges, in tiles. Not a gap in the floor (see Map) | `Monster::MeleeGap` |
| hitbox | | a body's box: half width from its model, used for reach and hits; F3 / `hitboxes on` shows them | `HalfWidth` |
| might | Might | added to every hit the player deals | `PlayerStats::Might` |
| player level | Level (stats) | the player's level from XP; raises HP, stamina, armour, might. Not the map level | `PlayerStats::CurrentLevel`, `levelXP` |
| poison tier | | weak, medium or strong poison; ticks damage until it runs out; medium and strong scale with the victim's max HP above a floor | `PoisonTier`, `POISON_TIERS` |
| touch poison | sap | a melee hit on the plant may poison the player; ranged hits do not | `TouchPoison` |
| reach | | how far a melee weapon hits, from the player's box edge, in tiles | `weaponReach` |
| range | | how far a weapon or ranged attack reaches, between the hitboxes; weapons in tenths of a tile, ranged attacks in tiles | `WeaponDef::range`, `RangedRules::range` |
| resistance | resists, tough | the share of a damage type a monster (or the player) takes: weak 200%, normal 100%, resists 50%, tough 25%. Also the resistance potion, against poison | `Resistances`, `resistedDamage` |
| weakness | weak; Weakness | (combat) a damage type a monster takes double from; no boss has one. Also a curse (see curse names) | `WEAK`, `Curse::Weakness` |
| curse | A curse! | a timed debuff a monster's hit lays on the player: 2 minutes, several at once, the same again restarts it; no cure, no resistance, ends on death. Purple-framed HUD tiles | `Curses`, `Curse`, `CURSE_MS` |
| curse names | Weakness, Clumsiness, Vulnerability, Fatigue, Disease, Slowness, Palsy, Insanity | weakness -50% Might (greater -125%); clumsiness -20% hit damage (greater -50%); vulnerability -60% armour; fatigue stamina to 20%, no regeneration; disease -30% max HP (greater -70%); slowness half walk, sprint 1.25 tiles/s; palsy no ranged shot; insanity screens, quick drinks and weapon keys locked, club in hand | `CURSES`, `curseDef` |
| greater curse | Greater weakness | a curse at its stronger grade: its own entry on a monster, with a lower chance | `CurseGrade` |
| curse entry | | a monster's chance to lay one curse with a melee hit or with its spit / bolt; at most two each, the strongest first, one curse a hit | `CurseEntry`, `MonsterKind::meleeCurses`, `MonsterKind::rangedCurses` |

## Items

| Term | In game | Meaning | Code |
|---|---|---|---|
| amulet | Amulet | worn one at a time, gives its bonus while on; four tiers per type | `AmuletType`, `AmuletTier` |
| bag | Inventory | what the player carries: counts, weapon levels, the worn amulet | `ItemBag` |
| fire guard | Sun Disc | the amulet that cuts fire damage, 20 / 40 / 60 / 80% (never more: fire always hurts) | `AmuletType::FireWard` |
| group | Weapons, Potions, Amulets, Rings | an inventory tab | `ItemGroup` |
| heka | Heka | the point a slain boss gives (once per level), spent on a skill on the Heka screen (K); also the screen's name | `Heka`, `SkillsScreen` |
| item kind | | one item: a weapon, a potion, an amulet tier; the save ids follow it | `ItemKind` |
| journal | Journal | the archaeologist's notebook: creatures, riddles, field notes, inscriptions | `Journal` |
| level | Lv | (weapon) a weapon's level, raised by copies found. Not the map level or the player's level | `ItemBag::Level`, `upgradeCost` |
| note | Field notes | a journal entry on a game rule, written the first time it matters | `FieldNote` |
| potion | | drunk from the inventory or a quick key | `PotionDef` |
| quick-drink | | the quick keys: drink the best fitting health or stamina potion | `Inventory::QuickDrink`, `quickPotion` |
| sealed | Sealed | a skill shown on the Heka screen whose rules are not in the game yet: not learnable | `SkillDef::sealed` |
| dodge | Dodged | the Dodge skill: 1 monster attack in 10 (`DODGE_PERCENT`) misses the player: no damage, no poison | `Player::Dodges` |
| skill | | a permanent ability bought with one heka; no respec | `SkillId`, `SKILLS` |
| tier | Lesser, Minor, Grand | an amulet's strength: lesser, minor, normal, grand ("Lesser Amulet of Armor", "Amulet of Minor Armor") | `AmuletTier` |

## Runs

| Term | In game | Meaning | Code |
|---|---|---|---|
| campaign run | | a run begun with New Game on level 1; only its win goes on the leaderboard (scenario `campaign`) | `RunStats::campaign` |
| exploration score | Explored | a run's inscriptions read x10 + treasures picked up x10 + riddles answered x30 | `explorationScore` |
| Hall of Records | Hall of Records | the main menu's leaderboard screen: rank, name, score in the chosen order; a row opens the run's details | `RecordsView` |
| leaderboard | Hall of Records | the won campaign runs, one record each, in `saves/leaderboard.txt` (never in a save game) | `Leaderboard`, `RunRecord` |
| play time | Time | a run's time in play: menus, the UI screens and the win do not count; per level on the details | `RunStats::PlayMs` |
| placeholder row | Spirit of Exploration | the empty board's one row, never written to the file | `placeholderRecord` |
| run statistics | | what a run did since New Game (kills, potions, play time, exploration, damage, times poisoned and cursed), kept in the save | `RunStats` |
| win entry | | the name field over the won game; Enter records the run, Esc skips | `WinEntry` |

## Tooling

| Term | In game | Meaning | Code |
|---|---|---|---|
| checker | | `bin/levelcheck`: can a level be finished, how big and hard is it; campaign levels pass with no warnings | `src/world/level_check.h` |
| draft plan | | an idea for later, `docs/plan/<slug>.draft.<model>.md` (`sonet` simple, `opus` complex) | `docs/plan` |
| effects stream | | the random stream for what only shows (a clip's start frame); never shifts the game's rolls | `GameRandom` |
| gameplay stream | | the random stream for what changes the game: loot, riddles, poison rolls | `GameRandom` |
| implemented plan | | a plan done but not confirmed by the user, `docs/plan/<slug>.md` | `docs/plan` |
| scenario | | a script in `tests/scenarios/` that drives the game and takes screenshots | `Scenario`, `docs/testing.md` |
| play mode | | `--level` / `--play`: a normal hand-played game started on a level or a scenario's setup commands, menu skipped | `Scenario::loadPlay`, `docs/testing.md` |
| setup commands | | the leading scenario commands that only set the game up (`level`, `give`, `god`, ...); `--play` runs them at once | `playSetup`, `docs/testing.md` |
| sim | | the sim library: the dungeon, the monsters and the player without GL, unit tested | `SimLinks`, `tests/unit/sim_world.h` |
| solved plan | | a plan the user confirmed, `docs/plan/solved/` | `docs/plan/solved` |
| threat | | (checker) the level's difficulty score, built from its monsters' threats. See threat under Monsters | `LevelReport::difficulty` |
| tick | | one fixed step of the game's update, 16 ms | `UPDATE_TICK_MS` |
| unit test | | a doctest case in `tests/unit/`, run by `make unit` | `tests/unit/main.cpp` |
