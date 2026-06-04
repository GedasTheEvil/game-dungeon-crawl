#include "dungeon.h"
#include "../state/cashe.h"
#include "../core/service_locator.h"
#include <GL/gl.h>
#include <memory>

namespace {
monster* getMbyType(int type) {
	if (type == 1)
		return GAME_STATE.scarab.get();
	if (type == 2)
		return GAME_STATE.worm.get();
	if (type == 3)
		return GAME_STATE.plant.get();
	if (type == 4)
		return GAME_STATE.anubis.get();

	return GAME_STATE.Player.get();
}
} // namespace

void Dungeon::UpdateMonsters() {
	for (int a = 0; a < CMaxMonsters; a++) {
		if (m[a].orX == -1 || m[a].orY == -1)
			continue;

		SyncMonsterFromToken(a);

		if (m[a].m->Alive() && !GAME_STATE.IHaveWon && m[a].t->TimePassed()) {
			if (!m[a].m->Seek())
				if (m[a].at->TimePassed())
					m[a].m->Attack();
			SyncTokenFromMonster(a, true);
		}
	}
}
//======================================================================================
void Dungeon::DrawMonsterTile(int i, int j) {
	SpawnMonster(i, j);
	for (int a = 0; a < CMaxMonsters; a++) {
		if (m[a].orX == i && m[a].orY == j) {
			SyncMonsterFromToken(a);
			glPushMatrix();
			glTranslatef(40, 0, 10);
			m[a].m->Draw();
			glPopMatrix();
			SyncTokenFromMonster(a, false);
		}
	}
}
//======================================================================================
void Dungeon::GetAttack(int damage, int attackRange) {
	for (int i = 0; i < CMaxMonsters; i++) {
		if (m[i].orX != -1 && m[i].orY != -1) {
			SyncMonsterFromToken(i);
			if (m[i].m->Alive() && m[i].m->Nearby(x, y, attackRange)) {
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
	m[index].m->DX = &x;
	m[index].m->DY = &y;
	m[index].m->X = static_cast<float>(i);
	m[index].m->Y = static_cast<float>(j);
	m[index].orX = i;
	m[index].orY = j;
	m[index].HP = m[index].m->MaxHP;
	m[index].m->GetCords(m[index].x, m[index].y);
	m[index].state = 1;
	m[index].facing_dir = 0;
	m[index].frame = 1;
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
