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

void Dungeon::UpdateMonsters() {
	updateBoss();
	for (Monster& mon : monsters) {
		if (!mon.Active() || mon.Emerging())
			continue;

		if (!mon.Alive())
			markSlain(mon); // also a trap's or the poison's kill
		if (std::optional<ItemKind> drop = mon.TakeDrop())
			dropChest(mon, *drop);
		auto waterAt = [&](float x) {
			return !mon.flies() && inHalfWater(MapAt(static_cast<int>(std::floor(x)), mon.Row()));
		};
		mon.SetInWater(waterAt(mon.CentreX()), waterAt(mon.HeadX()),
					   mon.flies() ? 0.f : WaterSink(mon.CentreX(), mon.Row()));
		const auto monCol = static_cast<int>(std::floor(mon.CentreX()));
		mon.SetOnRungs(mon.Climbing() || (!mon.flies() && MapAt(monCol, mon.Row()).type == Ladder &&
										  IsInBounds(monCol, mon.Row()) && !isSolidTile(MapAt(monCol, mon.Row() - 1))));

		if (!mon.Alive() && mon.OnRungs()) { // killed on the rungs: down the shaft to the floor
			if (!mon.Falling()) {
				int floor = mon.Row();
				while (floor > 0 && !isSolidTile(MapAt(monCol, floor - 1)))
					floor--;
				mon.DropTo(floor);
			}
			mon.UpdateClimb();
			continue;
		}

		if (mon.flies()) {
			updateFlyer(mon);
			continue;
		}

		if (mon.jumping()) { // lands even if killed in the air
			mon.UpdateJump();
			continue;
		}

		if (mon.LeavesChest()) { // a treasure tile never spawns a monster again
			// Its die clip ends with the lid falling open over the gold: the chest stays open.
			placeDroppedChest(MapIndex(mon.Col(), mon.SpawnRow()),
							  fileIdOf(RollMimicLoot(sim.items->Owned(), sim.random->gameplay)));
			mon.Clear();
			continue;
		}

		if (mon.Alive() && !won && (mon.Lurk(mapX, mapY) || mon.Rising()))
			continue;

		if (mon.Grabbing()) { // the tell, the hold (plans crocodile-hold-bite, giant-scorpion-grab)
			mon.UpdateGrab(mapX, mapY, PlayerWading());
			continue;
		}
		if (updateBurrower(mon) || updateCharger(mon))
			continue;

		if (float sx = 0.f, sy = 0.f; mon.TakeShot(mapX, sx, sy))
			shootAtPlayer(mon, sx, sy);
		if (mon.AttackingRanged())
			continue;
		if (!won && !mon.OnRungs() && mon.canAttackRanged(mapX, mapY) && clearRow(mon.HeadX(), mapX, mon.Row())) {
			mon.AttackRanged();
			continue;
		}

		updateWalker(mon);
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
bool Dungeon::PlayerInDanger() const {
	const Player& player = *sim.player;
	return player.stats.poison.Any() || player.burning.Any() || PlayerHeldBy() != nullptr || PlayerOnLadder() ||
		   player.jump.jumping || player.jump.falling || Travelling();
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
			placeDroppedChest(MapIndex(col, row), fileIdOf(weapon)); // open: the player sees what it left
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
	shots.clear();
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
	touchPoison(*nearest);
	return true;
}
//======================================================================================
// A monster tile came into view: its monster appears, unless it is already there. Uses a free slot, else the slot
// of a dead monster.
bool Dungeon::SpawnMonster(int i, int j) {
	const Tile tile = Map(static_cast<float>(i), static_cast<float>(j));
	const int typeId = tile.attr;
	if (tile.type != MonsterSpawn || typeId < 1 || typeId > MONSTER_TYPE_MAX) // a slain one's tile spawns nothing
		return false;
	for (const Monster& mon : monsters)
		if (mon.Active() && !mon.Minion() && mon.Col() == i && mon.SpawnRow() == j)
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
	const int healthBefore = mon.Health();
	const bool wasBurning = mon.Burns();
	const bool killed = mix ? mon.TakeWeaponHit(dmg, *mix) : mon.takeHit(dmg);
	sim.player->stats.run.Hit(healthBefore - std::max(0, mon.Health()));
	if (mix && (*mix)[FIRE] > 0)
		sim.events->Note(FieldNote::Fire);
	if (mon.Burns() && !wasBurning)
		sim.events->Status("%s catches fire", type.name);
	if (killed)
		rewardKill(mon);
	else if (mix)
		venomHit(mon);
	mon.Struggle(); // a scorpion that holds the player lets go sooner
}
//======================================================================================
const Monster* Dungeon::PlayerHeldBy() const {
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Holding())
			return &mon;
	return nullptr;
}
//======================================================================================
float Dungeon::HeldViewRoll() const {
	const Monster* holder = PlayerHeldBy();
	if (holder == nullptr)
		return 0.f;
	const HoldRules rules = holder->Hold();
	const auto age = static_cast<float>(holder->HoldAgeMs());
	if (rules.tickMs <= 0) // in the claws: a shake
		return HELD_SHAKE_DEG * std::sin(age * 0.06f);
	return HELD_ROLL_DEG * std::sin(age / static_cast<float>(rules.tickMs) * 2.f * static_cast<float>(M_PI));
}
//======================================================================================
// Thorns and sap: a melee hit on the plant may poison the player (plan plant-poison).
void Dungeon::touchPoison(const Monster& mon) {
	const MonsterType& type = *mon.Type();
	if (!type.touch || !sim.random->gameplay.percent(type.touch->chancePercent))
		return;
	sim.events->Status("Its sap burns");
	sim.player->Poison(type.touch->tier, *sim.events, sim.random->gameplay);
	sim.journal->SeeMove(type.id, levelNumber, CreatureMove::Poison);
}
//======================================================================================
// The venom amulet: a weapon hit that did not kill may poison (plan venom-amulet).
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
void Dungeon::markSlain(const Monster& mon) {
	if (mon.Minion() || mon.Type()->locomotion == Locomotion::Ambush)
		return;
	Tile& spawn = map[MapIndex(mon.Col(), mon.SpawnRow())];
	if (spawn.type == MonsterSpawn)
		setObject(spawn, slainObject(spawn.attr));
}
//======================================================================================
void Dungeon::rewardKill(Monster& mon) {
	markSlain(mon);
	const MonsterType& type = *mon.Type();
	sim.journal->KillCreature(type.id, levelNumber);
	sim.player->stats.run.Slay(type.id);
	const int xp = mon.Minion() ? MinionXP(type.xp) : type.xp;
	sim.events->Status("Gained %d XP", xp);
	sim.player->stats.AddXP(xp, *sim.events);
	if (mon.Minion() || type.locomotion == Locomotion::Ambush)
		return;
	mon.SetDrop(RollKillDrop(type.isBoss(), sim.items->Owned(), levelNumber, sim.random->gameplay));
}
