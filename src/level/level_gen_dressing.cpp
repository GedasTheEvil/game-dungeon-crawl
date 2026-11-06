#include "level_builder.h"
#include "inscriptions.h"
#include <algorithm>
#include <vector>

namespace levelgen {
// The decor layer where it tells something (step 9); the rest the game scatters by the zone's look (zone_look.h).
// The tombs and sealed rooms: a sarcophagus, canopic jars, pottery and scrolls on their free floor. The boss
// room: the zone's guardian statue before the boss gate, a winged sun on the wall beside it. The web halls'
// dark halls: webs under the ceiling.
void LevelBuilder::dressRooms() {
	constexpr int TOMB_PROPS[] = {DECOR_SARCOPHAGUS, DECOR_CANOPIC, DECOR_POTTERY, DECOR_SCROLLS};
	for (const Segment& room : tombRooms) {
		bool sarcophagus = false;
		for (int x = room.x0; x <= room.x1; x++) {
			if (!figureCell(x, room.row) || busy[index(x, room.row)] != 0 || !rng.chance(0.6f))
				continue;
			int prop = TOMB_PROPS[rng.range(sarcophagus ? 1 : 0, 3)];
			sarcophagus = sarcophagus || prop == DECOR_SARCOPHAGUS;
			setProp(x, room.row, prop);
		}
	}
	const int guardian = isZone(zoneId) ? ZONE_GUARDIAN_PROP[zoneId] : -1;
	if (bossGate >= 0 && guardian >= 0 && figureCell(bossGate - bossStep, bossGateRow))
		setProp(bossGate - bossStep, bossGateRow, guardian);
	if (bossGate >= 0) {
		const int x = bossGate - 2 * bossStep;
		Tile cell = g.at(x, bossGateRow);
		if (!isWall(cell) && cell.dressing.torch != TorchPlacing::Torch && cell.dressing.decal < 0 &&
			tileDef(cell.type).decals && tileDef(cell.fixture.type).decals) {
			cell.dressing.decal = DECAL_WINGED_SUN;
			g.set(x, bossGateRow, cell);
			shape.placedDecor++;
		}
	}
	if (zoneId == static_cast<int>(ZoneId::WebHalls))
		for (const Segment& hall : route)
			for (int n = hall.dark ? 3 : 0, tries = 0; n > 0 && tries < 20; tries++) {
				const int x = rng.range(hall.x0, hall.x1);
				const int y = hall.row + hall.height - 1;
				const Tile t = g.at(x, y);
				if (isWall(g.at(x, y + 1)) && freeCell(x, y) && !hasPlacedProp(t.dressing) && !isWall(t) &&
					t.structure == Structure::Empty && !scatterOwns(x, y)) {
					setProp(x, y, DECOR_WEB);
					n--;
				}
			}
}

// A cell the scatter draws its own figure in (a page's skeleton, a mummy's coffin, an ushabti): no placed prop.
bool LevelBuilder::scatterOwns(int col, int row) const {
	const Tile t = g.at(col, row);
	return t.type == Page || t.type == MonsterSpawn;
}

void LevelBuilder::setProp(int col, int row, int prop) {
	Tile cell = g.at(col, row);
	cell.dressing.prop = prop;
	cell.dressing.propMirror = rng.chance(0.5f);
	g.set(col, row, cell);
	shape.placedDecor++;
}

bool LevelBuilder::hasBoon(int id) const {
	const InscriptionText* text = content.texts != nullptr ? content.texts->Find(id) : nullptr;
	return text != nullptr && text->boon.Any();
}

// A secret room's lore (SECRET_ROOM_TEXTS): with the texts, none with a boon if the level has one already.
int LevelBuilder::pickLore() {
	if (lorePlaced < content.lore.size())
		return content.lore[lorePlaced++];
	std::vector<int> pool;
	for (int id : SECRET_ROOM_TEXTS)
		if (content.texts == nullptr || (content.texts->Find(id) != nullptr && (boons == 0 || !hasBoon(id))))
			pool.push_back(id);
	return pool.empty() ? SECRET_ROOM_TEXTS[0]
						: pool[static_cast<size_t>(rng.range(0, static_cast<int>(pool.size()) - 1))];
}

// The carried content (step 8): the inscriptions on the halls of the route, on the way (GenOptions::inscriptions,
// else 2 or 3 of the zone's texts, 1 or 2 in the first zone, at most one boon a level; the name gate's hint only
// with a name gate), and the expedition page on a skeleton at a wing's end.
void LevelBuilder::addTexts() {
	std::vector<int> ids = content.inscriptions;
	if (ids.empty() && content.texts != nullptr && isZone(zoneId)) {
		std::vector<int> pool;
		for (int id = ZONE_TEXT_FIRST[zoneId]; id < ZONE_TEXT_FIRST[zoneId + 1]; id++)
			if (isInscriptionId(id) && id != RING_TEXT && id != FORK_TEXT && id != STARGATE_TEXT &&
				(id != NAME_GATE_TEXT || shape.nameGates > 0) && content.texts->Find(id) != nullptr)
				pool.push_back(id);
		for (int i = static_cast<int>(pool.size()) - 1; i > 0; i--)
			std::swap(pool[static_cast<size_t>(i)], pool[static_cast<size_t>(rng.range(0, i))]);
		const int wanted = zoneId == static_cast<int>(ZoneId::TombEntrance) ? rng.range(1, 2) : rng.range(2, 3);
		for (int id : pool) {
			if (static_cast<int>(ids.size()) >= wanted)
				break;
			if (hasBoon(id) && boons > 0)
				continue;
			boons += hasBoon(id) ? 1 : 0;
			ids.push_back(id);
		}
	}
	for (int id : ids)
		for (int tries = 0; tries < 60; tries++) {
			const Segment& hall = route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))];
			const int x = rng.range(std::min(hall.entry, hall.exit), std::max(hall.entry, hall.exit));
			const Structure s = g.at(x, hall.row).structure;
			if (busy[index(x, hall.row)] == 0 && freeCell(x, hall.row) && isWall(g.at(x, hall.row - 1)) &&
				(s == Structure::Empty || s == Structure::Sand)) {
				put(x, hall.row, Tile{Inscription, id, 0});
				busy[index(x, hall.row)] = 1;
				shape.inscriptions++;
				break;
			}
		}
	if (isExpeditionPage(content.page) && shape.pages == 0) {
		const int w = freeWing();
		if (w >= 0) {
			Wing& wing = wings[static_cast<size_t>(w)];
			put(wing.end, wing.itemRow, Tile{Page, content.page, 0});
			wing.taken = true;
			shape.pages++;
		}
	}
}

// The light (ZoneTerrain): dark halls with no wall torch but a torch or two, braziers on the floor, daylight
// through cracks in the ceiling. Last, so nothing else takes their cells.
void LevelBuilder::addLight() {
	for (const Segment& hall : route)
		if (hall.dark)
			darken(hall);
	if (terrain.braziers)
		for (int n = rng.range(2, 5), tries = 0; n > 0 && tries < 40; tries++) {
			const Segment& hall = route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))];
			const int x = rng.range(hall.x0, hall.x1);
			const Tile t = g.at(x, hall.row);
			if (t.structure != Structure::Empty || !freeCell(x, hall.row) || busy[index(x, hall.row)] != 0 ||
				!isWall(g.at(x, hall.row - 1)) || t.dressing.torch == TorchPlacing::Torch || lightsFoe(x, hall.row) ||
				hasPlacedProp(g.at(x - 1, hall.row).dressing) || hasPlacedProp(g.at(x + 1, hall.row).dressing))
				continue;
			Tile cell = t;
			cell.dressing.prop = DECOR_BRAZIER;
			g.set(x, hall.row, cell);
			busy[index(x, hall.row)] = 1;
			shape.braziers++;
			n--;
		}
	if (terrain.lightShafts)
		for (int n = rng.range(2, 4), tries = 0; n > 0 && tries < 40; tries++) {
			const Segment& hall = route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))];
			const int x = rng.range(hall.x0, hall.x1);
			if (crack(hall, x)) {
				put(x, hall.row + hall.height - 1, Tile{LightShaft, static_cast<int>(BeamDir::Down), 0});
				n--;
			}
		}
}

// The figures the zone's monsters hide among: a mummy's coffin (its own, on its spawn) between decoy coffins with
// a mummy asleep in them, a living ushabti (its decor ushabti on its spawn) among decor ushabti.
void LevelBuilder::dressFigures() {
	for (int cell = 0; cell < CELLS; cell++) {
		const Tile t = g.cells[cell];
		if (t.type != MonsterSpawn || (t.attr != MonsterMummy && t.attr != MonsterUshabti))
			continue;
		const int prop = t.attr == MonsterMummy ? DECOR_COFFIN_MUMMY : DECOR_USHABTI;
		if (prop == DECOR_COFFIN_MUMMY && !rng.chance(0.5f))
			continue;
		const int col = cell % LEVEL_WIDTH;
		const int row = cell / LEVEL_WIDTH;
		for (int k : {-1, 1, -2, 2})
			if (figureCell(col + k, row)) {
				Tile beside = g.at(col + k, row);
				beside.dressing.prop = prop;
				beside.dressing.propMirror = k > 0;
				g.set(col + k, row, beside);
				busy[index(col + k, row)] = 1;
			}
	}
}

bool LevelBuilder::lightsFoe(int col, int row) const {
	for (int x = col - LIGHT_SHUN_CELLS; x <= col + LIGHT_SHUN_CELLS; x++) {
		const Tile t = g.at(x, row);
		const MonsterKind* kind = t.type == MonsterSpawn ? monsterKind(t.attr) : nullptr;
		if (kind != nullptr && kind->hostile.has(Terrain::Light))
			return true;
	}
	return false;
}

// Dry floor with nothing in it or on its back wall, no prop yet.
bool LevelBuilder::figureCell(int col, int row) const {
	const Tile t = g.at(col, row);
	return t.structure == Structure::Empty && freeCell(col, row) && isWall(g.at(col, row - 1)) &&
		   !hasPlacedProp(t.dressing);
}

// The dark halls (ZoneTerrain::dark), chosen before the monsters: the camel spiders run in them.
void LevelBuilder::chooseDark() {
	if (terrain.dark == TerrainAmount::Few)
		for (int n = rng.range(1, 2); n > 0; n--)
			route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))].dark = true;
	else if (terrain.dark != TerrainAmount::None)
		for (Segment& hall : route)
			hall.dark = rng.percent(terrain.dark == TerrainAmount::Much ? 75 : 40);
}

// No wall torch in the hall, but one every TORCH_GAP cells or so in a few places, on free floor.
void LevelBuilder::darken(const Segment& hall) {
	for (int y = hall.row; y < hall.row + hall.height; y++)
		for (int x = hall.x0; x <= hall.x1; x++)
			if (!isSolidStructure(g.at(x, y).structure)) {
				Tile cell = g.at(x, y);
				cell.dressing.torch = TorchPlacing::None;
				g.set(x, y, cell);
			}
	for (int x = hall.x0 + rng.range(2, TORCH_GAP); x <= hall.x1 - 2; x += rng.range(TORCH_GAP, 2 * TORCH_GAP)) {
		if (!freeCell(x, hall.row) || lightsFoe(x, hall.row))
			continue;
		Tile cell = g.at(x, hall.row);
		cell.dressing.torch = TorchPlacing::Torch;
		g.set(x, hall.row, cell);
	}
	shape.darkHalls++;
}

// A crack for daylight over column x of the hall: rock above its top row, the beam down to the floor through
// free cells, onto rock (no hole, no mirror, no sun disc it would light).
bool LevelBuilder::crack(const Segment& hall, int x) const {
	if (!isWall(g.at(x, hall.row + hall.height)) || !isWall(g.at(x, hall.row - 1)) || busy[index(x, hall.row)] != 0)
		return false;
	for (int y = hall.row; y < hall.row + hall.height; y++)
		if (!freeCell(x, y) || hasPlacedProp(g.at(x, y).dressing))
			return false;
	return true;
}
} // namespace levelgen
