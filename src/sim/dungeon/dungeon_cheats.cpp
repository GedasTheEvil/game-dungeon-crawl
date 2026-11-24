// What only the tests do to the dungeon (plan dungeon-subsystems).
#include "dungeon_cheats.h"
#include "../../core/rng.h"
#include "../../level/tuning/gameplay_skills.h"

Monster* DungeonCheats::nearest() const {
	const Monster* mon = d.nearestMonster();
	return mon == nullptr ? nullptr : &d.monsters[mon - d.monsters];
}
Monster* DungeonCheats::boss() const { return d.bossFight.slot < 0 ? nullptr : &d.monsters[d.bossFight.slot]; }
void DungeonCheats::HurtBoss(int dmg) {
	if (Monster* b = boss())
		d.playerHit(*b, dmg, nullptr);
}
bool DungeonCheats::KillNearestMonster() {
	Monster* mon = nearest();
	if (mon == nullptr)
		return false;
	d.playerHit(*mon, mon->Health(), nullptr);
	return true;
}
bool DungeonCheats::SetNearestMonsterHealth(int hp) {
	Monster* mon = nearest();
	if (mon == nullptr)
		return false;
	mon->SetHealth(hp);
	return true;
}
bool DungeonCheats::SetBossHealth(int hp) {
	Monster* b = boss();
	if (b == nullptr || !b->Alive())
		return false;
	b->SetHealth(hp);
	return true;
}
void DungeonCheats::PoisonBoss(PoisonTier tier) {
	if (Monster* b = boss())
		b->TakePoison(tier, true, d.sim.random->gameplay);
}
bool DungeonCheats::GrabNearest() {
	Monster* mon = nearest();
	return mon != nullptr && mon->StartGrab();
}
void DungeonCheats::BleedNearestMonster(BleedTier tier) {
	if (Monster* mon = nearest())
		mon->TakeBleed(tier, 100, true, d.sim.random->gameplay);
}
void DungeonCheats::PoisonNearestMonster(PoisonTier tier) {
	if (Monster* mon = nearest())
		mon->TakePoison(tier, true, d.sim.random->gameplay);
}
bool DungeonCheats::StunNearest() {
	Monster* target = nearest();
	if (Monster* b = boss(); b != nullptr && b->Alive())
		target = b;
	return target != nullptr && target->StunHit(STUN_MS, target->lurking());
}
bool DungeonCheats::PlaceWeb(int col, int row) {
	if (!d.webFloor(col, row) || d.hazards.WebAt(d.MapIndex(col, row)) >= 0)
		return false;
	d.hazards.webs.push_back({d.MapIndex(col, row), GameClock::now()});
	return true;
}
bool DungeonCheats::PlaceBlaze(int col, int row, int ms) {
	if (!d.blazeFloor(col, row))
		return false;
	d.blazeAt(static_cast<float>(col) + 0.5f, static_cast<float>(row), ms);
	return true;
}
bool DungeonCheats::SetMirror(int col, int row, int dir) {
	if (!d.IsInBounds(col, row) || d.MapAt(col, row).type != Mirror)
		return false;
	d.map[d.MapIndex(col, row)].value = static_cast<int>(beamDir(dir));
	d.sun.Trace(d.map);
	return true;
}
