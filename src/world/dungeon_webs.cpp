// The Mother of Webs on the level (plan camel-spider-boss): she puts out the torches and lamps she passes and spins
// webs over the floor. A web slows the player standing in it; a blade's swing cuts it, fire burns it. Drawn in
// dungeon_render_webs.cpp (the webs) and dungeon_render_decor.cpp (a flame dying down).
#include "dungeon.h"
#include "decor.h"
#include "../entities/player.h"
#include "journal.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cmath>

void Dungeon::updateSpinner(const Monster& mon) {
	const MonsterType& type = *mon.Type();
	const int row = mon.Row();
	const auto col = static_cast<int>(std::floor(mon.CentreX()));
	const int now = GameClock::now();
	const auto left = static_cast<int>(std::floor(mon.Left())), right = static_cast<int>(std::floor(mon.Right()));
	for (int c = left - type.douseCells; type.douseCells > 0 && c <= right + type.douseCells; c++) {
		if (!IsInBounds(c, row))
			continue;
		const int cell = MapIndex(c, row);
		if (decoration.doused[cell] || (!decoration.torch[cell] && decoration.decor[cell].type != DECOR_LAMP))
			continue;
		decoration.doused[cell] = true;
		snuffs.push_back({cell, now});
		sim.events->Play(WorldSound::FireHiss);
		sim.journal->SeeMove(type.id, levelNumber, CreatureMove::Douse);
	}
	if (!type.webs || mon.jumping() || now < nextWebMs || !webFloor(col, row) || webAt(MapIndex(col, row)) >= 0)
		return;
	if (std::count_if(webs.begin(), webs.end(), [](const Web& w) { return w.goneMs < 0; }) >= type.webs->max)
		return;
	webs.push_back({MapIndex(col, row), now});
	nextWebMs = now + type.webs->everyMs;
	sim.journal->SeeMove(type.id, levelNumber, CreatureMove::SpinWeb);
}
//======================================================================================
bool Dungeon::webFloor(int col, int row) const {
	if (!IsInBounds(col, row) || !IsInBounds(col, row - 1))
		return false;
	const Tile t = MapAt(col, row);
	return (t.type == NoObject || t.type == MonsterSpawn) && !isSolidTile(t) && !inHalfWater(t) &&
		   isSolidTile(MapAt(col, row - 1));
}
//======================================================================================
int Dungeon::webAt(int cell) const {
	for (size_t k = 0; k < webs.size(); k++)
		if (webs[k].cell == cell && webs[k].goneMs < 0)
			return static_cast<int>(k);
	return -1;
}
//======================================================================================
bool Dungeon::PlaceWeb(int col, int row) {
	if (!webFloor(col, row) || webAt(MapIndex(col, row)) >= 0)
		return false;
	webs.push_back({MapIndex(col, row), GameClock::now()});
	return true;
}
//======================================================================================
bool Dungeon::PlayerInWeb() const {
	const JumpState& jump = sim.player->jump;
	const auto col = static_cast<int>(std::floor(mapX)), row = static_cast<int>(std::floor(mapY));
	return !jump.jumping && !jump.falling && WebAt(col, row);
}
//======================================================================================
void Dungeon::burnWeb(int cell) {
	if (const int k = webAt(cell); k >= 0) {
		webs[static_cast<size_t>(k)].goneMs = GameClock::now();
		webs[static_cast<size_t>(k)].burnt = true;
	}
}
//======================================================================================
int Dungeon::cutWebs(float fromX, float toX, int row) {
	const auto c0 = static_cast<int>(std::floor(std::min(fromX, toX)));
	const auto c1 = static_cast<int>(std::floor(std::max(fromX, toX)));
	int cut = 0;
	for (Web& w : webs)
		if (w.goneMs < 0 && w.cell / MAP_WIDTH == row && w.cell % MAP_WIDTH >= c0 && w.cell % MAP_WIDTH <= c1) {
			w.goneMs = GameClock::now();
			cut++;
		}
	return cut;
}
//======================================================================================
void Dungeon::updateWebs() {
	const int now = GameClock::now();
	webs.erase(std::remove_if(webs.begin(), webs.end(),
							  [now](const Web& w) {
								  return w.goneMs >= 0 && now - w.goneMs >= (w.burnt ? WEB_BURN_MS : WEB_CUT_MS);
							  }),
			   webs.end());
	snuffs.erase(
		std::remove_if(snuffs.begin(), snuffs.end(), [now](const Motion& m) { return now - m.startMs >= SNUFF_MS; }),
		snuffs.end());
	for (Web& w : webs) {
		const int col = w.cell % MAP_WIDTH, row = w.cell / MAP_WIDTH;
		if (w.goneMs < 0 && inFireJet(static_cast<float>(col), static_cast<float>(col) + 1.f, row)) {
			w.goneMs = now;
			w.burnt = true;
		}
	}
	const bool in = PlayerInWeb() && sim.player->Alive();
	if (in && !playerWebbed)
		sim.events->Status("%s", "Your feet stick in the web");
	playerWebbed = in;
}
