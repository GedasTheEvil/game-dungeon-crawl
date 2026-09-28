#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include <GL/gl.h>
#include <cmath>
#include <memory>
#include "../graphics/render_config.h"
#include "../input/gameplay_config.h"

namespace {
monster* getMbyType(int type) {
	if (type == 1)
		return GAME_STATE.monsters.scarab.get();
	if (type == 2)
		return GAME_STATE.monsters.worm.get();
	if (type == 3)
		return GAME_STATE.monsters.plant.get();
	if (type == 4)
		return GAME_STATE.monsters.anubis.get();
	if (type == 5)
		return GAME_STATE.monsters.rat.get();
	if (type == 6)
		return GAME_STATE.monsters.giantRat.get();
	if (type == MonsterBat)
		return GAME_STATE.monsters.bat.get();
	if (type == MonsterGiantBat)
		return GAME_STATE.monsters.giantBat.get();

	return GAME_STATE.Player.get();
}
} // namespace

bool Dungeon::walkerBlocked(int col, int row) const {
	if (!IsInBounds(col, row))
		return true;
	Tint cell = MapAt(col, row);
	if (isSolidTile(cell) || cell.a == Spike || cell.a == Death)
		return true;
	return !IsInBounds(col, row - 1) || !isSolidTile(MapAt(col, row - 1)); // row 0 is the bottom
}
//======================================================================================
int Dungeon::leapLanding(int col, int row, int dir) const {
	if (!IsInBounds(col, row) || isSolidTile(MapAt(col, row)))
		return -1; // a wall, not a gap
	for (int k = 1; k <= MONSTER_JUMP_MAX_GAP; k++) {
		int c = col + dir * k;
		if (!IsInBounds(c, row) || isSolidTile(MapAt(c, row)))
			return -1;
		if (!walkerBlocked(c, row))
			return c;
	}
	return -1;
}
//======================================================================================
void Dungeon::UpdateMonsters() {
	for (int a = 0; a < CMaxMonsters; a++) {
		if (m[a].orX == -1 || m[a].orY == -1)
			continue;

		SyncMonsterFromToken(a);

		if (m[a].m->flies()) {
			if (!GAME_STATE.IHaveWon) {
				const auto col = static_cast<int>(std::floor(m[a].m->flightProbeX()));
				m[a].m->Fly(!IsInBounds(col, m[a].orY) || isSolidTile(MapAt(col, m[a].orY)));
			}
			SyncTokenFromMonster(a, true);
			continue;
		}

		if (m[a].m->jumping()) { // lands even if killed in the air
			m[a].m->UpdateJump();
			SyncTokenFromMonster(a, true);
			continue;
		}

		if (m[a].m->Alive() && !GAME_STATE.IHaveWon && m[a].t->TimePassed()) {
			int dir = m[a].m->attackDirection();
			auto col = static_cast<int>(std::floor(m[a].m->seekProbeX(dir)));
			bool blocked = walkerBlocked(col, m[a].orY);
			if (blocked && dir != 0 && m[a].m->canJump()) {
				int land = leapLanding(col, m[a].orY, dir);
				// Not while the player is in the gap: the rat would leap over him.
				float gapFrom = static_cast<float>(dir > 0 ? col : land + 1);
				float gapTo = static_cast<float>(dir > 0 ? land : col + 1);
				if (land >= 0 && (mapX < gapFrom || mapX >= gapTo)) {
					m[a].m->Jump(static_cast<float>(land) - static_cast<float>(m[a].orX));
					SyncTokenFromMonster(a, true);
					continue;
				}
			}
			if (!m[a].m->Seek(blocked))
				if (m[a].at->TimePassed())
					m[a].m->Attack();
			SyncTokenFromMonster(a, true);
		}
	}
}
//======================================================================================
void Dungeon::clearMonsters() {
	for (auto& token : m) {
		token.orX = -1;
		token.orY = -1;
		token.HP = 0;
	}
}
//======================================================================================
// Called in Draw() with the frame origin at the first drawn tile: column mapX - 4, row mapY - 3.
// A monster walks away from its spawn tile, so it is culled and placed by where it is now.
void Dungeon::DrawMonsters() {
	int firstCol = static_cast<int>(mapX) - 4;
	int firstRow = static_cast<int>(mapY) - 3;
	for (int a = 0; a < CMaxMonsters; a++) {
		if (m[a].orX == -1 || m[a].orY == -1)
			continue;
		float centre = static_cast<float>(m[a].orX) + m[a].mapX + 0.5f; // the drawn tiles: 10 x 6
		if (m[a].orY < firstRow || m[a].orY >= firstRow + 6 || centre < static_cast<float>(firstCol) ||
			centre >= static_cast<float>(firstCol + 10))
			continue;

		SyncMonsterFromToken(a);
		glPushMatrix();
		glTranslatef(RenderConfig::TILE_SIZE * static_cast<float>(m[a].orX - firstCol),
					 RenderConfig::TILE_SIZE * static_cast<float>(m[a].orY - firstRow), 0);
		glTranslatef(RenderConfig::MONSTER_OFFSET_X, 0, RenderConfig::MONSTER_OFFSET_Z);
		m[a].m->Draw();
		glPopMatrix();
		SyncTokenFromMonster(a, false);
	}
}
//======================================================================================
void Dungeon::GetAttack(int damage, int attackRange) {
	for (int i = 0; i < CMaxMonsters; i++) {
		if (m[i].orX != -1 && m[i].orY != -1) {
			SyncMonsterFromToken(i);
			if (m[i].m->Alive() && m[i].m->Nearby(mapX, mapY, attackRange)) {
				m[i].m->getHit(damage);
				SyncTokenFromMonster(i, true);
				break;
			}
		}
	}
}
//======================================================================================
void Dungeon::InitializeMonsterSlot(int index, int i, int j) {
	m[index].m = getMbyType(Map(static_cast<float>(i), static_cast<float>(j)).b);
	if (!m[index].blood)
		m[index].blood = std::make_unique<ParSys>();
	m[index].m->initBlood(*m[index].blood);
	m[index].m->dungeonCamX = &mapX;
	m[index].m->dungeonCamY = &mapY;
	m[index].m->tileOriginX = static_cast<float>(i);
	m[index].m->tileOriginY = static_cast<float>(j);
	m[index].orX = i;
	m[index].orY = j;
	m[index].HP = m[index].m->maxHealth;
	m[index].m->GetCords(m[index].mapX, m[index].mapY);
	m[index].state = static_cast<int>(m[index].m->flies() ? ModelState::Idle : ModelState::Move);
	m[index].facing_dir = 0;
	m[index].flight = Flight{};
	m[index].leap = Leap{};
	m[index].anim = m[index].m->spawnAnimations();
	if (!m[index].t)
		m[index].t = std::make_unique<timer>(70);
	if (!m[index].at)
		m[index].at = std::make_unique<timer>(800);
}
//======================================================================================
bool Dungeon::SpawnMonster(int i, int j) {
	int index = -1;

	for (int a = 0; a < CMaxMonsters; a++)
		if (m[a].orX == i && m[a].orY == j) {
			index = a;
			break;
		}

	if (index != -1)
		return false;

	for (int a = 0; a < CMaxMonsters; a++)
		if (m[a].orX == -1 && m[a].orY == -1) {
			index = a;
			break;
		}

	if (index != -1) {
		InitializeMonsterSlot(index, i, j);
		return true;
	}

	for (int a = 0; a < CMaxMonsters; a++)
		if (m[a].HP < 1) {
			index = a;
			break;
		}

	if (index != -1) {
		if (m[index].orX != i || m[index].orY != j) {
			InitializeMonsterSlot(index, i, j);
			return true;
		}
	}

	return false;
}
