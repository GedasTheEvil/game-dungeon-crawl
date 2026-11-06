#include "level_gen.h"
#include "level_builder.h"
#include "inscriptions.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace levelgen {
bool LevelBuilder::build(LevelGrid& out) {
	if (!(zoneTarget > 0 ? buildHalls() : buildMainRoute()))
		return false;
	if (boss != 0) { // carved first: no pool digs into it
		bossRoom.row = BOSS_ROOM_ROW;
		bossRoom.height = bossRoomHeight(boss);
		bossRoom.x0 = rng.range(MIN_COL + 1, MAX_COL - BOSS_ROOM_LENGTH);
		bossRoom.x1 = bossRoom.x0 + BOSS_ROOM_LENGTH - 1;
		carveSegment(bossRoom);
	}
	if (zoneTarget > 0) {
		addWater();
		addSand();
		chooseDark();
	}
	placeLocks();
	if (boss != 0 && !placeBoss())
		return false;
	addGauntlets();
	addGateRooms();
	if (zoneTarget > 0) {
		placeHapi();
		if (forkSeg > 0)
			placeForkContent();
		addTexts();
	}
	int branches = rng.range(1, 2) + d / 4;
	for (int i = 0; i < branches * 3 && branches > 0; i++) {
		int s = rng.range(0, static_cast<int>(route.size()) - 1);
		if (tryBranch(s, route[s].x0, route[s].x1, Tile{Treasure, 0, 0}))
			branches--;
	}
	fillWings();
	for (Segment& s : route)
		fillSegment(s);
	if (zoneTarget > 0) {
		placeSupplies();
		dressFigures();
		addLight();
		dressRooms();
	}
	if (content.finale && shape.bossRoom) { // no exit: a rare chest where it stood
		const Segment& last = route.back();
		g.set(last.exit, last.row, rareChest());
	}
	out = g;
	return true;
}
} // namespace levelgen

using namespace levelgen;

float targetScore(int difficulty, int openCells) {
	float score = 2.5f + 2.2f * static_cast<float>(difficulty);
	return openCells > 0 ? score * static_cast<float>(openCells) / static_cast<float>(ZONE_SCORE_CELLS) : score;
}

GenResult generateLevel(const GenOptions& options) {
	GenResult best;
	int difficulty = std::clamp(options.difficulty, GEN_MIN_DIFFICULTY, GEN_MAX_DIFFICULTY);
	const bool zone = isZone(options.zone);
	const int openTarget = !zone ? 0 : options.openCells > 0 ? options.openCells : ZONE_OPEN_CELLS;
	best.target = targetScore(difficulty, openTarget);
	const int boss = zone && options.boss ? zoneBoss(static_cast<ZoneId>(options.zone)) : 0;
	float bestError = 0.f;
	for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
		best.attempts = attempt + 1;
		LevelBuilder builder(
			options.seed * 7919U + static_cast<uint32_t>(attempt) * 104729U,
			difficulty,
			openTarget,
			options.zone,
			boss,
			options
		);
		LevelGrid grid;
		if (!builder.build(grid))
			continue;
		LevelReport report = checkLevel(grid);
		if (!report.valid || !report.warnings.empty())
			continue;
		float error = std::fabs(report.difficulty - best.target) / best.target;
		if (zone)
			error += std::fabs(static_cast<float>(report.openCells - openTarget)) / static_cast<float>(openTarget);
		// What was asked for and did not fit: a candidate with it wins over any without.
		const GenShape& shape = builder.Shape();
		error += (options.fork > 0 && shape.forks == 0 ? 10.f : 0.f) +
				 (options.stargate && shape.nameGates == 0 ? 10.f : 0.f) +
				 (isExpeditionPage(options.page) && shape.pages == 0 ? 10.f : 0.f);
		if (!best.ok || error < bestError) {
			best.ok = true;
			best.grid = grid;
			best.report = report;
			best.shape = builder.Shape();
			bestError = error;
		}
		if (bestError <= GOOD_ENOUGH)
			break;
	}
	if (zone) {
		best.grid.meta.zone = options.zone;
		best.grid.meta.title = options.title;
	}
	return best;
}

bool parseGenSpec(const std::string& spec, GenOptions& options) {
	unsigned int seed = 0;
	int difficulty = 0;
	int read = 0;
	if (sscanf(spec.c_str(), "gen:%u:%d%n", &seed, &difficulty, &read) != 2)
		return false;
	GenOptions parsed;
	parsed.seed = seed;
	parsed.difficulty = difficulty;
	const std::string rest = spec.substr(static_cast<size_t>(read));
	if (!rest.empty()) {
		std::string slug = rest.substr(1);
		const size_t colon = slug.find(':');
		if (colon != std::string::npos) {
			if (slug.substr(colon + 1) != "boss")
				return false;
			parsed.boss = true;
			slug.resize(colon);
		}
		parsed.zone = rest[0] == ':' ? zoneBySlug(slug) : NO_ZONE;
		if (parsed.zone == NO_ZONE || (parsed.boss && zoneBoss(static_cast<ZoneId>(parsed.zone)) == 0))
			return false;
	}
	options = parsed;
	return true;
}
