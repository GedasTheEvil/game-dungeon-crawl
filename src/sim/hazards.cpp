// What lies or hangs on the level for a while (plan dungeon-subsystems): the parts that need no level.
#include "hazards.h"
#include "../level/tuning/gameplay_monster_kinds.h"
#include "../level/tuning/gameplay_environment.h"
#include "../level/tuning/gameplay_traps.h"
#include <algorithm>

void Hazards::Clear() {
	chestGas.clear();
	webs.clear();
	snuffs.clear();
	blazes.clear();
	holes.clear();
	sprays.clear();
	puffs.clear();
	pours.clear();
	landings.clear();
}
void Hazards::ExpireMarks(int now) {
	const auto expire = [now](auto& list, int ms) {
		std::erase_if(list, [now, ms](const auto& e) { return now - e.startMs >= ms; });
	};
	expire(holes, BURROW_HOLE_MS);
	expire(sprays, BURROW_SPRAY_MS);
	expire(puffs, SAND_PUFF_MS);
	expire(landings, LANDING_PUFF_MS);
	expire(pours, SAND_POUR_MS + SAND_POUR_TAIL_MS);
}
int Hazards::WebAt(int cell) const {
	for (size_t k = 0; k < webs.size(); k++)
		if (webs[k].cell == cell && webs[k].goneMs < 0)
			return static_cast<int>(k);
	return -1;
}
bool Hazards::Blazing(int cell) const {
	return std::ranges::any_of(blazes, [cell](const Blaze& b) { return b.cell == cell; });
}
