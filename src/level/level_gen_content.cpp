#include "level_builder.h"
#include "inscriptions.h"
#include "level_check.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace levelgen {
// A rare chest for a secret room or a tomb: an amulet of the depth's tier, or a rare potion.
Tile LevelBuilder::rareChest() {
	constexpr ItemKind AMULETS[] = {
		ItemKind::StrengthLesser,
		ItemKind::ArmorLesser,
		ItemKind::HealthLesser,
		ItemKind::PoisonWardLesser,
		ItemKind::TrapWardLesser,
		ItemKind::BluntWardLesser,
		ItemKind::SlashWardLesser,
		ItemKind::PierceWardLesser,
		ItemKind::VenomLesser,
		ItemKind::FireWardLesser,
		ItemKind::CrushingLesser,
		ItemKind::CleavingLesser,
		ItemKind::PiercingLesser
	};
	constexpr ItemKind POTIONS[] =
		{ItemKind::LargeHealth, ItemKind::Life, ItemKind::Might, ItemKind::Armor, ItemKind::GreaterResistance};
	if (rng.chance(0.5f)) {
		const ItemKind lesser = AMULETS[rng.range(0, static_cast<int>(std::size(AMULETS)) - 1)];
		return chestOf(itemAt(itemIndex(lesser) + std::min(3, (d - 1) / 3))); // lesser, minor, normal, grand
	}
	return chestOf(POTIONS[rng.range(0, static_cast<int>(std::size(POTIONS)) - 1)]);
}

// The fork's content: the warning between the drops on the last fork (FORK_TEXT) and the expedition page on the
// way to it, on the floor; traps on every other cell of the right route, at its end the remains of the earlier
// expedition (a skeleton) and their gear (a weapon of the depth or a rare chest); the left route gets two monsters
// more.
void LevelBuilder::placeForkContent() {
	const Segment& a = route[static_cast<size_t>(forkSeg - 1)];
	const int between = a.exit + 2; // under it, the pillar between the two routes
	if (content.fork >= 2 && freeCell(between, a.row))
		put(between, a.row, Tile{Inscription, FORK_TEXT, 0});
	if (isExpeditionPage(content.page)) {
		const int dir = a.exit > a.entry ? 1 : -1;
		for (int k = 3; k <= 9; k++) {
			const int x = a.exit - k * dir;
			if ((x - a.entry) * dir > 1 && busy[index(x, a.row)] == 0 && isEmptyCell(g.at(x, a.row)) &&
				isWall(g.at(x, a.row - 1))) {
				put(x, a.row, Tile{Page, content.page, 0});
				busy[index(x, a.row)] = 1;
				shape.pages++;
				break;
			}
		}
	}
	const Segment& right = forkRight;
	constexpr Feature KINDS[] = {Feature::Spike, Feature::RockFall, Feature::Dart, Feature::FireVent};
	for (int x = right.entry + 1; x < right.exit; x += 2)
		for (int k = 0; k < 4; k++) {
			const Feature kind = KINDS[(x + k) % 4];
			if (trapFits(right, x, kind)) {
				put(x, right.row, trapTile(kind));
				busy[index(x, right.row)] = 1;
				break;
			}
		}
	tombRooms.push_back(right);
	const int end = right.x1;
	put(end, right.row, rng.chance(0.5f) ? chestOf(randomWeapon(true)) : rareChest());
	busy[index(end, right.row)] = 1;
	Tile bones = g.at(end - 1, right.row);
	if (freeCell(end - 1, right.row) && !hasPlacedProp(bones.dressing)) {
		bones.dressing.prop = DECOR_SKELETON;
		g.set(end - 1, right.row, bones);
		busy[index(end - 1, right.row)] = 1;
	}
	const Segment& left = route[static_cast<size_t>(forkSeg)];
	for (int n = 0, tries = 0; n < 2 && tries < 30; tries++)
		n += placeMonster(rng.range(left.x0 + 1, left.x1 - 1), left.row, left.dark) ? 1 : 0;
}

// The wings no key or lever took: treasure at the end, now and then; hazards and monsters along all of them.
void LevelBuilder::fillWings() {
	for (Wing& wing : wings) {
		if (!wing.taken && rng.chance(WING_TREASURE)) {
			put(wing.end, wing.itemRow, randomTreasure());
			busy[index(wing.end, wing.itemRow)] = 1;
		}
		fillRun(route[static_cast<size_t>(wing.seg)], wing.inner, wing.fillEnd);
	}
}

// A plain potion's chest (health, stamina, might, armour): one a trapped chest or a resistance potion may take.
bool LevelBuilder::plainPotionChest(const Tile& t) const {
	if (t.type != Treasure)
		return false;
	const std::optional<ItemKind> kind = itemFromFile(t.attr, t.value);
	return kind == ItemKind::SmallHealth || kind == ItemKind::LargeHealth || kind == ItemKind::SmallStamina ||
		   kind == ItemKind::LargeStamina || kind == ItemKind::Might || kind == ItemKind::Armor;
}

// Treasure and supplies by the level's dangers (plan campaign-levelgen, step 7), once everything else is placed.
// With a poisoner, a dart plate or a poison drip: an antidote in a wing on the way before the first of them, a
// lesser resistance potion, a greater one too from GREATER_RESISTANCE_FROM on. Then the zone's trapped chests
// (ZONE_TRAPPED_CHESTS), each a chest that held a plain potion: never what the level needs.
void LevelBuilder::placeSupplies() {
	int firstPoison = -1; // the first segment of the route with poison on its rows
	bool poison = false;
	for (int cell = 0; cell < CELLS; cell++) {
		const Tile& t = g.cells[cell];
		poison = poison || (t.type == MonsterSpawn && isPoisoner(t.attr)) || t.type == DartPlate ||
				 t.fixture.type == PoisonDrip;
	}
	for (int si = 0; si < static_cast<int>(route.size()) && poison && firstPoison < 0; si++) {
		const Segment& hall = route[static_cast<size_t>(si)];
		for (int y = hall.row; y < hall.row + hall.height && firstPoison < 0; y++)
			for (int x = hall.x0; x <= hall.x1; x++) {
				const Tile t = g.at(x, y);
				if ((t.type == MonsterSpawn && isPoisoner(t.attr)) || t.type == DartPlate ||
					t.fixture.type == PoisonDrip) {
					firstPoison = si;
					break;
				}
			}
	}
	if (poison) {
		placeAntidote(firstPoison < 0 ? static_cast<int>(route.size()) - 1 : firstPoison);
		int resistance = zoneId >= static_cast<int>(GREATER_RESISTANCE_FROM) ? 2 : 1;
		for (ItemKind kind : {ItemKind::LesserResistance, ItemKind::GreaterResistance})
			if (resistance-- > 0 && swapPlainChest(chestOf(kind)))
				shape.resistancePotions++;
	}
	for (int n = isZone(zoneId) ? ZONE_TRAPPED_CHESTS[zoneId] : 0; n > 0; n--) {
		std::vector<int> plain;
		for (int cell = 0; cell < CELLS; cell++)
			if (plainPotionChest(g.cells[cell]))
				plain.push_back(cell);
		if (plain.empty())
			break;
		g.cells[plain[static_cast<size_t>(rng.range(0, static_cast<int>(plain.size()) - 1))]].type = TrappedTreasure;
		shape.trappedChests++;
	}
}

// One of the level's plain potion chests, by chance, now holds chest's item instead; false if there is none.
bool LevelBuilder::swapPlainChest(const Tile& chest) {
	std::vector<int> plain;
	for (int cell = 0; cell < CELLS; cell++)
		if (plainPotionChest(g.cells[cell]))
			plain.push_back(cell);
	if (plain.empty())
		return false;
	const int cell = plain[static_cast<size_t>(rng.range(0, static_cast<int>(plain.size()) - 1))];
	put(cell % LEVEL_WIDTH, cell / LEVEL_WIDTH, chest);
	return true;
}

// The antidote: in the chest (or else at the free end) of the wing nearest before the segment the poison starts
// on, else of any wing, else in the first chest on the route.
void LevelBuilder::placeAntidote(int poisonSeg) {
	auto chest = [this](const Wing& w) { return !w.taken && g.at(w.end, w.itemRow).type == Treasure; };
	auto open = [this](const Wing& w) { return !w.taken && freeCell(w.end, w.itemRow); };
	const Wing* spot = nullptr;
	for (int pass = 0; pass < 2 && spot == nullptr; pass++)
		for (const Wing& w : wings)
			if ((pass == 0 ? chest(w) : open(w)) && w.seg <= poisonSeg && (spot == nullptr || w.seg > spot->seg))
				spot = &w;
	if (spot == nullptr) {
		auto any = std::find_if(wings.begin(), wings.end(), chest);
		if (any == wings.end())
			any = std::find_if(wings.begin(), wings.end(), open);
		spot = any == wings.end() ? nullptr : &*any;
	}
	if (spot != nullptr) {
		put(spot->end, spot->itemRow, chestOf(ItemKind::Antidote));
		return;
	}
	for (const Segment& s : route)
		for (int x = s.x0; x <= s.x1; x++)
			if (g.at(x, s.row).type == Treasure) {
				put(x, s.row, chestOf(ItemKind::Antidote));
				return;
			}
}

// The Blessing of Hapi (plan water-blessing-potion): a chest a few cells before the first half water the route
// wades through, on dry floor of its hall or, if there is none, of the hall before it.
void LevelBuilder::placeHapi() {
	for (size_t si = 0; si < route.size(); si++) {
		const Segment& hall = route[si];
		const int dir = hall.exit >= hall.entry ? 1 : -1;
		int wet = -1;
		for (int x = hall.entry; x != hall.exit + dir && wet < 0; x += dir)
			wet = inHalfWater(g.at(x, hall.row)) ? x : -1;
		if (wet < 0)
			continue;
		auto fits = [this](const Segment& s, int x) {
			return x >= s.x0 && x <= s.x1 && busy[index(x, s.row)] == 0 && isEmptyCell(g.at(x, s.row)) &&
				   isWall(g.at(x, s.row - 1));
		};
		for (int k = 2; k <= 6; k++)
			if (fits(hall, wet - k * dir)) {
				putHapi(wet - k * dir, hall.row);
				return;
			}
		if (si == 0)
			return;
		const Segment& before = route[si - 1];
		const int back = before.exit >= before.entry ? -1 : 1;
		for (int k = 2; k <= 6; k++)
			if (fits(before, before.exit + k * back)) {
				putHapi(before.exit + k * back, before.row);
				return;
			}
		return;
	}
}

void LevelBuilder::putHapi(int col, int row) {
	put(col, row, chestOf(ItemKind::WaterBlessing));
	busy[index(col, row)] = 1;
	shape.hapiPotions++;
}

// A treasure chest, or now and then a mimic in its place.
Tile LevelBuilder::randomTreasure() {
	Tile chest = randomChest();
	if (fauna != nullptr) {
		if (!mimicRng.percent(fauna->mimicPercent))
			return chest;
		if (mimicRng.percent(fauna->cursedPercent))
			return Tile{MonsterSpawn, MonsterCursedMimic, 0};
		// The tombs' mimic jars (plan tomb-curse-containers): rolled only where there are some, so the other
		// zones' mimics come out as before.
		const bool jar = fauna->jarPercent > 0 && mimicRng.percent(fauna->jarPercent);
		return Tile{MonsterSpawn, jar ? MonsterMimicJar : MonsterMimic, 0};
	}
	if (d >= MIMIC_MIN_DIFFICULTY && mimicRng.chance(MIMIC_CHANCE)) {
		const bool cursed = d >= CURSED_MIMIC_MIN_DIFFICULTY && mimicRng.chance(CURSED_MIMIC_SHARE);
		const bool jar = !cursed && d >= MIMIC_JAR_MIN_DIFFICULTY && mimicRng.chance(MIMIC_JAR_SHARE);
		return Tile{MonsterSpawn, cursed ? MonsterCursedMimic : jar ? MonsterMimicJar : MonsterMimic, 0};
	}
	return chest;
}

Tile LevelBuilder::randomChest() {
	int roll = rng.range(0, 99);
	if (roll < 62) { // a potion by its weight (PotionDef::generatedWeight)
		int pick = rng.range(0, 99);
		int i = WEAPON_KIND_COUNT;
		while (i < FIRST_AMULET - 1 && pick >= potionDef(itemAt(i)).generatedWeight) {
			pick -= potionDef(itemAt(i)).generatedWeight;
			i++;
		}
		return chestOf(itemAt(i));
	}
	// The weapons come in by depth, like in the campaign (WeaponDef::generatedFrom): the deeper, the more kinds.
	return chestOf(randomWeapon(roll < 88));
}

ItemKind LevelBuilder::randomWeapon(bool melee) {
	auto allowed = [this, melee](int i) {
		const ItemKind kind = itemAt(i);
		return isRanged(kind) != melee && weaponDef(kind).generatedFrom <= d;
	};
	int kinds = 0;
	for (int i = 0; i < WEAPON_KIND_COUNT; i++)
		kinds += allowed(i) ? 1 : 0;
	int pick = rng.range(0, kinds - 1);
	for (int i = 0; i < WEAPON_KIND_COUNT; i++)
		if (allowed(i) && pick-- == 0)
			return itemAt(i);
	return ItemKind::Club;
}

Tile LevelBuilder::chestOf(ItemKind kind) {
	const ItemFileId file = fileIdOf(kind);
	return Tile{Treasure, file.type, file.id};
}

// Weak monsters give way to their giant kin deeper down (MonsterKind::generated).
int LevelBuilder::randomMonster() {
	auto allowed = [this](const GenPick& p) { return p.weight > 0 && d >= p.minDifficulty && d <= p.maxDifficulty; };
	int total = 0;
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
		if (const GenPick& p = monsterKind(type)->generated; allowed(p))
			total += p.weight;
	int roll = rng.range(0, total - 1);
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++) {
		const GenPick& p = monsterKind(type)->generated;
		if (!allowed(p))
			continue;
		if (roll < p.weight)
			return type;
		roll -= p.weight;
	}
	return MonsterRat;
}

// A zone level's monster for the cell, from the zone's set (ZoneFauna): swimmers and lotuses only in half water and
// likelier there, mummies and living ushabti (their coffin, their fellow figures) never in it, sand burrowers
// likelier in sand, light shunners likelier in a dark hall and rarer in a lit one. 0 if none of the set fits.
int LevelBuilder::zoneMonster(int col, int row, bool dark) {
	const Tile t = g.at(col, row);
	int weights[ZONE_MONSTER_MAX] = {};
	int total = 0;
	for (int i = 0; i < ZONE_MONSTER_MAX; i++) {
		const ZoneMonster& m = fauna->monsters[i];
		const MonsterKind* kind = monsterKind(m.type);
		if (kind == nullptr)
			break;
		const bool water = kind->locomotion == Locomotion::Submerged || kind->locomotion == Locomotion::Floating;
		int w = m.weight * 2;
		if (water)
			w = inHalfWater(t) ? w * 4 : 0;
		else if ((m.type == MonsterMummy || m.type == MonsterUshabti) && inHalfWater(t))
			w = 0; // a coffin, a figure: on dry floor

		if (kind->sandBurrow.has_value() && inSand(t))
			w *= 3;
		// Never in a hostile cell (plan terrain-affinity): water, sand, a fire vent. The light comes later (addLight
		// keeps clear of them).
		if (hostileCell(col, row, m.type))
			w = 0;
		if (kind->shunsLight && terrain.dark != TerrainAmount::None)
			w = dark ? w * 3 : w / 4;
		weights[i] = w;
		total += w;
	}
	if (total == 0)
		return 0;
	int roll = rng.range(0, total - 1);
	for (int i = 0; i < ZONE_MONSTER_MAX; i++) {
		if (roll < weights[i])
			return fauna->monsters[i].type;
		roll -= weights[i];
	}
	return 0;
}

bool LevelBuilder::placeMonster(int col, int row, bool dark) {
	if (monsters >= monsterCap || !freeCell(col, row) || !isSolidStructure(g.at(col, row - 1).structure))
		return false;
	for (int x = col - MONSTER_GAP; x <= col + MONSTER_GAP; x++)
		if (g.at(x, row).type == MonsterSpawn)
			return false;
	const int type = fauna != nullptr ? zoneMonster(col, row, dark) : randomMonster();
	if (type == 0 || hostileCell(col, row, type))
		return false;
	if (monsterKind(type)->group > 1 && !groupCell(col - 1, row) && !groupCell(col + 1, row))
		return false; // no room for a group
	put(col, row, Tile{MonsterSpawn, type, 0});
	busy[index(col, row)] = 1;
	monsters++;
	placeGroup(col, row, type);
	return true;
}

// The cell is hostile to the type (plan terrain-affinity, spawnTerrains): no spawn there.
bool LevelBuilder::hostileCell(int col, int row, int type) const {
	const MonsterKind& kind = *monsterKind(type);
	return terrainMood(spawnTerrains(g, col, row, kind), kind.native, kind.hostile) == TerrainMood::Hostile;
}

// One of a group may stand there: a free floor cell no other feature uses.
bool LevelBuilder::groupCell(int col, int row) const {
	return col >= 1 && col < LEVEL_WIDTH - 1 && busy[index(col, row)] == 0 && freeCell(col, row) &&
		   isSolidStructure(g.at(col, row - 1).structure);
}

// The rest of a group (MonsterKind::group, the tiny spiders): 1 to group - 1 more of its kind on the free floor
// cells next to it, either side in turn, each side up to the first cell that is not.
void LevelBuilder::placeGroup(int col, int row, int type) {
	const int group = monsterKind(type)->group;
	if (group < 2)
		return;
	int more = rng.range(1, group - 1);
	int next[2] = {col + 1, col - 1};
	bool open[2] = {true, true};
	for (int side = 0; more > 0 && (open[0] || open[1]); side = 1 - side) {
		const int c = next[side];
		open[side] = open[side] && groupCell(c, row) && !hostileCell(c, row, type);
		if (!open[side])
			continue;
		put(c, row, Tile{MonsterSpawn, type, 0});
		busy[index(c, row)] = 1;
		monsters++;
		more--;
		next[side] += side == 0 ? 1 : -1;
	}
}

bool LevelBuilder::floorCell(const Segment& s, int col) const {
	return col >= s.x0 && col <= s.x1 && isEmptyCell(g.at(col, s.row)) && isWall(g.at(col, s.row - 1));
}

// A drift on rock (not quicksand) with nothing in it.
bool LevelBuilder::sandFloor(const Segment& s, int col) const {
	const Tile t = g.at(col, s.row);
	return col >= s.x0 && col <= s.x1 && t.structure == Structure::Sand && freeCell(col, s.row) &&
		   isWall(g.at(col, s.row - 1));
}

// A trap at col the player jumps: from either side, never out of half water.
bool LevelBuilder::jumpable(const Segment& s, int col) const {
	return !inHalfWater(g.at(col - 1, s.row)) && !inHalfWater(g.at(col + 1, s.row));
}

// A one-cell hole in the floor with a death trap in it: jump it or die.
bool LevelBuilder::placePit(const Segment& s, int col) {
	if (!floorCell(s, col - 1) || !floorCell(s, col) || !floorCell(s, col + 1))
		return false;
	int below = s.row - 1;
	if (used[index(col, below - 1)] != 0 || !isWall(g.at(col, below - 1)) || !isWall(g.at(col - 1, below)) ||
		!isWall(g.at(col + 1, below)))
		return false;
	carve(col, below, Tile{Death, 0, 0});
	busy[index(col - 1, s.row)] = busy[index(col, s.row)] = busy[index(col + 1, s.row)] = 1;
	return true;
}

Feature LevelBuilder::pickFeature(const float (&weights)[FEATURE_COUNT]) {
	float total = 0.f;
	for (float w : weights)
		total += w;
	float roll = static_cast<float>(rng.range(0, 9999)) / 10000.f * total;
	for (int i = 0; i < FEATURE_COUNT; i++) {
		roll -= weights[i];
		if (roll < 0)
			return static_cast<Feature>(i);
	}
	return Feature::Treasure;
}

// Hazards, monsters and loot along the part of the segment the player has to cross.
void LevelBuilder::fillSegment(const Segment& s) { fillRun(s, s.entry, s.exit >= 0 ? s.exit : farEnd(s, s.entry)); }

float LevelBuilder::share(int percent) { return static_cast<float>(percent) / 100.f; }

// A cell of s's floor a trap of kind can go in: free, dry or a drift (no quicksand), on rock; a rock fall, wall
// spikes and a poison drip under a one-row ceiling; spikes and a dart plate on dry floor, spikes never next to half
// water (they are jumped).
bool LevelBuilder::trapFits(const Segment& s, int col, Feature kind) const {
	const Tile t = g.at(col, s.row);
	if (col < s.x0 || col > s.x1 || busy[index(col, s.row)] != 0 || !freeCell(col, s.row) || inHalfWater(t) ||
		inQuicksand(t) || !isWall(g.at(col, s.row - 1)))
		return false;
	const bool ceiling = isWall(g.at(col, s.row + 1));
	switch (kind) {
	case Feature::Spike:
		return t.structure == Structure::Empty && jumpable(s, col);
	case Feature::Dart:
		return t.structure == Structure::Empty;
	case Feature::RockFall:
	case Feature::WallSpikes:
	case Feature::Drip:
		return ceiling;
	case Feature::FireVent:
		return true;
	default:
		return false;
	}
}

// A rhythm trap (plan rhythm-traps) of type: a phase of its own, now and then slow, or fast deeper down.
Tile LevelBuilder::rhythmTrap(int type) {
	const int roll = rng.range(0, 99);
	const SpikeSpeed speed = d >= FAST_TRAP_MIN_DIFFICULTY && roll < 30 ? SpikeSpeed::Fast
							 : roll >= 80								? SpikeSpeed::Slow
																		: SpikeSpeed::Normal;
	return Tile{type, static_cast<int>(speed), rng.range(0, SPIKE_PHASES - 1)};
}

Tile LevelBuilder::trapTile(Feature kind) {
	switch (kind) {
	case Feature::Spike:
		return Tile{Spike, 0, 0};
	case Feature::RockFall:
		return Tile{RockFall, terrain.sand == TerrainAmount::Much && rng.chance(SAND_FALL_SHARE) ? SAND_FALL : 0, 0};
	case Feature::Dart:
		return Tile{DartPlate, 0, 0};
	case Feature::WallSpikes:
		return rhythmTrap(WallSpikes);
	case Feature::FireVent:
		return rhythmTrap(FireVent);
	default:
		return rhythmTrap(PoisonDrip);
	}
}

// Trap gauntlets (ZoneTraps::gauntlets): a stretch of a hall the route has to cross (off a pillar loop, clear of
// its ends) with a trap on every other cell, of the zone's kinds by their weights, and a free cell between to wait
// in; from PAIR_MIN_DIFFICULTY now and then two rhythm traps side by side, in step. Before the rest of the
// content, so the stretch is the traps' alone.
void LevelBuilder::addGauntlets() {
	constexpr Feature KINDS[] =
		{Feature::Spike, Feature::RockFall, Feature::Dart, Feature::WallSpikes, Feature::FireVent, Feature::Drip};
	const int weights[] =
		{traps.spikes, traps.rockFalls, traps.darts, traps.wallSpikes, traps.fireVents, traps.poisonDrips};
	for (int n = traps.gauntlets, tries = 0; n > 0 && tries < 30; tries++) {
		const auto s = static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1));
		const Segment& hall = route[s];
		if (hall.height > 1 && tries < 15)
			continue; // a one-row ceiling first: the rock falls, wall spikes and drips need it
		int from = 0, to = 0;
		lockSpan(s, from, to);
		const int dir = to >= from ? 1 : -1;
		const int room = std::abs(to - from) - 1 - 2 * GAUNTLET_CLEAR;
		if (room < GAUNTLET_MIN)
			continue;
		int length = rng.range(GAUNTLET_MIN, std::min(GAUNTLET_MAX, room));
		length -= length % 2 == 0 ? 1 : 0;
		const int start = from + dir * (GAUNTLET_CLEAR + 1 + rng.range(0, room - length));
		bool clear = true;
		for (int k = -1; k <= length && clear; k++) {
			const int x = start + k * dir;
			const Tile t = g.at(x, hall.row);
			clear = busy[index(x, hall.row)] == 0 && freeCell(x, hall.row) && !inHalfWater(t) && !inQuicksand(t) &&
					isWall(g.at(x, hall.row - 1));
		}
		if (!clear)
			continue;
		std::vector<std::pair<int, Feature>> picks;
		for (int k = 0; k < length; k += 2) {
			const int x = start + k * dir;
			int total = 0;
			for (size_t i = 0; i < std::size(KINDS); i++)
				total += trapFits(hall, x, KINDS[i]) ? weights[i] : 0;
			if (total == 0)
				continue;
			int roll = rng.range(0, total - 1);
			for (size_t i = 0; i < std::size(KINDS); i++) {
				if (!trapFits(hall, x, KINDS[i]))
					continue;
				if (roll < weights[i]) {
					picks.emplace_back(x, KINDS[i]);
					break;
				}
				roll -= weights[i];
			}
		}
		if (static_cast<int>(picks.size()) < GAUNTLET_TRAPS)
			continue;
		for (size_t i = 0; i < picks.size(); i++) {
			const auto [x, kind] = picks[i];
			const Tile trap = trapTile(kind);
			put(x, hall.row, trap);
			const bool rhythm = isRhythmTrap(trap.type);
			const int next = x + dir;
			const bool lastPick = i + 1 == picks.size();
			// The pair takes the wait cell after it; the next pick goes, so no run of three forms.
			if (rhythm && d >= PAIR_MIN_DIFFICULTY && (next - start) * dir < length && rng.chance(PAIR_CHANCE) &&
				trapFits(hall, next, kind)) {
				const Tile waitCell = g.at(next, hall.row);
				put(next, hall.row, trap);
				const int left = std::min(x, next);
				if (wallSpikesCrossable(g, left, hall.row, 2, 1) && wallSpikesCrossable(g, left, hall.row, 2, -1)) {
					if (!lastPick && picks[i + 1].first == x + 2 * dir)
						i++;
				} else
					g.set(next, hall.row, waitCell);
			}
		}
		for (int k = -1; k <= length; k++)
			busy[index(start + k * dir, hall.row)] = 1;
		shape.gauntlets++;
		n--;
	}
}

// The same along s's floor between from and to, not on the cells at either end.
void LevelBuilder::fillRun(const Segment& s, int from, int to) {
	int dir = to >= from ? 1 : -1;
	float fd = static_cast<float>(d);
	const float newTrap = 0.04f + 0.01f * fd; // dart plates and each rhythm trap, by the zone's share
	for (int x = from + 2 * dir; (to - x) * dir > 1; x += dir) {
		if (busy[index(x, s.row)] != 0 || !freeCell(x, s.row))
			continue;
		// Weights in Feature order: none, pit, spike, rock fall, monster, treasure, dart plate, wall spikes, fire
		// vent, poison drip.
		float weights[FEATURE_COUNT] = {
			zoneTarget > 0 ? ZONE_CALM : 2.5f,
			(0.2f + 0.08f * fd) * share(traps.pits),
			(0.35f + 0.05f * fd) * share(traps.spikes),
			(s.height == 1 && d >= 2 ? 0.15f + 0.07f * fd : 0.f) * share(traps.rockFalls),
			0.6f + 0.25f * fd,
			0.2f,
			newTrap * share(traps.darts),
			newTrap * share(traps.wallSpikes),
			newTrap * share(traps.fireVents),
			newTrap * share(traps.poisonDrips)
		};
		int size = 1;
		const Feature feature = pickFeature(weights);
		switch (feature) {
		case Feature::None:
			continue;
		case Feature::Pit:
			if (!placePit(s, x + dir))
				continue;
			size = 3;
			break;
		case Feature::Spike:
			// A zone level's halls are many: a pair to walk over would turn up on almost every one.
			size = zoneTarget == 0 && d >= 5 && rng.chance(0.4f) ? 2 : 1;
			for (int k = 0; k < size; k++)
				if (floorCell(s, x + k * dir) && busy[index(x + k * dir, s.row)] == 0 && jumpable(s, x + k * dir))
					put(x + k * dir, s.row, Tile{Spike, 0, 0});
			break;
		case Feature::RockFall:
			size = rng.range(1, std::min(3, 1 + d / 3));
			for (int k = 0; k < size; k++) {
				int cx = x + k * dir;
				if ((floorCell(s, cx) || sandFloor(s, cx)) && busy[index(cx, s.row)] == 0 &&
					isWall(g.at(cx, s.row + 1))) {
					const bool sand = terrain.sand == TerrainAmount::Much && rng.chance(SAND_FALL_SHARE);
					put(cx, s.row, Tile{RockFall, sand ? SAND_FALL : 0, 0});
				}
			}
			break;
		case Feature::Monster:
			if (!placeMonster(x, s.row, s.dark))
				continue;
			break;
		case Feature::Treasure:
			put(x, s.row, randomTreasure());
			break;
		case Feature::Dart:
		case Feature::WallSpikes:
		case Feature::FireVent:
		case Feature::Drip:
			if (!trapFits(s, x, feature))
				continue;
			put(x, s.row, trapTile(feature));
			busy[index(x, s.row)] = 1;
			break;
		}
		x += dir * (size - 1 + rng.range(1, 2)); // a free cell after every feature
	}
}
} // namespace levelgen
