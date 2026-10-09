#include "dungeon.h"
#include "../entities/player.h"
#include "item_bag.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include "../core/gameplay_config.h"
#include "loot.h"

bool Dungeon::walkerBlocked(int col, int row, bool reckless) const {
	if (!IsInBounds(col, row))
		return true;
	Tile cell = MapAt(col, row);
	if (isSolidTile(cell))
		return true;
	const bool trap = cell.type == Spike || cell.type == Death || cell.type == DartPlate ||
					  (cell.type == RockFall && rockState(cell) != RockState::Fallen);
	if (trap && !reckless)
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
// The centre of the landing cell. On the player's cell: in reach of them, never past them, or the monster would turn
// and leap back over the gap.
float Dungeon::leapTarget(const Monster& mon, int land, int dir) const {
	const float centre = static_cast<float>(land) + 0.5f;
	if (static_cast<int>(std::floor(mapX)) != land)
		return centre;
	const float stop = mon.HalfWidth() + sim.player->HalfWidth() + MONSTER_BITE_REACH / 2.f;
	const float bite = mapX - static_cast<float>(dir) * stop;
	return dir > 0 ? std::clamp(bite, static_cast<float>(land), centre)
				   : std::clamp(bite, centre, static_cast<float>(land + 1));
}
//======================================================================================
void Dungeon::UpdateMonsters() {
	updateBoss();
	for (Monster& mon : monsters) {
		if (!mon.Active() || mon.Emerging())
			continue;

		if (std::optional<ItemKind> drop = mon.TakeDrop())
			dropChest(mon, *drop);
		auto waterAt = [&](float x) {
			return !mon.flies() && inHalfWater(MapAt(static_cast<int>(std::floor(x)), mon.Row()));
		};
		mon.SetInWater(waterAt(mon.CentreX()), waterAt(mon.HeadX()),
					   mon.flies() ? 0.f : waterSink(mon.CentreX(), mon.Row()));

		if (mon.flies()) {
			if (!won) {
				const auto col = static_cast<int>(std::floor(mon.flightProbeX()));
				mon.Fly(!IsInBounds(col, mon.Row()) || isSolidTile(MapAt(col, mon.Row())), mapX, mapY);
			}
			continue;
		}

		if (mon.jumping()) { // lands even if killed in the air
			mon.UpdateJump();
			continue;
		}

		if (mon.LeavesChest()) { // a treasure tile never spawns a monster again
			ItemFileId loot = fileIdOf(RollMimicLoot(sim.items->Owned(), sim.random->gameplay));
			setObject(map[MapIndex(mon.Col(), mon.Row())], Tile{Treasure, loot.type, loot.id});
			mon.Clear();
			continue;
		}

		if (mon.Alive() && !won && (mon.Lurk(mapX, mapY) || mon.Rising()))
			continue;

		if (mon.Burrowing()) {
			if (mon.UpdateBurrow())
				holes.push_back({mon.CentreX(), mon.Row(), GameClock::now()});
			continue;
		}
		if (!won && mon.canDive(mapY)) {
			if (float to = 0.f; burrowTarget(mon, to)) {
				holes.push_back({mon.CentreX(), mon.Row(), GameClock::now()});
				mon.Dive(to - static_cast<float>(mon.Col()) - 0.5f);
			} else
				mon.DelayDive();
			continue;
		}

		if (mon.Charging()) {
			const auto col = static_cast<int>(std::floor(mon.HeadX() + 0.05f * static_cast<float>(mon.ChargeDir())));
			mon.UpdateCharge(walkerBlocked(col, mon.Row(), true), mapX, mapY);
			continue;
		}
		if (!won && mon.canCharge(mapX, mapY) && clearRow(mon.HeadX(), mapX, mon.Row())) {
			mon.StartCharge(mapX);
			continue;
		}

		if (float sx = 0.f, sy = 0.f; mon.TakeSpit(sx, sy))
			spitVenom(mon, sx, sy);
		if (mon.Spitting())
			continue;
		if (!won && mon.canSpit(mapX, mapY) && clearRow(mon.HeadX(), mapX, mon.Row())) {
			mon.Spit();
			continue;
		}

		if (mon.Alive() && !won && mon.StepDue()) {
			int dir = mon.attackDirection(mapX, mapY);
			auto col = static_cast<int>(std::floor(mon.seekProbeX(dir)));
			bool blocked = walkerBlocked(col, mon.Row(), mon.reckless());
			if (blocked && dir != 0 && mon.canJump()) {
				int land = leapLanding(col, mon.Row(), dir);
				// Not while the player is in the gap: the rat would leap over them.
				float gapFrom = static_cast<float>(dir > 0 ? col : land + 1);
				float gapTo = static_cast<float>(dir > 0 ? land : col + 1);
				if (land >= 0 && (mapX < gapFrom || mapX >= gapTo)) {
					mon.Jump(leapTarget(mon, land, dir) - static_cast<float>(mon.Col()) - 0.5f);
					continue;
				}
			}
			if (!mon.Seek(blocked, mapX, mapY))
				if (mon.AttackDue())
					mon.Attack(mapY);
		}
	}
}
//======================================================================================
bool Dungeon::PlayerSafe() const {
	for (const Monster& mon : monsters)
		if (mon.Threatens(mapX, mapY) && (mon.rooted() || clearRow(mon.CentreX(), mapX, mon.Row())))
			return false;
	return true;
}
//======================================================================================
bool Dungeon::burrowTarget(const Monster& mon, float& outX) const {
	const int side = mon.CentreX() < mapX ? 1 : -1;
	for (int s : {side, -side}) {
		const float x = mapX + static_cast<float>(s) * BURROW_BEHIND;
		if (!walkerBlocked(static_cast<int>(std::floor(x)), mon.Row(), true)) {
			outX = x;
			return true;
		}
	}
	return false;
}
//======================================================================================
// A killed monster's weapon chest: on the floor under where it died (a flyer's falls down to it), else on the nearest
// free floor cell beside it; lost if there is none (a kill over a pit or a trap).
void Dungeon::dropChest(const Monster& mon, ItemKind weapon) {
	constexpr int MAX_FALL = 12;
	constexpr int MAX_SIDE = 3;
	const auto deathCol = static_cast<int>(std::floor(mon.CentreX()));
	auto floorCell = [this](int col, int row) {
		for (int fall = 0; fall < MAX_FALL && IsInBounds(col, row - 1); fall++) {
			if (MapAt(col, row).type != NoObject || isSolidTile(MapAt(col, row)))
				return -1;
			if (isSolidTile(MapAt(col, row - 1)))
				return row;
			row--;
		}
		return -1;
	};
	for (int side = 0; side <= MAX_SIDE; side++)
		for (int dir : {1, -1}) {
			const int col = deathCol + side * dir;
			const int row = IsInBounds(col, mon.Row()) ? floorCell(col, mon.Row()) : -1;
			if (row < 0)
				continue;
			const ItemFileId loot = fileIdOf(weapon);
			setObject(map[MapIndex(col, row)], Tile{Treasure, loot.type, loot.id});
			return;
		}
}
//======================================================================================
int Dungeon::ChestCount() const {
	int n = 0;
	for (const Tile& t : map)
		n += t.type == Treasure ? 1 : 0;
	return n;
}
//======================================================================================
int Dungeon::MonsterBarsShown() const {
	int n = 0;
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && mon.Alerted())
			n++;
	return n;
}
//======================================================================================
void Dungeon::clearMonsters() {
	for (Monster& mon : monsters)
		mon.Clear();
	missiles.clear();
	venoms.clear();
	holes.clear();
	bossFight = BossFight{};
}
//======================================================================================
// A monster walks away from its spawn tile, so it is culled by where it is now.
bool Dungeon::inView(const Monster& mon) const { return mon.Active() && view().contains(mon.CentreX(), mon.Row()); }
//======================================================================================
// A monster on screen goes in the journal; a lurker only once it gives itself away (the mimic looks like a chest).
void Dungeon::noteSeenMonsters() {
	for (const Monster& mon : monsters)
		if (inView(mon) && mon.Alive() && !mon.lurking())
			sim.journal->SeeCreature(mon.Type()->id, levelNumber);
}
//======================================================================================
// Every active monster, seen or not: a dead one finishes its death clip (a killed mimic leaves its chest) off screen
// too. The blood has its own random stream, so this does not change the game's rolls.
void Dungeon::AnimateMonsters() {
	for (Monster& mon : monsters)
		if (mon.Active())
			mon.Animate(mapX, mapY);
}
//======================================================================================
bool Dungeon::AttackNearest(int damage, const DamageMix& mix, float reach, int dir) {
	Monster* nearest = nullptr;
	for (Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && !mon.Hidden() && mon.Nearby(mapX, mapY, reach, dir) &&
			(!nearest || mon.MeleeGap(mapX, dir) < nearest->MeleeGap(mapX, dir)))
			nearest = &mon;
	if (!nearest)
		return false;
	playerHit(*nearest, damage, &mix);
	return true;
}
//======================================================================================
// A monster tile came into view: its monster appears, unless it is already there. Uses a free slot, else the slot
// of a dead monster.
bool Dungeon::SpawnMonster(int i, int j) {
	const int typeId = Map(static_cast<float>(i), static_cast<float>(j)).attr;
	if (typeId < 1 || typeId > MONSTER_TYPE_MAX)
		return false;
	for (const Monster& mon : monsters)
		if (mon.Active() && !mon.Minion() && mon.Col() == i && mon.Row() == j)
			return false;
	Monster* slot = freeMonsterSlot();
	if (!slot)
		return false;
	slot->Spawn((*sim.monsterTypes)[typeId], i, j, monsterLinks(), sim.random->effects);
	if (slot->Type()->isBoss())
		startBossFight(static_cast<int>(slot - monsters));
	return true;
}
//======================================================================================
// Never the boss's slot: its death is handled on the next update (updateBoss).
Monster* Dungeon::freeMonsterSlot() {
	for (Monster& mon : monsters)
		if (!mon.Active())
			return &mon;
	for (Monster& mon : monsters)
		if (mon.Health() < 1 && &mon - monsters != bossFight.slot)
			return &mon;
	return nullptr;
}
//======================================================================================
void Dungeon::playerHit(Monster& mon, int dmg, const DamageMix* mix) {
	const MonsterType& type = *mon.Type();
	if (mix && sim.journal->TryDamage(type.id, levelNumber, mainType(*mix))) {
		const DamageType main = mainType(*mix);
		const int rate = type.resist[static_cast<size_t>(main)];
		const char* how = rate > NORMAL		? "weak to"
						  : rate == NORMAL	? "no resistance to"
						  : rate >= RESISTS ? "resists"
											: "barely hurt by";
		sim.events->Status("Journal: %s, %s %s", type.name, how, DAMAGE_TYPE_NAMES[static_cast<size_t>(main)]);
	}
	if (mix ? mon.TakeWeaponHit(dmg, *mix) : mon.takeHit(dmg))
		rewardKill(mon);
	else if (mix)
		venomHit(mon);
}
//======================================================================================
// The venom amulet: a weapon hit that did not kill may poison (docs/plan/solved/venom-amulet.md).
void Dungeon::venomHit(Monster& mon) {
	const PlayerStats& stats = sim.player->stats;
	if (stats.VenomPercent() <= 0 || !mon.Alive() || !sim.random->gameplay.percent(stats.VenomPercent()))
		return;
	const MonsterType& type = *mon.Type();
	const bool took = mon.TakePoison(stats.VenomTier(), true, sim.random->gameplay);
	if (sim.journal->TryPoison(type.id, levelNumber))
		sim.events->Status("Journal: %s, %s", type.name,
						   type.poisonResistPercent >= 100 ? "immune to poison"
						   : type.poisonResistPercent > 0  ? "resists poison"
														   : "no resistance to poison");
	else if (took)
		sim.events->Status("%s is poisoned", type.name);
}
//======================================================================================
// A minion's XP depends on its boss (MinionXP). A mimic leaves its own chest, a minion none.
void Dungeon::rewardKill(Monster& mon) {
	const MonsterType& type = *mon.Type();
	sim.journal->KillCreature(type.id, levelNumber);
	const int xp = mon.Minion() ? MinionXP(type.xp) : type.xp;
	sim.events->Status("Gained %d XP", xp);
	sim.player->stats.AddXP(xp, *sim.events);
	if (mon.Minion() || type.locomotion == Locomotion::Ambush)
		return;
	mon.SetDrop(RollKillDrop(type.isBoss(), sim.items->Owned(), levelNumber, sim.random->gameplay));
}
