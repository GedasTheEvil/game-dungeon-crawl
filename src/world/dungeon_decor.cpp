#include "dungeon.h"
#include "../core/gameplay_config.h"
#include "../core/logger.h"

bool Dungeon::bossCoffin(int i, int j) const { return bossCoffinCell(map, i, j); }

int Dungeon::CoffinCount() const {
	int n = 0;
	for (const DecorCell& cell : decoration.decor)
		if (cell.type == DECOR_COFFIN || cell.type == DECOR_COFFIN_LARGE)
			n++;
	return n;
}

bool Dungeon::PlaceDecor(int col, int row, int type) {
	if (!IsInBounds(col, row) || type < 0 || type >= DECOR_COUNT)
		return false;
	decoration.decor[MapIndex(col, row)] = DecorCell{static_cast<int8_t>(type), false, 0.f};
	return true;
}

bool Dungeon::Lit(int col, int row) const {
	Flame flames[MAX_CELL_FLAMES];
	for (int c = col - LIGHT_SHUN_CELLS; c <= col + LIGHT_SHUN_CELLS; c++)
		if (IsInBounds(c, row) && (flamesAt(decoration, MapIndex(c, row), flames) > 0 || Sunlit(c, row)))
			return true;
	return false;
}

bool Dungeon::PlaceTorch(int col, int row, bool on) {
	if (!IsInBounds(col, row))
		return false;
	decoration.torch[MapIndex(col, row)] = on;
	return true;
}

int Dungeon::DecorTierUsed() const { return decorTierUsed(map, decoration); }

void Dungeon::scatterDecorations(const char* levelName, int depth) {
	campaignLevel = depth == levelNumber && campaignLevelFile(depth) == levelName;
	const DecorCounts counts = scatterDecor(map, levelName, depth, meta.zone, decoration);
	// An opened container stays where it stood, also after its item was taken (a save): no prop on top of it.
	for (int c = 0; c < MAP_CELL_COUNT; c++)
		if (containers[c].opened && !isCoffin(decoration.decor[c].type))
			decoration.decor[c] = DecorCell{};
	LOG_INFOF("world", "Decorations in %s: %d", levelName, counts.props);
	LOG_INFOF("world", "Torches: %d", counts.torches);
	LOG_INFOF("world", "Wall decals: %d", counts.decals);
	LOG_INFOF("world", "Ladder shafts: %d", counts.ladderShafts);
	LOG_INFOF("world", "Treasure containers: %d", counts.treasures);
	LOG_INFOF("world", "Surfaces: %d painted, %d stone, %d rough cells", counts.painted, counts.stone, counts.rough);
}
