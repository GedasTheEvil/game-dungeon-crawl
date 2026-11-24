// The Mother of Webs on the level (plan camel-spider-boss): she puts out the torches and lamps she passes and spins
// webs over the floor. A web slows the player standing in it; a blade's swing cuts it, fire burns it. Drawn in
// dungeon_render_webs.cpp (the webs) and dungeon_render_decor.cpp (a flame dying down).
#include "dungeon.h"
#include "../level/decor.h"
#include "player.h"
#include "../level/rules/journal.h"
#include "../level/rules/world_events.h"
#include "../level/tuning/gameplay_monster_kinds.h"
#include "../level/tuning/gameplay_shared.h"
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
		hazards.snuffs.push_back({cell, now});
		sim.events->Play(WorldSound::FireHiss);
		sim.journal->SeeMove(type.id, levelNumber, CreatureMove::Douse);
	}
	if (!type.webs || mon.jumping() || now < nextWebMs || !webFloor(col, row) || hazards.WebAt(MapIndex(col, row)) >= 0)
		return;
	if (std::count_if(hazards.webs.begin(), hazards.webs.end(), [](const Web& w) { return w.goneMs < 0; }) >=
		type.webs->max)
		return;
	hazards.webs.push_back({MapIndex(col, row), now});
	nextWebMs = now + type.webs->everyMs;
	sim.journal->SeeMove(type.id, levelNumber, CreatureMove::SpinWeb);
}
bool Dungeon::webFloor(int col, int row) const {
	if (!IsInBounds(col, row) || !IsInBounds(col, row - 1))
		return false;
	const Tile t = MapAt(col, row);
	return (t.type == NoObject || t.type == MonsterSpawn) && !isSolidTile(t) && !inHalfWater(t) &&
		   isFloor(MapAt(col, row - 1));
}
bool Dungeon::PlayerInWeb() const {
	const JumpState& jump = sim.player->jump;
	const auto col = static_cast<int>(std::floor(mapX)), row = static_cast<int>(std::floor(mapY));
	return !jump.jumping && !jump.falling && WebAt(col, row);
}
Terrains Dungeon::PlayerTerrains() const {
	const JumpState& jump = sim.player->jump;
	return TerrainsAt(
		static_cast<int>(std::floor(mapX)),
		static_cast<int>(std::floor(mapY)),
		!jump.jumping && !jump.falling,
		false
	);
}
TerrainMood Dungeon::PlayerTerrainMood() const {
	const PlayerStats& stats = sim.player->stats;
	return terrainMood(PlayerTerrains(), stats.TerrainNative(), stats.TerrainHostile());
}
TerrainFactors Dungeon::PlayerTerrainFactors() const {
	const PlayerStats& stats = sim.player->stats;
	return terrainFactors(PlayerTerrains(), stats.TerrainNative(), stats.TerrainHostile(), PLAYER_TERRAIN_ROWS);
}
void Dungeon::burnWeb(int cell) {
	if (const int k = hazards.WebAt(cell); k >= 0) {
		hazards.webs[static_cast<size_t>(k)].goneMs = GameClock::now();
		hazards.webs[static_cast<size_t>(k)].burnt = true;
	}
}
int Dungeon::cutWebs(float fromX, float toX, int row) {
	const auto c0 = static_cast<int>(std::floor(std::min(fromX, toX)));
	const auto c1 = static_cast<int>(std::floor(std::max(fromX, toX)));
	int cut = 0;
	for (Web& w : hazards.webs)
		if (w.goneMs < 0 && w.cell / MAP_WIDTH == row && w.cell % MAP_WIDTH >= c0 && w.cell % MAP_WIDTH <= c1) {
			w.goneMs = GameClock::now();
			cut++;
		}
	return cut;
}
void Dungeon::updateWebs() {
	const int now = GameClock::now();
	hazards.webs.erase(
		std::remove_if(
			hazards.webs.begin(),
			hazards.webs.end(),
			[now](const Web& w) { return w.goneMs >= 0 && now - w.goneMs >= (w.burnt ? WEB_BURN_MS : WEB_CUT_MS); }
		),
		hazards.webs.end()
	);
	hazards.snuffs.erase(
		std::remove_if(
			hazards.snuffs.begin(),
			hazards.snuffs.end(),
			[now](const Motion& m) { return now - m.startMs >= SNUFF_MS; }
		),
		hazards.snuffs.end()
	);
	for (Web& w : hazards.webs) {
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
