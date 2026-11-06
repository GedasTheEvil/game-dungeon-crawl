// The Wisdom of Seshat and Disarm trap skills (plan heka-surplus): the hidden and secret rooms on the draft map, the
// trapped containers' and mimics' red contour near the player (drawn in dungeon_render_outlines.cpp); the riddle hint
// and the royal name are in ui/riddle.cpp and dungeon_interact.cpp.
#include "dungeon.h"
#include "player.h"
#include <algorithm>
#include <cmath>

namespace {
// A pocket bigger than this is part of the level, not a sealed room.
constexpr int SECRET_ROOM_MAX_CELLS = 80;
} // namespace

std::vector<std::vector<int>> secretRooms(const Tile* cells) {
	std::vector<std::vector<int>> rooms;
	std::vector<char> seen(LEVEL_CELL_COUNT, 0);
	for (int start = 0; start < LEVEL_CELL_COUNT; start++) {
		if (!isTeleporter(cells[start]) || seen[start] != 0)
			continue;
		// The open cells joined to the teleporter's (walls only bound it: a sealed room has no other way in).
		std::vector<int> room{start};
		seen[start] = 1;
		bool open = false; // the entrance or the exit in it, or too big: the level itself
		for (size_t k = 0; k < room.size() && !open; k++) {
			const int col = room[k] % LEVEL_WIDTH, row = room[k] / LEVEL_WIDTH;
			const Tile& t = cells[room[k]];
			open = t.type == Door && (t.attr == GateEntrance || t.attr == GateExit);
			for (const auto [dc, dr] : {std::pair{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
				const int c = col + dc, r = row + dr;
				if (c < 0 || c >= LEVEL_WIDTH || r < 0 || r >= LEVEL_HEIGHT)
					continue;
				const int next = r * LEVEL_WIDTH + c;
				if (seen[next] == 0 && !isWall(cells[next])) {
					seen[next] = 1;
					room.push_back(next);
				}
			}
			open = open || static_cast<int>(room.size()) > SECRET_ROOM_MAX_CELLS;
		}
		if (!open)
			rooms.push_back(std::move(room));
	}
	return rooms;
}
//======================================================================================
void Dungeon::findSecretRooms() { secrets = secretRooms(map); }
//======================================================================================
// With the Wisdom of Seshat, a room within SESHAT_ROOM_RADIUS cells of the player goes on the draft map whole, its
// walls round it too.
void Dungeon::revealSecretRooms() {
	if (sim.player == nullptr || !sim.player->stats.heka.Has(SkillId::WisdomOfSeshat))
		return;
	const int col = static_cast<int>(mapX), row = static_cast<int>(mapY);
	for (const std::vector<int>& room : secrets) {
		const bool near = std::ranges::any_of(room, [col, row](int cell) {
			return std::abs(cell % MAP_WIDTH - col) <= SESHAT_ROOM_RADIUS &&
				   std::abs(cell / MAP_WIDTH - row) <= SESHAT_ROOM_RADIUS;
		});
		if (!near)
			continue;
		for (int cell : room)
			for (int dr = -1; dr <= 1; dr++)
				for (int dc = -1; dc <= 1; dc++)
					if (IsInBounds(cell % MAP_WIDTH + dc, cell / MAP_WIDTH + dr))
						explored[MapIndex(cell % MAP_WIDTH + dc, cell / MAP_WIDTH + dr)] = true;
	}
}
//======================================================================================
float Dungeon::outlineNear(float col, float row, float far) const {
	const float dx = mapX - col;
	const float dy = mapY - row;
	const float d = std::sqrt(dx * dx + dy * dy);
	return std::clamp((far - d) / OUTLINE_FADE, 0.f, 1.f);
}
//======================================================================================
float Dungeon::containerOutlineAt(int cell) const {
	if (sim.player == nullptr || !sim.player->stats.heka.Has(SkillId::DisarmTrap) ||
		map[cell].type != TrappedTreasure || containers[cell].opened)
		return 0.f;
	const int col = cell % MAP_WIDTH, row = cell / MAP_WIDTH;
	return outlineNear(static_cast<float>(col) + 0.5f, static_cast<float>(row), DISARM_SEEN_FAR);
}
//======================================================================================
float Dungeon::mimicOutline(const Monster& mon) const {
	if (sim.player == nullptr || !sim.player->stats.heka.Has(SkillId::DisarmTrap) || !mon.Active() || !mon.Alive() ||
		!ambushes(mon.Type()->locomotion) || mon.Alerted())
		return 0.f;
	return outlineNear(mon.CentreX(), mon.BottomY(), DISARM_SEEN_FAR);
}
