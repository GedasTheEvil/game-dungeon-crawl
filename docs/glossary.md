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
| structure | | a cell's bottom layer: wall, empty, half water or deep water. Not the structure of the code | `Structure`, `Tile::structure` |
| object | | a cell's middle layer, at most one: a gate, a key, a trap, a ladder, a treasure, a monster spawn, the ankh; a type, an attribute and a value | `DungeonTileType`, `hasObject`, `setObject` |
| cut-scene | Cut-scenes (Options) | the camera leaves the player to show a far gate a lever or a boss's death opens, then comes back; the world stands still, a key skips it | `CutShot`, `Dungeon::CutScenePlaying` |
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
| copy | Copy | an inscription read without the Words of Thoth skill: the glyphs go into the journal untranslated, and learning the skill translates every copy ([skill-hieroglyphs](plan/solved/skill-hieroglyphs.md)) | `Interaction::Copy`, `JournalInscription::translated`, `Journal::TranslateInscriptions` |
| glint | | the gold band sweeping now and then over an inscription not read yet; dimmer over a copy not translated ([interact-prompt](plan/solved/interact-prompt.md)) | `Dungeon::InscriptionGlint`, `glintQuad` |
| interact verb | "[E] Open" | what the interact key does in the player's cell (Open, Pull, Read, Copy, Light arrows, Enter, Answer, Take), shown by the interact prompt at the bottom right ([interact-prompt](plan/solved/interact-prompt.md)) | `Interaction`, `Dungeon::InteractVerb`, `InteractPrompt` |
| boon | Gift | a permanent stat an inscription gives when it is translated: Might, Armor, max health % or max stamina % ([inscription-boons](plan/solved/inscription-boons.md)) | `Boon`, `InscriptionText::boon`, `JournalInscription::boon`, `PlayerStats::Grant` |
| level | Level (save slots) | (map) one of the campaign's maps, "lvl5" in the docs. Not the player's level or a weapon's level (see Items) | `Dungeon::LevelNumber` |
| level grid | | every level is 40 x 47 cells, row 0 at the bottom | `LEVEL_WIDTH`, `LEVEL_HEIGHT` |
| pit | | a drop with no floor; a spike pit kills (the checker does not count it as a softlock) | `src/world/level_check.h` |
| riddle gate | | a gate that opens on the right answer to a riddle | `GateRiddle` |
| gate | Gate | a portcullis: opens with the key of its lock colour or a lever of that colour; a boss gate opens when the boss dies. The sphinx gates (`Door`) are the entrance, the exit, riddle gates and teleporters | `Gate`, `Door`, `GateType` |
| lock colour | | ties keys, gates and levers together: four colours, and the boss lock for boss gates | `LockColour`, `LOCK_COLOURS`, `BOSS_LOCK` |
| key | Key | lies on the floor, picked up on touch; opens the gates of its colour | `Key` |
| lever | Lever | a fixture; pulled, it opens every gate of its colour | `Lever` |
| exit | | the sphinx gate to the next level (Enter); its plasma is red. The entrance is where the player starts | `GateExit`, `GateEntrance` |
| teleporter | | a sphinx gate that sends the player to the other one with the same pair id (Enter) | `GateTeleport`, `Dungeon::Teleport` |
| gate travel | | the fade into the gate's plasma colour and back (0.3 s each way) on the exit or a teleporter; the world stands still | `GateTravel`, `GATE_TRAVEL_FADE_MS`, `Dungeon::Travelling` |
| ankh | | the goal on the last level: Take wins the game | `Ankh` |
| name gate | The Name Gate | a teleporter sealed by the level's royal name: the riddle scroll asks for it, the right name opens it for good ([secret-name-gates](plan/solved/secret-name-gates.md)). Its plasma breathes until then | `NAME_SEAL`, `isNameGate`, `Dungeon::SpeakName` |
| name cartouche | A royal name | an inscription that holds the level's royal name for its name gate, not a text; its picture is one of 16 sign groups that do not spell the name | `NAME_CARTOUCHE_ID`, `nameCartouchePicture`, `Dungeon::NameCartouchePicture` |
| secret room | | the pocket of a level only the name gate leads to: a chest, a lore inscription, now and then a guardian; never needed for the exit | `checkNameGates` |
| row | | a horizontal line of cells, row 0 at the bottom. Walkers follow the player along their row | `LevelGrid` |
| slain marker | | a monster's spawn cell remembers the monster killed there, so it does not come back after a load | `slainMonster`, `slainObject` |
| softlock | | a reachable cell from which the exit can no longer be reached; the checker warns | `LevelReport` |
| trap | | a cell that hurts whoever steps in: spikes, a death trap, a rock fall, a dart plate, wall spikes. Cowards keep out, the reckless walk through | `TrapHurt`, `Courage` |
| spikes | | a small spike trap in the floor: hurts whoever stands in it, more the longer they stay | `Spike`, `TrapHurt` |
| death trap | | a large spike trap: hurts like spikes, and its tips poison strongly once per entry | `Death`, `TrapHurt` |
| rock fall | | loose ceiling over a walkable cell: a rock falls about 1 s after the player steps in, crushes or grazes | `RockFall` |
| dart plate | | a pressure plate in the floor: the player or a heavy monster on it sets off 3 poison darts from the back wall above. Drafts also say "dart trap" | `DartPlate`, `DartTraps`, `DART_PLATE_WEIGHT` |
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
| ambush | | rooted and still, a fake chest until the player comes near (the mimic; the cursed mimic until it wakes) | `Locomotion::Ambush` |
| cursed mimic | Cursed chest of Set | the mimic's darker kin (purple where a treasure chest is blue): a chest until opened or hit, then it hops after the player and leaps pits and traps; a quarter of its bites lay insanity (its only source); killed, it leaves its chest where it died, half the time an amulet ([cursed-mimic](plan/solved/cursed-mimic.md)) | `MonsterCursedMimic`, `Locomotion::AmbushHop`, `ambushes`, `leaps`, `RollCursedMimicLoot` |
| boss | | one strong monster per boss level; summons minions, its death opens the boss gates | `MonsterKind::isBoss`, `BossRules` |
| burrower | | dives into the floor and comes up elsewhere on its row (Apep) | `Locomotion::Burrow`, `Burrow` |
| charge | | a rush along its row through the player after a wind-up (Sobek, the hippo); range and speed per kind | `Charge`, `ChargeRules`, `MonsterKind::charge` |
| climber | | a monster that follows the player up and down ladders once it has seen them ([monster-climbers](plan/solved/monster-climbers.md)) | `MonsterKind::climbs`, `ClimbMap`, `Dungeon::followPath` |
| coiled | | lies coiled until the player comes near, then rears up (the cobra) | `Locomotion::Coiled` |
| courage | | whether a monster sets foot on a trap: coward or reckless | `Courage` |
| coward | | afraid of traps: a walker stops at their edge, a walk-jumper leaps over them | `Courage::Coward` |
| creature | Creatures | the player's word for a monster; the journal's section | `Journal` |
| entombed | | lies in its coffin until the player comes near, then climbs out (the mummy) | `Locomotion::Entombed` |
| floating | | rooted in half water, a flower on the surface until the player comes near, then rears up and bites (the blue lotus, [water-lotus](plan/solved/water-lotus.md)) | `Locomotion::Floating`, `Monster::floatLift` |
| figurine | | stands still among the decor ushabti of its cell, as one of them, until the player comes near or hits it, then steps off and walks (the living ushabti, [living-ushabti](plan/solved/living-ushabti.md)) | `Locomotion::Figurine`, `FIGURINE_WAKE_RANGE`, `CreatureMove::StepOff` |
| flee | | a coward that cannot reach the player runs along its row out of the bow's range ([coward-flee-ranged](plan/solved/coward-flee-ranged.md)) | `Monster::flees`, `Monster::Flee`, `Dungeon::fleeFrom` |
| flyer | | flies along the row and crosses pits (bats); follows the player up or down to another row through open cells ([bats-high-ceiling](plan/solved/bats-high-ceiling.md)) | `Locomotion::Fly`, `Flight`, `FlightGoal`, `Dungeon::flightGoal` |
| hold | | a bite that grabs: after its tell the monster lunges and holds the player (no walk, jump, drink or screen); the crocodile's and Sobek's death roll ticks (in water only), the giant scorpion's and the queen's hold ends in a sure sting unless the player's hits break it ([crocodile-hold-bite](plan/solved/crocodile-hold-bite.md), [giant-scorpion-grab](plan/solved/giant-scorpion-grab.md)) | `HoldRules`, `MonsterKind::hold`, `Grab`, `Dungeon::PlayerHeldBy` |
| tell | | the warning before a grab: jaws or claws wide for `GRAB_TELL_MS`; stepping back or jumping makes the lunge miss | `GrabPhase::Tell`, `GRAB_TELL_MS` |
| death roll | | the crocodile's hold: it rolls over with the player, a hit with each tick | `HoldRules::tickMs`, `Monster::HoldRoll` |
| kin | | a boss's common monster (the Anubis boss: the Anubis guard, in the game "Anubis living statue"); a new ability for the kin goes to its boss too | `MonsterKind::kin` |
| Egyptian names | | the game shows Egyptian names, the code and the editor keep the old ids and labels: guard = Anubis living statue, Anubis boss = Anubis, boss scarab = Khepri, vampire bat = Bat of Shezmu, mimic = Chest of Set, worm = Sand worm, plant = Thorn acacia, mummy priest = High Priest of Anubis, cursed mimic = Cursed chest of Set; amulets are named by their object (Jackal Tooth, Eye of Horus), labels by their effect | `MonsterKind::name`, `MonsterKind::label` |
| kind | | a monster type: one row of stats, model and rules | `MonsterKind`, `monsterKind` |
| leap | | a walk-jumper's jump over a pit or a trap | `Leap` |
| locomotion | | how a monster gets around: stationary, ambush, walk, walk-jump, fly, ... | `Locomotion` |
| lit cell | | within `LIGHT_SHUN_CELLS` (1) along the row of a flame: a wall torch, a brazier, an oil lamp | `Dungeon::Lit` |
| light shunner | | a walker that does not step into a lit cell; light on its way to the player is a wall to it, so it flees; caught in the light, it runs for the dark (the camel spider, [camel-spider](plan/solved/camel-spider.md)) | `MonsterKind::shunsLight`, `Dungeon::walkerStops`, `Dungeon::wayOutOfLight`, `CreatureMove::ShunLight` |
| lurking | | ambush, entombed, submerged, coiled, floating or a figurine, before it wakes | `Monster::Lurk` |
| minion | | a monster a boss summons; gives less XP | `Monster::Minion`, `Dungeon::MinionXP` |
| summon | | how a boss's minions arrive: dig out, drop, climb out of a coffin, hatch, step out of a wall | `Summon` |
| reckless | | walks straight through traps and takes their damage | `Courage::Reckless` |
| ranged attack | | a monster's shot from afar along its row: the cobras' spit, the Anubis's bolt; range in tiles; clip and sound suffix `_att_ranged` | `RangedRules`, `MonsterKind::ranged`, `Dungeon::MonsterShot`, `ModelState::AttackRanged` |
| spit | | the cobras' ranged attack: a poisoned venom glob | `RangedRules::poison` |
| fire ramp | | the colours of a fire over a particle's life, one per `FireStyle` (flame, venom green) | `FireRamp`, `Fire::VENOM_RAMP` |
| glob with flames | | a spat glob drawn as a cloud of fire sprites with smaller clouds trailing, no ball; ramp per projectile (venom: green). Without flames: a plain ball | `drawGlobWithFlames`, `GlobLook` |
| fire bolt | | the Anubis's ranged attack: a small fireball (orange core in flames, a flickering light), mostly fire, sets the player burning (the boss's longer) | `RangedRules::mix`, `RangedRules::burnMs`, `Fire::BOLT` |
| stamina drain | | a bite that takes a share of the player's max stamina (the blue lotus's sedative, 25%); the journal's "drowsy" note | `MonsterKind::staminaDrainPct`, `CreatureMove::Sedate` |
| stationary | | rooted to its spawn cell, bites when the player is next to it (the plant) | `Locomotion::Stationary` |
| submerged | | lies under the water until the player comes near (the crocodile; the hippo wakes at its charge range) | `Locomotion::Submerged` |
| swimmer | | faster in half water, floats at the surface (the crocodile, the hippo) | `Wading::Swimmer` |
| threat | | (monster) one monster's weight in the checker's difficulty score. Not how dangerous it plays | `MonsterKind::threat`, `monsterThreat` |
| walker | | follows the player along its row, stops at pits | `Locomotion::Walk` |
| walk-jumper | | a walker that leaps pits and traps (giant rat, giant scarab) | `Locomotion::WalkJump` |

## Combat

| Term | In game | Meaning | Code |
|---|---|---|---|
| armour | Armor | taken off every hit on the player (its non-fire shares), at least 1 HP gets through; the game shows "Armor" | `PlayerStats::Armor` |
| burning | You are on fire!, catches fire | a fire hit sets its target burning: 3 s at a fifth of the hit's fire share a second, armour or not; half water puts it out ([fire-damage](plan/solved/fire-damage.md)) | `Burning`, `BURN_MS`, `Fire::body` |
| bite reach | | how close a walker comes before it bites: 0.1 tiles between hitboxes (0.25 for the rooted) | `MONSTER_BITE_REACH`, `ROOTED_BITE_REACH` |
| bite cone | | where a bite lands: a cone from the monster's mouth along the way it bites (35°, 0.15 tiles past its bite reach; a boss's longer and wider, tall walkers strike from up high), or its body; a jump clears a low one, not a tall one ([monster-attack-reach](plan/solved/monster-attack-reach.md)) | `MeleeReach`, `Monster::Reaches`, `Monster::Reach` |
| wind-up | | a bite lands 40% of the monster's `attackMs` after it starts, if the player is still in its bite cone; meanwhile it keeps its facing and follows only forward ([monster-attack-reach](plan/solved/monster-attack-reach.md)) | `MELEE_WINDUP_PCT`, `Monster::Biting`, `Monster::UpdateBite` |
| damage mix | | a weapon's or a monster's damage split into blunt, slash, pierce and fire, in percent | `DamageMix` |
| damage type | blunt, slash, pierce, fire (player reads: smash, cut, stab, fire) | one of the four kinds of damage | `DamageType` |
| fire | fire | the fourth damage type: goes through armour, sets the target burning; only the fire guard cuts it on the player | `DamageType::Fire`, `fireDamage` |
| fire arrow | Dipped 5 arrows in the brazier's fire | interact by a brazier with a bow: the next 5 arrows deal 60% of their damage as fire | `Player::fireArrows`, `Dungeon::lightArrows`, `FIRE_ARROW_SHOTS` |
| flaming arrows | Flaming arrows (a Heka skill), "Stamina skills on" | the skill: with the stamina-skill key (Q) on, a bow shot deals 40% of its damage on top as fire (through the fire resistances, it ignites) for 10% of max stamina. Too little stamina: a normal arrow, the HUD sign blinks. A brazier-lit arrow neither gets the bonus nor costs. Rule behind it: whoever resists pierce is weak to fire, bar the scorpions and the lotus ([plan](plan/solved/skill-flaming-arrows.md)) | `SkillId::FlamingArrows`, `FLAMING_ARROW_BONUS_PERCENT`, `FLAMING_ARROW_STAMINA_PERCENT`, `Player::staminaSkills`, `BindAction::StaminaSkills` |
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
| curse | A curse! | a timed debuff a monster's hit lays on the player: 2 minutes (1 with the Blessing of Isis skill), several at once, the same again restarts it; no cure, no resistance, ends on death. Purple-framed HUD tiles | `Curses`, `Curse`, `CURSE_MS`, `CURSE_MS_ISIS` |
| curse names | Weakness, Clumsiness, Vulnerability, Fatigue, Disease, Slowness, Palsy, Insanity | weakness -50% Might (greater -125%); clumsiness -20% hit damage (greater -50%); vulnerability -60% armour; fatigue stamina to 20%, no regeneration; disease -30% max HP (greater -70%); slowness half walk, sprint 1.25 tiles/s; palsy no ranged shot; insanity screens, quick drinks and weapon keys locked, club in hand | `CURSES`, `curseDef` |
| Blessing of Hapi | Blessing of Hapi, Hapi | the potion that lifts half water's limits from the player: wade at the walk speed, sprint at 40% of the land sprint, jump. 2 minutes (4 with the Favour of Hapi skill, which also gives 125% wade and the full land sprint at half the drain); ends on death. Blue-green HUD tile. The skill alone: 75% wade, sprint at 50% | `ItemKind::WaterBlessing`, `PlayerStats::WaterBlessed`, `Dungeon::PlayerWalkFactor` |
| greater curse | Greater weakness | a curse at its stronger grade: its own entry on a monster, with a lower chance | `CurseGrade` |
| curse entry | | a monster's chance to lay one curse with a melee hit or with its spit / bolt; at most three each (most kinds one or two), the strongest first, one curse a hit | `CurseEntry`, `MonsterKind::meleeCurses`, `MonsterKind::rangedCurses` |

## Items

| Term | In game | Meaning | Code |
|---|---|---|---|
| amulet | Amulet | worn one at a time, gives its bonus while on; four tiers per type (regeneration: normal and grand only) | `AmuletType`, `AmuletTier` |
| bag | Inventory | what the player carries: counts, weapon levels, the worn amulet | `ItemBag` |
| container | | what a treasure looks like: a wooden chest, a painted coffer, a reed basket, canopic jars, a sealed jar (it smashes), an offering table. Closed until opened (Open), its clip, then its item shows (Take); emptied, it stays open. Rolled per cell by depth, wooden chests 40 % or more (the mimic hides among them) | `Container`, `CONTAINER_DEFS`, `CONTAINER_PERCENT` |
| fire guard | Sun Disc | the amulet that cuts fire damage, 20 / 40 / 60 / 80% (never more: fire always hurts) | `AmuletType::FireWard` |
| group | Weapons, Potions, Amulets, Rings | an inventory tab; no rings yet, so that tab stays disabled | `ItemGroup` |
| heka | Heka | the point a slain boss gives (once per level), spent on a skill on the Heka screen (K); also the screen's name | `Heka`, `SkillsScreen` |
| item kind | | one item: a weapon, a potion, an amulet tier; the save ids follow it | `ItemKind` |
| journal | Journal | the archaeologist's notebook: creatures, riddles, field notes, inscriptions | `Journal` |
| level | Lv | (weapon) a weapon's level, raised by copies found. Not the map level or the player's level | `ItemBag::Level`, `upgradeCost` |
| note | Field notes | a journal entry on a game rule, written the first time it matters | `FieldNote` |
| potion | | drunk from the inventory or a quick key | `PotionDef` |
| antidote | Antidote | the potion that cures poison; the checker wants one where something poisons | `ItemKind::Antidote` |
| quick-drink | | the quick keys: drink the best fitting health or stamina potion | `Inventory::QuickDrink`, `quickPotion` |
| sealed | Sealed | a skill shown on the Heka screen whose rules are not in the game yet: not learnable | `SkillDef::sealed` |
| dodge | Dodged | the Dodge skill: 1 monster attack in 10 (`DODGE_PERCENT`) misses the player: no damage, no poison | `Player::Dodges` |
| stance | | how a container stands in its cell: facing the camera, angled, side-on, against the back wall or a side wall | `STANCES`, `TreasureLook` |
| skill | | a permanent ability bought with one heka; no respec | `SkillId`, `SKILLS` |
| skill names | Stun, Words of Thoth, Dodge, Power hit, Sharp eye, Favour of Hapi, Serket's favour, Blessing of Isis, Flaming arrows | Words of Thoth reads (translates) inscriptions; Sharp eye ranged weapons reach 30% farther; Favour of Hapi wade at 75%, sprint and jump in half water; Serket's favour poisons half as long; Blessing of Isis curses half as long. Stun and Power hit are sealed. Dodge, flaming arrows: their rows | `SKILLS`, `skillDef`, `SkillId::Hieroglyphs` |
| tier | Lesser, Minor, Grand | an amulet's strength: lesser, minor, normal, grand ("Lesser Amulet of Armor", "Amulet of Minor Armor") | `AmuletTier` |

## Runs

| Term | In game | Meaning | Code |
|---|---|---|---|
| autosave | Autosave | the one save the game writes itself at safe moments (a level start, a riddle answered, a boss slain), never while the player is in danger; `saves/autosave.sav`, read-only for the player ([autosave](plan/solved/autosave.md)) | `GameState::Autosave`, `SaveSlots::AUTOSAVE`, `AutosaveMoment` |
| Continue | Continue | the main menu's first button while an autosave exists: loads it | `MenuAction::Continue`, `GameState::LoadAutosave` |
| campaign run | | a run begun with New Game on level 1; only its win goes on the leaderboard (scenario `campaign`) | `RunStats::campaign` |
| death screen | You died | the panel over the dimmed game after the death clip: Retry from Autosave, Main Menu | `DeathScreen`, `Screen::Death` |
| deaths | Deaths | a run's deaths, counted at the death and written into the autosave then, so a retry keeps them | `RunStats::deaths` |
| exploration score | Explored | a run's inscriptions read x10 + treasures picked up x10 + riddles answered x30 | `explorationScore` |
| Hall of Records | Hall of Records | the main menu's leaderboard screen: rank, name, score in the chosen order; a row opens the run's details | `RecordsView` |
| leaderboard | Hall of Records | the won campaign runs, one record each, in `saves/leaderboard.txt` (never in a save game) | `Leaderboard`, `RunRecord` |
| play time | Time | a run's time in play: menus, the UI screens and the win do not count; per level on the details | `RunStats::PlayMs` |
| placeholder row | Spirit of Exploration | the empty board's one row, never written to the file | `placeholderRecord` |
| name list | | the royal names of the name gates, `content/names/*.txt` ([names](names.md)); each run gives every level its own (the run's name seed) | `NameList`, `RoyalName`, `RunStats::nameSeed` |
| run statistics | | what a run did since New Game (kills, potions, play time, exploration, damage, times poisoned and cursed, deaths), kept in the save | `RunStats` |
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
