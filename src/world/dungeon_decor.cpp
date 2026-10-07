#include "dungeon.h"
#include "../state/assets.h"
#include "../core/logger.h"

bool Dungeon::bossCoffin(int i, int j) const { return bossCoffinCell(map, i, j); }

int Dungeon::CoffinCount() const {
	int n = 0;
	for (const DecorCell& cell : decoration.decor)
		if (cell.type == DECOR_COFFIN)
			n++;
	return n;
}

bool Dungeon::PlaceDecor(int col, int row, int type) {
	if (!IsInBounds(col, row) || type < 0 || type >= DECOR_COUNT)
		return false;
	decoration.decor[MapIndex(col, row)] = DecorCell{static_cast<int8_t>(type), false, 0.f};
	return true;
}

int Dungeon::DecorTierUsed() const { return decorTierUsed(map, decoration); }

void Dungeon::scatterDecorations(const char* levelName, int depth) {
	const DecorCounts counts = scatterDecor(map, levelName, depth, decoration);
	LOG_INFOF("world", "Decorations in %s: %d", levelName, counts.props);
	LOG_INFOF("world", "Torches: %d", counts.torches);
	LOG_INFOF("world", "Wall decals: %d", counts.decals);
	LOG_INFOF("world", "Ladder shafts: %d", counts.ladderShafts);
	LOG_INFOF("world", "Surfaces: %d painted, %d stone, %d rough cells", counts.painted, counts.stone, counts.rough);
}
