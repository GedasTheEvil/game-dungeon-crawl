// The monster balance sim pass (plan monster-balance, step 5): every monster against the player of the levels it
// appears on, in the real world (SimWorld, the unit tests' sim), with three tactics: tank it in melee, kite it with the
// bow, run past it. The player of a level comes from the XP sum of the target campaign's zone levels, generated
// (level_gen.h) on a ramp of difficulty and size, and the gear found on them.
//
//   balance curve                  the player level and gear at the start of each target campaign level
//   balance today                  the same for the campaign's levels (content/levels/)
//   balance tune [--gear G]        each monster's HP and damage for its class's target in its home zone (G: play)
//   balance home MONSTER [...]     one monster against the player of its home zone (play gear), fight's options
//   balance table [--gear G]       every zone's monsters and boss against its player (G: gen, the default, or play)
//   balance fight MONSTER LEVEL [--armor A] [--might M] [--hp P]
//                                  one monster (its label: "giant rat") against a player of that level
//   ... --amulet ID                every fight with that amulet worn (its place among the amulets)
//
// Gear "gen": the might, armour and life potions on the generated levels, the level's own gains, an amulet of the
// depth's tier; "play": the user's playthrough (plan monster-balance, Playthrough reference data), as a share of the
// level's own gains.
#include "../../tests/unit/sim_world.h"
#include "../../src/world/level_gen.h"
#include "../../src/world/progression.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <vector>

namespace {
// The target campaign (plan campaign-zones, Zones decided 2026-10-21): the levels of each ZoneId.
struct TargetZone {
	ZoneId id;
	int first, last;
};
constexpr TargetZone TARGET[] = {
	{ZoneId::TombEntrance, 1, 4},
	{ZoneId::BatGalleries, 5, 9},
	{ZoneId::WebHalls, 10, 13},
	{ZoneId::PriestsQuarter, 14, 18},
	{ZoneId::ScorpionNest, 19, 22},
	{ZoneId::SerpentTemple, 23, 27},
	{ZoneId::CisternsOfSobek, 28, 32},
	{ZoneId::SethsDesert, 33, 37},
	{ZoneId::LandOfSokar, 38, 41},
	{ZoneId::LakeOfFire, 42, 45},
	{ZoneId::Necropolis, 46, 50},
	{ZoneId::HallOfJudgment, 51, 54},
};
constexpr int TARGET_LEVELS = 54;

const char* zoneSlug(ZoneId id) { return ZONE_INFO[static_cast<int>(id)].slug; }

const TargetZone& targetZoneOf(int level) {
	for (const TargetZone& z : TARGET)
		if (level <= z.last)
			return z;
	return TARGET[std::size(TARGET) - 1];
}

// The ramp the rebuild aims at (plan denser-levels): difficulty 1 to 10 over the campaign, the open cells growing to
// the full grid by the third zone (ZONE_SIZE, campaign.h). A size that fails is grown until it works.
int difficultyOf(int level) { return 1 + (9 * (level - 1) + TARGET_LEVELS / 2) / (TARGET_LEVELS - 1); }
int openCellsOf(int level) {
	const TargetZone& z = targetZoneOf(level);
	return zoneLevelCells(z.id, level - z.first, z.last - z.first + 1);
}

// The campaign's end, as today's 35 levels (plan longer-campaign): about player level 60. The XP the generated levels
// give is scaled to land there (balance curve prints the factor).
constexpr int END_LEVEL = 60;

// What the player has at the start of a campaign level.
struct Player {
	int level = 1;
	double xp = 0;
	int might = 0, armor = 0, lifePotions = 0; // potions drunk
	int amuletTier = -1; // the best amulet of the depth found so far (0 lesser .. 3 grand), -1 none
	int difficulty = 1;
};

// The amulet bonus by tier (items.cpp: Strength, Armor {1, 2, 4, 6}).
constexpr int AMULET_STAT[] = {1, 2, 4, 6};

int levelOf(double xp) {
	int level = 1;
	while (levelXP(level + 1) <= xp)
		level++;
	return level;
}

// What a level gives: its monsters' XP, its riddles, its boss; the gear on it goes into p.
struct Tally {
	int monsters = 0, riddles = 0, boss = 0;
	double xp = 0;
};
Tally tallyLevel(const LevelGrid& grid, Player& p) {
	Tally tally;
	for (const Tile& t : grid.cells) {
		if (t.type == MonsterSpawn && monsterKind(t.attr) != nullptr) {
			const MonsterKind& kind = *monsterKind(t.attr);
			tally.xp += kind.xp;
			tally.monsters++;
			if (kind.isBoss())
				tally.boss = t.attr;
		} else if (t.type == Door && t.attr == GateRiddle)
			tally.riddles++;
		else if (t.type == Treasure || t.type == TrappedTreasure) {
			const std::optional<ItemKind> item = itemFromFile(t.attr, t.value);
			if (item == ItemKind::Might)
				p.might++;
			else if (item == ItemKind::Armor)
				p.armor++;
			else if (item == ItemKind::Life)
				p.lifePotions++;
			else if (item && isAmulet(*item))
				p.amuletTier = std::max(p.amuletTier, static_cast<int>(amuletOf(*item).tier));
		}
	}
	return tally;
}

// Generates the target campaign and sums what each level gives, its XP times xpScale. players[L]: at the start of level
// L (1-based).
std::vector<Player> buildCurve(bool print, double xpScale) {
	std::vector<Player> players(TARGET_LEVELS + 2);
	Player p;
	if (print)
		std::printf(
			"lvl zone              d cells  monsters    XP  riddles  boss  | start: plvl  might armor life amulet\n"
		);
	for (int level = 1; level <= TARGET_LEVELS; level++) {
		const TargetZone& zone = targetZoneOf(level);
		p.level = levelOf(p.xp);
		p.difficulty = difficultyOf(level);
		players[static_cast<size_t>(level)] = p;
		GenOptions options;
		options.seed = static_cast<uint32_t>(level);
		options.difficulty = p.difficulty;
		options.zone = static_cast<int>(zone.id);
		options.openCells = openCellsOf(level);
		options.boss = level == zone.last && zoneBoss(zone.id) != 0;
		GenResult gen = generateLevel(options);
		while (!gen.ok && options.openCells < ZONE_OPEN_CELLS) {
			options.openCells += 50;
			gen = generateLevel(options);
		}
		const Tally tally = tallyLevel(gen.grid, p);
		const int monsters = tally.monsters, riddles = tally.riddles, boss = tally.boss;
		double xp = tally.xp;
		xp *= xpScale;
		for (int r = 0; r < riddles; r++)
			xp += riddleXP(levelOf(p.xp + xp));
		if (print)
			std::printf(
				"%3d %-17s %2d %5d  %8d %5.0f  %7d  %-4s  | %4d %6d %5d %4d %6d\n",
				level,
				zoneSlug(zone.id),
				p.difficulty,
				options.openCells,
				monsters,
				xp,
				riddles,
				boss != 0 ? "yes" : "",
				p.level,
				p.might,
				p.armor,
				p.lifePotions,
				p.amuletTier
			);
		p.xp += xp;
	}
	p.level = levelOf(p.xp);
	players[TARGET_LEVELS + 1] = p;
	if (print)
		std::printf("end: player level %d, XP %.0f (monster XP x %.2f)\n", p.level, p.xp, xpScale);
	return players;
}

// The campaign's levels (content/levels/; the rebuilt campaign since plan denser-levels, the old 35 levels the
// playthrough was on before), as buildCurve prints the target's. The player level at the start of each level, [1]
// for lvl1.
std::vector<int> todayCurve(bool print) {
	std::vector<int> levels(CAMPAIGN_LEVELS + 2, 1);
	Player p;
	if (print)
		std::printf("lvl  monsters    XP  riddles  boss  | start: plvl  might armor life amulet\n");
	for (int level = 1; level <= CAMPAIGN_LEVELS; level++) {
		LevelGrid grid;
		if (!loadLevelFile(campaignLevelFile(level).c_str(), grid).empty())
			continue;
		p.level = levelOf(p.xp);
		levels[static_cast<size_t>(level)] = p.level;
		const Player start = p;
		Tally tally = tallyLevel(grid, p);
		for (int r = 0; r < tally.riddles; r++)
			tally.xp += riddleXP(levelOf(p.xp + tally.xp));
		if (print)
			std::printf(
				"%3d  %8d %6.0f  %7d  %-4s  | %4d %6d %5d %4d %6d\n",
				level,
				tally.monsters,
				tally.xp,
				tally.riddles,
				tally.boss != 0 ? "yes" : "",
				start.level,
				start.might,
				start.armor,
				start.lifePotions,
				start.amuletTier
			);
		p.xp += tally.xp;
	}
	levels[CAMPAIGN_LEVELS + 1] = levelOf(p.xp);
	if (print)
		std::printf("end: player level %d\n", levels[CAMPAIGN_LEVELS + 1]);
	return levels;
}

// The player level at the start of target level L at today's pace: the same share of the way through the campaign
// as on today's levels, the played curve (the rebuild keeps it, plan denser-levels: recheck the XP sum).
// The weapons found by target level L at today's pace: the difficulty whose levelgen depth (genDecorDepth) is today's
// level at the same share of the way (the sword and spear from lvl3, the axes from lvl10, as on today's levels).
int pacedDifficulty(int level) {
	const double at = 1.0 + (level - 1) * static_cast<double>(CAMPAIGN_LEVELS) / static_cast<double>(TARGET_LEVELS);
	return std::clamp(static_cast<int>(1.0 + 2.0 * (at - 1.0) / 3.0), GEN_MIN_DIFFICULTY, GEN_MAX_DIFFICULTY);
}

int pacedLevel(const std::vector<int>& today, int level) {
	const double at = 1.0 + (level - 1) * static_cast<double>(CAMPAIGN_LEVELS) / static_cast<double>(TARGET_LEVELS);
	const auto i = static_cast<size_t>(at);
	const double part = at - static_cast<double>(i);
	return static_cast<int>(std::lround(today[i] + part * (today[std::min(i + 1, today.size() - 1)] - today[i])));
}

// The monster XP factor that ends the campaign at END_LEVEL.
double endScale() {
	double lo = 0.05, hi = 2.0;
	for (int i = 0; i < 20; i++) {
		const double mid = (lo + hi) / 2;
		(buildCurve(false, mid)[TARGET_LEVELS + 1].level > END_LEVEL ? hi : lo) = mid;
	}
	return lo;
}

// The player's stats for a fight.
struct Stats {
	int level = 1;
	int armor = 0; // on top of the level's own
	int might = 0;
	int hpPercent = 0; // max HP on top of the level's own
	int difficulty = 1; // which weapons are found by now
};

// gear "gen": the potions found, the amulet worn (armour; a strength one would be the same lever on the other side).
Stats genStats(const Player& p) {
	Stats s;
	s.level = p.level;
	s.armor = p.armor + (p.amuletTier >= 0 ? AMULET_STAT[p.amuletTier] : 0);
	s.might = p.might;
	s.hpPercent = static_cast<int>(std::lround((std::pow(1.05, p.lifePotions) - 1.0) * 100.0));
	s.difficulty = p.difficulty;
	return s;
}

// gear "play": the playthrough's (player level 42: armour 18, might 21, HP +36%; level 50: armour 32, might 34, HP
// +73%), by player level: the extra over the level's own gains grows from level 20 on, straight through both points.
Stats playStats(const Player& p) {
	Stats s = genStats(p);
	const double past = std::max(0, p.level - 20);
	s.armor = static_cast<int>(std::lround(past * 22.0 / 30.0)); // +10 at 42, +22 at 50 (fit through 50, from 20)
	s.might = static_cast<int>(std::lround(past * 28.0 / 30.0));
	s.hpPercent = static_cast<int>(std::lround(past * 73.0 / 30.0));
	return s;
}

// An arena: one long hall on ARENA_ROW, the player's entrance at col entry, the monster's spawn at col monster; water
// monsters get a pool of half water over deep water round their spawn, sand burrowers sand.
constexpr int ARENA_ROW = 10;
constexpr int ARENA_FIRST = 1, ARENA_LAST = 38;

LevelGrid arena(const MonsterKind& kind, int entry, int monster) {
	LevelGrid grid;
	for (int col = ARENA_FIRST; col <= ARENA_LAST; col++)
		grid.cells[col + ARENA_ROW * LEVEL_WIDTH].structure = Structure::Empty;
	const bool water = kind.locomotion == Locomotion::Submerged || kind.locomotion == Locomotion::Floating;
	if (water || kind.sandBurrow)
		for (int col = monster - 3; col <= monster + 3; col++) {
			grid.cells[col + ARENA_ROW * LEVEL_WIDTH].structure = water ? Structure::HalfWater : Structure::Sand;
			if (water)
				grid.cells[col + (ARENA_ROW + 1) * LEVEL_WIDTH].structure = Structure::DeepWater;
		}
	Tile& door = grid.cells[entry + ARENA_ROW * LEVEL_WIDTH];
	door.type = Door;
	door.attr = GateEntrance;
	Tile& spawn = grid.cells[monster + ARENA_ROW * LEVEL_WIDTH];
	spawn.type = MonsterSpawn;
	spawn.attr = static_cast<int>(kind.id);
	return grid;
}

// The melee weapon a player takes against the kind among those found by the difficulty: the hardest hit (the damage
// the inventory shows), then the quicker. By damage a second the knife would win once Might is high (it adds to each
// hit), with a tenth of a tile's reach.
ItemKind bestMelee(const MonsterKind& kind, int difficulty, int might) {
	ItemKind best = ItemKind::Club;
	double bestHit = 0;
	for (int i = 0; i < ITEM_KIND_COUNT; i++) {
		const ItemKind item = itemAt(i);
		if (!isWeapon(item) || isRanged(item) || weaponDef(item).generatedFrom > difficulty)
			continue;
		const WeaponDef& def = weaponDef(item);
		const double hit = resistedDamage(def.damage + might, def.mix, kind.resist) +
						   1.0 / static_cast<double>(def.motion.AttackMs());
		if (hit > bestHit) {
			bestHit = hit;
			best = item;
		}
	}
	return best;
}
ItemKind bestBow(int difficulty) {
	return weaponDef(ItemKind::CompositeBow).generatedFrom <= difficulty ? ItemKind::CompositeBow : ItemKind::SelfBow;
}

enum class Tactic : std::uint8_t { Tank, Kite, RunPast };
constexpr const char* TACTIC_NAMES[] = {"tank", "kite", "run past"};

struct Outcome {
	bool killed = false; // the monster (tank, kite) / got past it (run past)
	bool died = false;
	int maxHp = 0;
	int taken = 0; // damage taken in all, potions and level-up heals aside
	int ms = 0; // to the kill or past it
	bool bowOnly = false; // kite: killed with the bow, never cornered
	int potions = 0; // large health potions drunk
	ItemKind weapon = ItemKind::Club;
	[[nodiscard]] int lostPercent() const { return maxHp > 0 ? 100 * taken / maxHp : 0; }
};

const Monster* theMonster(const SimWorld& world, MonsterTypeId id) {
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = world.dungeon.Monsters()[i];
		if (mon.Active() && mon.Type()->id == id && !mon.Minion())
			return &mon;
	}
	return nullptr;
}

// --amulet ID (any mode): the amulet worn in every fight, its place among the amulets (tools/editor/readme.md), on top
// of the gear's own (gear "gen" counts an armour amulet).
std::optional<ItemKind> wornAmulet;

void setUp(SimWorld& world, const Stats& s) {
	world.player.stats.AddXP(static_cast<int>(std::ceil(levelXP(s.level))), world.events);
	world.player.stats.AddArmor(s.armor);
	world.player.stats.AddMight(s.might);
	if (s.hpPercent > 0)
		world.player.stats.AddMaxHP(s.hpPercent);
	if (wornAmulet)
		world.Wear(*wornAmulet);
	world.player.Reanimate();
}

constexpr int FIGHT_MS = 120000;
// The player drinks a large health potion (50%) below this share of max HP, up to POTIONS a fight, a boss fight
// BOSS_POTIONS (the quick-drink key; a potion takes no time).
constexpr int DRINK_BELOW_PERCENT = 35;
constexpr int POTIONS = 1, BOSS_POTIONS = 4;
constexpr int RUN_MS = 30000;

Outcome fight(const MonsterKind& kind, const Stats& s, Tactic tactic, uint64_t seed) {
	SimWorld world;
	// The player at col 30: the monster 4 tiles west, at the edge of the spawn window (Dungeon::view); 7 tiles of room
	// east to back off. Run past: to the far end of the hall, 23 tiles past its spawn (a chaser follows).
	constexpr int ENTRY = 30, MONSTER = 26, PAST = ARENA_FIRST + 2;
	world.random.Seed(seed);
	world.dungeon.LoadGrid(arena(kind, ENTRY, MONSTER), "balance");
	world.dungeon.ClearWin();
	setUp(world, s);
	Outcome out;
	out.weapon = tactic == Tactic::Kite ? bestBow(s.difficulty) : bestMelee(kind, s.difficulty, s.might);
	world.Equip(out.weapon);
	out.maxHp = world.player.stats.CurrentMaxHP();
	world.Tick();
	const Monster* mon = theMonster(world, kind.id);
	if (mon == nullptr)
		return out;
	const float reach = weaponReach(bestMelee(kind, s.difficulty, s.might));
	const float keep = std::max(1.2f, weaponMinRange(out.weapon) + 0.3f); // kite: backs off when closer
	bool cornered = false;
	const int limit = tactic == Tactic::RunPast ? RUN_MS : FIGHT_MS;
	for (int t = 0; t < limit; t += UPDATE_TICK_MS) {
		if (!world.player.Alive()) {
			out.died = true;
			break;
		}
		if (tactic == Tactic::RunPast ? world.X() <= static_cast<float>(PAST) : !mon->Alive()) {
			out.killed = true;
			out.ms = t;
			break;
		}
		const int maxHp = world.player.stats.CurrentMaxHP();
		if (world.Hp() * 100 < maxHp * DRINK_BELOW_PERCENT && out.potions < (kind.isBoss() ? BOSS_POTIONS : POTIONS)) {
			world.player.stats.Drink(potionGain(ItemKind::LargeHealth));
			out.potions++;
		}
		const int dir = mon->CentreX() > world.X() ? 1 : -1;
		const float gap = mon->PlayerGap(world.X());
		switch (tactic) {
		case Tactic::Tank: // up to it and swinging whenever it is in reach; a flyer it waits for, swinging at its swoop
			world.facing = dir;
			if (gap > reach * 0.8f && (!mon->flies() || !mon->Alerted()))
				world.Step(dir);
			if (gap <= reach || (mon->flies() && gap < 1.f)) // a swoop is quick: swinging as it comes
				world.Attack();
			break;
		// Up to it until it wakes (a coffin, a coil, a chest), then backing off out of the bow's minimum range and
		// shooting; out of shots it waits for them there (plan ranged-weaker-than-melee). At the wall it fights with
		// the melee weapon.
		case Tactic::Kite:
			if (!mon->Alerted() && !cornered) {
				world.facing = dir;
				world.Step(dir);
				if (gap <= 0.f)
					world.Attack();
			} else if (!cornered && gap < keep && mon->Alive()) {
				const float before = world.X();
				world.Step(-dir);
				if (world.X() == before) { // the wall: it fights
					cornered = true;
					world.Equip(bestMelee(kind, s.difficulty, s.might));
				}
			} else if (cornered && gap > reach * 0.8f) {
				world.facing = dir;
				world.Step(dir);
			} else {
				world.facing = dir;
				world.Attack();
			}
			break;
		case Tactic::RunPast:
			world.player.stats.SetSprintRequested(true);
			world.Step(-1);
			if (gap < 0.5f && mon->CentreX() < world.X())
				world.Jump();
			break;
		}
		const int before = world.Hp();
		if (std::getenv("BALANCE_TRACE") != nullptr && t % 1000 == 0)
			std::fprintf(
				stderr,
				"t%d %s px %.2f mx %.2f gap %.2f hp %d mhp %d jump %d\n",
				t,
				TACTIC_NAMES[static_cast<int>(tactic)],
				world.X(),
				mon->CentreX(),
				gap,
				world.Hp(),
				mon->Health(),
				mon->jumping() ? 1 : 0
			);
		world.Tick();
		out.taken += std::max(0, before - world.Hp());
	}
	out.bowOnly = tactic == Tactic::Kite && out.killed && !cornered;
	return out;
}

// Seeds per tactic: the bites, grabs and poison are rolled.
constexpr int SEEDS = 3;

struct Summary {
	int lost = 0; // average % of max HP lost
	int deaths = 0;
	int wins = 0; // killed / got past
	int seconds = 0; // average to the win
	int bowOnly = 0;
	int potions = 0; // drunk, summed over the seeds
	ItemKind weapon = ItemKind::Club;
};

Summary run(const MonsterKind& kind, const Stats& s, Tactic tactic) {
	Summary sum;
	int ms = 0;
	for (int seed = 1; seed <= SEEDS; seed++) {
		const Outcome o = fight(kind, s, tactic, static_cast<uint64_t>(seed));
		sum.lost += o.lostPercent();
		sum.deaths += o.died ? 1 : 0;
		sum.wins += o.killed ? 1 : 0;
		sum.bowOnly += o.bowOnly ? 1 : 0;
		sum.potions += o.potions;
		ms += o.ms;
		sum.weapon = o.weapon;
	}
	sum.lost /= SEEDS;
	sum.seconds = sum.wins > 0 ? ms / sum.wins / 1000 : 0;
	return sum;
}

std::string cell(const Summary& s, Tactic tactic) {
	char buf[64];
	if (s.deaths == SEEDS)
		std::snprintf(buf, sizeof(buf), "dies");
	else if (s.wins == 0 && s.deaths == 0)
		std::snprintf(buf, sizeof(buf), "%s", tactic == Tactic::RunPast ? "STOP" : "no kill");
	else if (tactic == Tactic::RunPast)
		std::snprintf(buf, sizeof(buf), "%s %3d%%", s.wins == SEEDS ? "past" : "STOP", s.lost);
	else
		std::snprintf(
			buf,
			sizeof(buf),
			"%3d%% %3ds%s%s%s",
			s.lost,
			s.seconds,
			s.deaths > 0 ? " D" : "",
			tactic == Tactic::Kite && s.bowOnly == SEEDS ? " b" : "",
			s.potions > 0 ? (" p" + std::to_string((s.potions + SEEDS - 1) / SEEDS)).c_str() : ""
		);
	return buf;
}

void printHeader() {
	std::printf(
		"%-13s %-20s %4s %4s %4s %4s  %-9s %-16s %-16s %-10s\n",
		"zone",
		"monster",
		"plvl",
		"hp",
		"arm",
		"mig",
		"melee",
		"tank lost/time",
		"kite lost/time",
		"run past"
	);
}

void printRow(const char* zone, const MonsterKind& kind, const Stats& s) {
	SimWorld probe; // the player's max HP of these stats
	setUp(probe, s);
	const Summary tank = run(kind, s, Tactic::Tank);
	const Summary kite = run(kind, s, Tactic::Kite);
	const Summary past = run(kind, s, Tactic::RunPast);
	std::printf(
		"%-13s %-20s %4d %4d %4d %4d  %-9s %-16s %-16s %-10s\n",
		zone,
		kind.label,
		s.level,
		probe.player.stats.CurrentMaxHP(),
		probe.player.stats.CurrentArmor(),
		probe.player.stats.CurrentMight(),
		itemText(tank.weapon).shortName,
		cell(tank, Tactic::Tank).c_str(),
		cell(kite, Tactic::Kite).c_str(),
		cell(past, Tactic::RunPast).c_str()
	);
	std::fflush(stdout);
}

// A target for the tank in the home zone: the share of max HP a fight costs and its length.
struct Target {
	int lostPercent;
	int seconds;
};
// By the monster's place (plan monster-balance, 2026-10-26): a weak one a nip, a giant a fight, the Anubis guard the
// zone's terror, a boss two or three potions.
Target targetOf(const MonsterKind& kind) {
	if (kind.isBoss())
		return {150, 30};
	switch (kind.id) {
	case MonsterRat:
	case MonsterScarab:
	case MonsterBat:
	case MonsterScorpion:
	case MonsterCamelSpider:
		return {6, 3};
	case MonsterPlant:
	case MonsterWorm:
	case MonsterUshabti:
	case MonsterLotus:
	case MonsterCobra:
		return {10, 4};
	case MonsterHippo:
	case MonsterCursedMimic:
		return {30, 8};
	case MonsterAnubis:
		return {45, 10};
	default:
		return {20, 6};
	}
}

// Scales a kind's HP and every hit it deals in the sim's monster types (the game's KINDS stay as they are).
void scaleKind(int id, double hp, double damage) {
	auto& types = const_cast<MonsterTypes&>(
		SimWorld::monsterTypes()
	); // NOLINT(cppcoreguidelines-pro-type-const-cast): the tool's own copy
	MonsterType& type = types[static_cast<size_t>(id)];
	const MonsterKind& kind = *monsterKind(id);
	const auto scaled = [](int v, double f) { return std::max(1, static_cast<int>(std::lround(v * f))); };
	type.maxHealth = scaled(kind.maxHealth, hp);
	type.damage = kind.damage > 0 ? scaled(kind.damage, damage) : 0;
	if (kind.ranged && type.ranged)
		type.ranged->damage = scaled(kind.ranged->damage, damage);
	if (kind.roll && type.roll)
		type.roll->damage = scaled(kind.roll->damage, damage);
	if (kind.roar && type.roar && kind.roar->damage > 0)
		type.roar->damage = scaled(kind.roar->damage, damage);
}

// The first zone a kind lives in (ZONE_FAUNA; the mimic in the first with mimics, the cursed one in the first with
// cursed ones; a boss in its own), -1 for none.
int homeZone(const MonsterKind& kind) {
	for (int z = 0; z < ZONE_ID_COUNT; z++) {
		const ZoneFauna& fauna = zoneFauna(static_cast<ZoneId>(z));
		if (zoneBoss(static_cast<ZoneId>(z)) == kind.id || (kind.id == MonsterMimic && fauna.mimicPercent > 0) ||
			(kind.id == MonsterCursedMimic && fauna.cursedPercent > 0))
			return z;
		for (const ZoneMonster& m : fauna.monsters)
			if (m.type == kind.id && m.type != 0)
				return z;
	}
	return -1;
}

// The player a kind meets in its home zone: halfway through it, a boss at its level.
Stats homeStats(const MonsterKind& kind, bool play) {
	static const std::vector<Player> PLAYERS = buildCurve(false, 1.0);
	static const std::vector<int> TODAY = todayCurve(false);
	const TargetZone& zone = TARGET[static_cast<size_t>(std::max(0, homeZone(kind)))];
	const int level = kind.isBoss() ? zone.last : (zone.first + zone.last) / 2;
	Player p = PLAYERS[static_cast<size_t>(level)];
	p.level = pacedLevel(TODAY, level);
	p.difficulty = pacedDifficulty(level);
	return play ? playStats(p) : genStats(p);
}

int tune(bool play) {
	std::printf(
		"%-20s %-17s %4s  %-12s %5s %5s  %-16s -> %5s %5s  %-16s %-16s\n",
		"monster",
		"home zone",
		"plvl",
		"target",
		"hp",
		"dmg",
		"tank now",
		"hp",
		"dmg",
		"tank tuned",
		"kite tuned"
	);
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& kind = *monsterKind(id);
		const int z = homeZone(kind);
		if (z < 0 || kind.damage <= 0)
			continue;
		const Stats s = homeStats(kind, play);
		const Target target = targetOf(kind);
		scaleKind(id, 1.0, 1.0);
		const Summary before = run(kind, s, Tactic::Tank);
		double hp = 1.0, damage = 1.0;
		Summary now = before;
		for (int round = 0; round < 6; round++) {
			// A death or no kill: no time to read; else the time follows the HP, the share lost the damage.
			const double seconds = now.wins > 0 ? std::max(1, now.seconds) : target.seconds * 3.0;
			hp = std::clamp(hp * target.seconds / seconds, 0.1, 10.0);
			scaleKind(id, hp, damage);
			now = run(kind, s, Tactic::Tank);
			const double lost = now.deaths > 0 ? target.lostPercent * 3.0 : std::max(1, now.lost);
			damage = std::clamp(damage * target.lostPercent / lost, 0.1, 10.0);
			scaleKind(id, hp, damage);
			now = run(kind, s, Tactic::Tank);
		}
		const Summary kite = run(kind, s, Tactic::Kite);
		char goal[32];
		std::snprintf(goal, sizeof(goal), "%d%% %ds", target.lostPercent, target.seconds);
		std::printf(
			"%-20s %-17s %4d  %-12s %5d %5d  %-16s -> %5d %5d  %-16s %-16s\n",
			kind.label,
			zoneSlug(static_cast<ZoneId>(z)),
			s.level,
			goal,
			kind.maxHealth,
			kind.damage,
			cell(before, Tactic::Tank).c_str(),
			std::max(1, static_cast<int>(std::lround(kind.maxHealth * hp))),
			std::max(1, static_cast<int>(std::lround(kind.damage * damage))),
			cell(now, Tactic::Tank).c_str(),
			cell(kite, Tactic::Kite).c_str()
		);
		std::fflush(stdout);
		scaleKind(id, 1.0, 1.0);
	}
	return 0;
}

const MonsterKind* kindByLabel(const char* label) {
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++)
		if (const MonsterKind* kind = monsterKind(id); kind != nullptr && std::strcmp(kind->label, label) == 0)
			return kind;
	return nullptr;
}

int table(bool play) {
	const std::vector<Player> players = buildCurve(false, 1.0);
	const std::vector<int> today = todayCurve(false);
	const auto statsAt = [&](int level) {
		Player p = players[static_cast<size_t>(level)];
		p.level = pacedLevel(today, level);
		p.difficulty = pacedDifficulty(level);
		return play ? playStats(p) : genStats(p);
	};
	printHeader();
	for (const TargetZone& zone : TARGET) {
		// The player halfway through the zone; its boss at the start of the boss level.
		const Stats s = statsAt((zone.first + zone.last) / 2);
		for (const ZoneMonster& m : zoneFauna(zone.id).monsters)
			if (m.type != 0)
				printRow(zoneSlug(zone.id), *monsterKind(m.type), s);
		if (zoneFauna(zone.id).mimicPercent > 0)
			printRow(zoneSlug(zone.id), *monsterKind(MonsterMimic), s);
		if (const int boss = zoneBoss(zone.id); boss != 0)
			printRow(zoneSlug(zone.id), *monsterKind(boss), statsAt(zone.last));
	}
	return 0;
}

int usage() {
	std::fprintf(
		stderr,
		"usage: balance curve | today | tune | table [--gear gen|play] | fight MONSTER LEVEL | home MONSTER [--armor "
		"A] [--might M] "
		"[--hp P] [--difficulty D] [--monster-hp HP] [--monster-damage D] [--minions N]; any: [--amulet ID]\n"
	);
	return 2;
}
} // namespace

int main(int argc, char** argv) {
	std::vector<char*> args; // without --amulet ID
	for (int i = 0; i < argc; i++)
		if (std::strcmp(argv[i], "--amulet") == 0 && i + 1 < argc) {
			wornAmulet = itemFromFile(ItemType::AMULET, std::atoi(argv[++i]));
			if (!wornAmulet)
				return usage();
		} else
			args.push_back(argv[i]);
	argc = static_cast<int>(args.size());
	argv = args.data();
	if (argc < 2)
		return usage();
	const std::string mode = argv[1];
	if (mode == "today") {
		todayCurve(true);
		return 0;
	}
	if (mode == "curve") {
		buildCurve(true, 1.0);
		buildCurve(true, endScale());
		return 0;
	}
	if (mode == "tune") {
		const bool play = argc < 4 || std::strcmp(argv[3], "gen") != 0;
		return tune(play);
	}
	if (mode == "table") {
		const bool play = argc >= 4 && std::strcmp(argv[2], "--gear") == 0 && std::strcmp(argv[3], "play") == 0;
		return table(play);
	}
	const bool home = mode == "home" && argc >= 3;
	if ((mode == "fight" && argc >= 4) || home) {
		const MonsterKind* kind = kindByLabel(argv[2]);
		if (kind == nullptr) {
			std::fprintf(stderr, "no monster labelled \"%s\"\n", argv[2]);
			return 2;
		}
		Stats s;
		if (home)
			s = homeStats(*kind, true);
		else {
			s.level = std::atoi(argv[3]);
			s.difficulty = 10;
		}
		double hp = 1.0, damage = 1.0;
		for (int i = home ? 3 : 4; i + 1 < argc; i += 2) {
			const int value = std::atoi(argv[i + 1]);
			if (std::strcmp(argv[i], "--monster-hp") == 0)
				hp = static_cast<double>(value) / kind->maxHealth;
			else if (std::strcmp(argv[i], "--monster-damage") == 0)
				damage = static_cast<double>(value) / kind->damage;
			else if (std::strcmp(argv[i], "--minions") == 0) { // a boss: at most this many minions alive
				auto& types = const_cast<MonsterTypes&>(
					SimWorld::monsterTypes()
				); // NOLINT(cppcoreguidelines-pro-type-const-cast): the tool's own copy
				BossRules& boss = types[static_cast<size_t>(kind->id)].boss;
				boss.maxAlive = value;
				boss.minAlive = std::min(boss.minAlive, value);
			} else if (std::strcmp(argv[i], "--armor") == 0)
				s.armor = value;
			else if (std::strcmp(argv[i], "--might") == 0)
				s.might = value;
			else if (std::strcmp(argv[i], "--hp") == 0)
				s.hpPercent = value;
			else if (std::strcmp(argv[i], "--difficulty") == 0)
				s.difficulty = value;
			else
				return usage();
		}
		scaleKind(kind->id, hp, damage);
		printHeader();
		printRow("-", *kind, s);
		return 0;
	}
	return usage();
}
