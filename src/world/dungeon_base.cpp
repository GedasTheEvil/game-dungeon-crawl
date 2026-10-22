#include "dungeon.h"
#include "../entities/player.h"
#include "item_bag.h"
#include "journal.h"
#include "rng.h"
#include "skills.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include "../graphics/render_config.h"
#include "../core/logger.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace {
constexpr float LADDER_REACH = 0.2f; // closer to LADDER_GRIP_X than this (tiles), the player holds on to the ladder
constexpr float CLIMB_SIDE_RATE = 0.5f;
} // namespace

bool Dungeon::IsInBounds(int col, int row) const { return col >= 0 && col < MAP_WIDTH && row >= 0 && row < MAP_HEIGHT; }
//======================================================================================
int Dungeon::MapIndex(int col, int row) const {
	if (!IsInBounds(col, row))
		return 0;

	return MAP_WIDTH * row + col;
}
//======================================================================================
Tile Dungeon::MapAt(int col, int row) const { return map[MapIndex(col, row)]; }
//======================================================================================
Tile Dungeon::Map(float x, float y) const { return MapAt(static_cast<int>(x), static_cast<int>(y)); }
//======================================================================================
void Dungeon::SetMapBAtPlayer(int value) { map[MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))].attr = value; }
//======================================================================================
bool Dungeon::PlacePlayer(int col, int row) {
	if (!IsInBounds(col, row) || isSolidTile(MapAt(col, row)) ||
		(MapAt(col, row).type != Ladder && !isSolidTile(MapAt(col, row - 1))))
		return false;
	mapX = static_cast<float>(col) + 0.5f;
	mapY = static_cast<float>(row);
	resetPlayerMotion();
	exploreAroundPlayer();
	return true;
}
//======================================================================================
void Dungeon::resetPlayerMotion() {
	JumpState& jump = sim.player->jump;
	jump.jumping = false;
	jump.falling = false;
	jump.velocity = 0.f;
	jump.fall_velocity = FALL_STEP;
	shove = Shove{};
	hazeMs = 0;
	sunkMs = 0;
	sandSlowUntilMs = 0;
}
//======================================================================================
void Dungeon::exploreAroundPlayer() {
	int col = static_cast<int>(mapX);
	int row = static_cast<int>(mapY);
	for (int j = row - EXPLORE_RADIUS; j <= row + EXPLORE_RADIUS; j++)
		for (int i = col - EXPLORE_RADIUS; i <= col + EXPLORE_RADIUS; i++)
			if (IsInBounds(i, j))
				explored[MapIndex(i, j)] = true;
}
//======================================================================================
Dungeon::Dungeon() {
	mapX = 0;
	mapY = 0;
}
//======================================================================================
void Dungeon::UpdateMovementState() {
	if (Map(mapX, mapY).type != Ladder && !sim.player->jump.jumping) {
		// Climbing steps leave float drift (38.9999 for 39): without the snap a jump off the ladder lands a hair
		// below the floor top, falls and ends up inside the floor tile.
		float nearestRow = std::round(mapY);
		if (std::fabs(mapY - nearestRow) < FALL_START_THRESHOLD)
			mapY = nearestRow;
		if ((mapY - static_cast<float>(static_cast<int>(mapY))) > FALL_START_THRESHOLD ||
			!isSolidTile(Map(mapX, mapY - 1))) {
			JumpState& jump = sim.player->jump;
			if (jump.fall_inc.TimePassed()) {
				float floorY = std::floor(mapY);
				mapY -= jump.fall_velocity;
				if (mapY < floorY && isSolidTile(Map(mapX, floorY - 1))) {
					mapY = floorY; // landed: don't sink into the floor tile
					if (inHalfWater(Map(mapX, mapY)))
						sim.events->Play(WorldSound::Splash);
				}
				jump.fall_velocity = std::min(jump.fall_velocity + FALL_GRAVITY_STEP, FALL_MAX_STEP);
			}
			jump.falling = true;
		} else {
			mapY = std::floor(mapY); // drop the sub-threshold remainder left by the last step
			sim.player->jump.falling = false;
			sim.player->jump.fall_velocity = FALL_STEP;
		}
	}

	if (sim.player->jump.jumping) {
		if (sim.player->jump.jump_inc.TimePassed()) {
			if (sim.player->jump.dir_x != 0)
				Move(sim.player->jump.dir_x * sim.player->jump.speed, 0);

			mapY += sim.player->jump.velocity;
			sim.player->jump.velocity -= JUMP_GRAVITY_STEP;

			if (sim.player->jump.velocity <= 0 && mapY <= sim.player->jump.start_y) {
				mapY = sim.player->jump.start_y;
				sim.player->jump.jumping = false;
				sim.player->jump.falling = false;
				if (inHalfWater(Map(mapX, mapY)))
					sim.events->Play(WorldSound::Splash);
			}
		}
	}
}
//======================================================================================
void Dungeon::Update() {
	if (CutScenePlaying()) {
		updateCutScene();
		return;
	}
	if (Travelling()) {
		updateTravel();
		return;
	}
	UpdateMovementState();
	updateShove();
	updateQuicksand();
	exploreAroundPlayer();
	updateMechanisms();
	UpdateMonsters();
	noteSeenMonsters();
	updateMissiles();
	updateShots();
	updateDarts();
	updateChestGas();
	updateTraps();
	updateMonsterPoison();
	updateBurning();
	spawnInView();
	updateAnimations();
}
//======================================================================================
void Dungeon::spawnInView() {
	const ViewWindow v = view();
	for (int j = v.originRow; j < v.originRow + ViewWindow::HEIGHT; j++)
		for (int i = v.firstCol(); i < v.firstCol() + ViewWindow::WIDTH; i++)
			if (IsInBounds(i, j) && MapAt(i, j).type == MonsterSpawn)
				SpawnMonster(i, j);
}
//======================================================================================
void Dungeon::updateAnimations() {
	if (portalTimer.TimePassed())
		portalScroll -= 0.022f;
	riddleMarkYaw += 1.f;
	treasureSpin += 1.f;
}
//======================================================================================
std::optional<Dungeon::TrapContact> Dungeon::inTrap(float x, float y) const {
	const auto col = static_cast<int>(std::floor(x));
	const auto row = static_cast<int>(std::floor(y));
	for (int j = row - 1; j <= row + 1; j++) // the death trap's hitbox reaches into the next cells
		for (int i = col - 1; i <= col + 1; i++) {
			if (!IsInBounds(i, j))
				continue;
			const int type = MapAt(i, j).type;
			if (type != Spike && type != Death)
				continue;
			const float scale = type == Death ? DEATH_TRAP_SCALE : SPIKES_SCALE;
			const float centre = static_cast<float>(i) + 0.5f, half = TRAP_HITBOX_X_SCALE * scale;
			if (std::fabs(x - centre) <= half && std::fabs(y - static_cast<float>(j)) <= TRAP_HITBOX_Y_SCALE * scale)
				return TrapContact{type, (x - centre) / half, centre, half};
		}
	return std::nullopt;
}
//======================================================================================
// Only walkers on the ground stand in a spike trap: flyers pass over, a leaper is in the air. A fire vent's jet fills
// its cell (plan fire-jet-trap): it burns them all, the player in a jump too.
void Dungeon::updateTraps() {
	const std::optional<TrapContact> here = inTrap(mapX, mapY);
	if (!here || here->type != Death)
		playerVenomed = false;
	updateTrapCrossing(here);
	const float half = sim.player->HalfWidth();
	const bool jet = !here && sim.player->Alive() && inFireJet(mapX - half, mapX + half, static_cast<int>(mapY));
	if (!jet)
		playerScorched = false;
	if (jet && trapHurt.hit() > 0) {
		PlayerStats& stats = sim.player->stats;
		const int perSecond =
			std::max(FIRE_JET_HP_PER_SECOND * 100, stats.CurrentMaxHP() * FIRE_JET_PERCENT_PER_SECOND);
		const int hundredths = perSecond * TRAP_HURT_INTERVAL_MS / 1000 + jetCarry;
		jetCarry = hundredths % 100;
		if (const int dmg = stats.TrapDamage(hundredths / 100); dmg > 0) {
			sim.player->TakeHit(dmg, FIRE_JET_MIX, *sim.events);
			const double warded = perSecond / 100.0 * (100 - stats.TrapWardPercent()) / 100.0;
			sim.player->burning.Ignite(fireDamage(1, FIRE_JET_MIX, stats.resist) * warded);
			if (!playerScorched) {
				playerScorched = true;
				sim.events->Status("The wall exhales. You smell your eyebrows.");
			}
		}
	}
	if (here)
		if (const int dmg = sim.player->stats.TrapDamage(trapHurt.hit()); dmg > 0) {
			sim.player->TakeHit(dmg, SPIKE_ATTACK_MIX, *sim.events);
			if (here->type == Death && !playerVenomed) { // the death trap's tips are venomous: once per entry
				playerVenomed = true;
				sim.events->Status("The spike tips are wet with venom.");
				sim.player->Poison(PoisonTier::Strong, *sim.events, sim.random->gameplay);
			}
		}
	for (Monster& mon : monsters) {
		if (!mon.Active() || !mon.Alive())
			continue;
		if (!mon.flies() && !mon.jumping())
			if (const auto trap = inTrap(mon.CentreX(), static_cast<float>(mon.Row()))) {
				mon.StandInTrap(trap->type == Death ? std::optional(PoisonTier::Strong) : std::nullopt,
								sim.random->gameplay);
				continue;
			}
		if (!mon.Hidden() && inFireJet(mon.Left(), mon.Right(), mon.Row()))
			mon.StandInFire();
	}
}
//======================================================================================
// The player crossing a spike or death trap on foot: the way covered in it this tick, weighted by the bell (TrapBell),
// its speed and the crossing's damage; armour does not help, the trap ward does. Dealt once a whole HP is due.
void Dungeon::updateTrapCrossing(const std::optional<TrapContact>& here) {
	const float lastX = trapLastX;
	trapLastX = mapX;
	const JumpState& jump = sim.player->jump;
	if (!here || !sim.player->Alive() || jump.jumping || jump.falling) { // a jump over it only grazes it (TrapHurt)
		trapBellCarry = 0;
		return;
	}
	PlayerStats& stats = sim.player->stats;
	const bool death = here->type == Death;
	const int cross =
		TrapBell::crossHundredths(death ? DEATH_TRAP_CROSS_HP : SPIKES_CROSS_HP,
								  death ? DEATH_TRAP_CROSS_PERCENT : SPIKES_CROSS_PERCENT, stats.CurrentMaxHP());
	const float moved = std::fabs(mapX - lastX);
	if (moved > 1.f) // a teleport or a load, not a step
		return;
	const float speed = moved * 1000.f / static_cast<float>(UPDATE_TICK_MS);
	const double dealt =
		cross * TrapBell::share((lastX - here->centre) / here->half, here->offset) * TrapBell::speedFactor(speed);
	const int hundredths = static_cast<int>(std::lround(dealt)) + trapBellCarry;
	trapBellCarry = hundredths % 100;
	if (const int dmg = stats.TrapDamage(hundredths / 100); dmg > 0)
		sim.player->TakeHit(dmg, SPIKE_ATTACK_MIX, *sim.events, true);
}
//======================================================================================
void Dungeon::updateMonsterPoison() {
	for (Monster& mon : monsters)
		if (mon.Active() && mon.UpdatePoison())
			rewardKill(mon);
}
//======================================================================================
void Dungeon::updateBurning() {
	sim.player->UpdateBurning(PlayerSink() > 0.f, *sim.events);
	for (Monster& mon : monsters)
		if (mon.Active() && mon.UpdateBurning())
			rewardKill(mon);
}
//======================================================================================
bool Dungeon::Move(float dirX, float dirY) {
	if (PlayerHeldBy() != nullptr) // plans crocodile-hold-bite, giant-scorpion-grab
		return false;
	const float startX = mapX;
	const float startY = mapY;
	if (dirX != 0) {
		playerFacing = dirX > 0 ? 1 : -1;
		float probeX = mapX + dirX + (dirX > 0 ? PLAYER_BODY_HALF_WIDTH : -PLAYER_BODY_HALF_WIDTH);
		if (!isSolidTile(Map(mapX, mapY)) && !isSolidTile(Map(probeX, mapY)))
			mapX += dirX;
		else if (Map(probeX, mapY).type == Gate)
			bumpGate(static_cast<int>(probeX), static_cast<int>(mapY));
	}

	if (dirY > 0) {
		if (Map(mapX, mapY).type == Ladder && Map(mapX, mapY + dirY + PLAYER_CLIMB_HEADROOM).type == Ladder)
			mapY += dirY;
	} else if (Map(mapX, mapY).type == Ladder && Map(mapX, mapY + dirY).type == Ladder)
		mapY += dirY;

	// Climbing pulls the player over to the ladder, so the hands reach the rungs.
	if (dirY != 0 && Map(mapX, mapY).type == Ladder) {
		float offset = std::floor(mapX) + LADDER_GRIP_X - mapX;
		mapX += std::clamp(offset, -std::fabs(dirY), std::fabs(dirY));
	}
	const bool moved = mapX != startX || mapY != startY;
	if (moved && PlayerInSand()) {
		sim.events->Note(FieldNote::Sand);
		if (GameClock::now() - wadeSoundMs >= SAND_STEP_MS) {
			wadeSoundMs = GameClock::now();
			sim.events->Play(WorldSound::SandStep);
			puffs.push_back({mapX, static_cast<int>(mapY), GameClock::now()});
		}
	}
	if (moved && PlayerWading() && GameClock::now() - wadeSoundMs >= WADE_SPLASH_MS) {
		wadeSoundMs = GameClock::now();
		sim.events->Play(WorldSound::Wade);
	}
	return moved;
}
//======================================================================================
bool Dungeon::PlayerWading() const {
	const JumpState& jump = sim.player->jump;
	return !jump.jumping && !jump.falling && inHalfWater(Map(mapX, mapY));
}
//======================================================================================
bool Dungeon::dryOpen(int col, int row) const {
	return IsInBounds(col, row) && !isSolidTile(MapAt(col, row)) && !inHalfWater(MapAt(col, row));
}
//======================================================================================
float Dungeon::WaterSink(float x, int row) const {
	const auto col = static_cast<int>(std::floor(x));
	if (!IsInBounds(col, row) || !inHalfWater(MapAt(col, row)))
		return 0.f;
	float in = 1.f; // how far in from a dry edge, in ramps
	if (dryOpen(col - 1, row))
		in = std::min(in, (x - static_cast<float>(col)) / RenderConfig::WATER_SINK_RAMP);
	if (dryOpen(col + 1, row))
		in = std::min(in, (static_cast<float>(col + 1) - x) / RenderConfig::WATER_SINK_RAMP);
	in = std::clamp(in, 0.f, 1.f);
	return RenderConfig::WATER_BASIN_DEPTH * in * in * (3.f - 2.f * in);
}
//======================================================================================
float Dungeon::PlayerSink() const {
	const auto row = static_cast<int>(std::floor(mapY));
	const float above = mapY - static_cast<float>(row); // in the air over the floor: sinks as it comes down
	return WaterSink(mapX, row) * std::clamp(1.f - above, 0.f, 1.f) + QuicksandSink();
}
//======================================================================================
void Dungeon::StartJump(int dir) {
	Player& player = *sim.player;
	if (player.jump.jumping || player.jump.falling || !player.Alive() || won || PlayerHeldBy() != nullptr)
		return;
	if (!JumpAllowed())
		return;

	const int cost = PlayerInQuicksand() ? QUICKSAND_JUMP_STAMINA : JUMP_STAMINA_COST; // out of the sand's grip
	if (player.stats.Stamina() < cost) {
		player.stats.RefuseStamina(*sim.events);
		return;
	}

	player.stats.ConsumeStamina(cost);
	player.jump.start_y = mapY;
	player.jump.dir_x = static_cast<float>(dir);
	player.jump.speed = JUMP_FORWARD_SPEED;
	player.jump.velocity = JUMP_INITIAL_VELOCITY;
	player.jump.jumping = true;
	player.jump.jump_up_timer.Reset();
	sim.events->PlayCharacter(PLAYER_CHARACTER, CharacterSound::Jump);
}
//======================================================================================
bool Dungeon::AttackAllowed(bool tell) {
	if (const Monster* holder = PlayerHeldBy(); holder != nullptr && holder->Hold().breakMs <= 0) {
		if (tell)
			sim.events->Status("%s", "Held fast in its jaws");
		return false;
	}
	if (!PlayerOnLadder())
		return true;
	if (tell)
		sim.events->Status("%s", "Hands on the rungs");
	return false;
}
//======================================================================================
bool Dungeon::PlayerHasHapi() const {
	return sim.player != nullptr && sim.player->stats.heka.Has(SkillId::FavourOfHapi); // plan skill-blessing-of-hapi
}
//======================================================================================
bool Dungeon::PlayerWaterBlessed() const {
	return sim.player != nullptr && sim.player->stats.WaterBlessed(); // plan water-blessing-potion
}
//======================================================================================
float Dungeon::PlayerWalkFactor() const {
	const float slow = GameClock::now() < sandSlowUntilMs ? SAND_SLOW_FACTOR : 1.f; // a sand fall's
	if (PlayerInQuicksand())
		return slow * QUICKSAND_SPEED_FACTOR *
			   std::max(0.f, 1.f - static_cast<float>(sunkMs) / static_cast<float>(QUICKSAND_STUCK_MS));
	if (PlayerInSand())
		return slow * SAND_SPEED_FACTOR;
	if (!PlayerWading())
		return slow;
	if (PlayerHasHapi() && PlayerWaterBlessed())
		return WADE_SPEED_FACTOR_BOTH;
	if (PlayerWaterBlessed())
		return WADE_SPEED_FACTOR_BLESSED;
	return PlayerHasHapi() ? WADE_SPEED_FACTOR_HAPI : WADE_SPEED_FACTOR;
}
//======================================================================================
float Dungeon::PlayerSprintMultiplier(float sprint) const {
	if (PlayerInSand())
		return 1.f;
	if (!PlayerWading())
		return sprint;
	if (!PlayerWadesFree() || sprint <= 1.f)
		return 1.f;
	// The share of the land sprint, divided by the wade factor PlayerWalkFactor applies on top.
	if (PlayerHasHapi() && PlayerWaterBlessed())
		return sprint * WADE_SPRINT_FACTOR_BOTH / WADE_SPEED_FACTOR_BOTH;
	if (PlayerWaterBlessed())
		return sprint * WADE_SPRINT_FACTOR_BLESSED / WADE_SPEED_FACTOR_BLESSED;
	return sprint * WADE_SPRINT_FACTOR_HAPI / WADE_SPEED_FACTOR_HAPI; // times the wade factor: sprint x 0.5
}
//======================================================================================
bool Dungeon::JumpAllowed() {
	if (!PlayerWading())
		return true;
	if (PlayerWadesFree())
		return true;
	const int now = GameClock::now();
	if (now - waterHintMs >= WATER_JUMP_HINT_MS) {
		waterHintMs = now;
		sim.events->Status("%s", "The water is too deep to jump");
	}
	return false;
}
//======================================================================================
bool Dungeon::PlayerOnLadder() const {
	if (Map(mapX, mapY).type != Ladder || std::fabs(mapX - std::floor(mapX) - LADDER_GRIP_X) > LADDER_REACH)
		return false;
	bool floorBelow = isSolidTile(Map(mapX, mapY - 1));
	return !floorBelow || mapY - std::floor(mapY) >= STANDING_EPSILON;
}
//======================================================================================
float Dungeon::ClimbPhase() const { return mapY + CLIMB_SIDE_RATE * mapX; }
//======================================================================================
int Dungeon::Type(float x, float y) { return Map(x, y).type; }
//======================================================================================
void Dungeon::getC(float& outX, float& outY) {
	outX = mapX;
	outY = mapY;
}
//======================================================================================
Dungeon::~Dungeon() {
	void* selfPtr = this;
	LOG_DEBUGF("world", "Deleting Dungeon %p", selfPtr);
}
