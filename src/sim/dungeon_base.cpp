#include "dungeon.h"
#include "player.h"
#include "../level/item_bag.h"
#include "../level/journal.h"
#include "../level/rng.h"
#include "../level/skills.h"
#include "../level/world_events.h"
#include "../level/gameplay_config.h"
#include "render_config.h"
#include "../core/logger.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace {
constexpr float LADDER_REACH = 0.2f; // closer to LADDER_GRIP_X than this (tiles), the player holds on to the ladder
// Sub-cells (plan subcell-physics): a body touching a sub-cell's face is not in it; the feet that stand on a step reach
// this far in from the hitbox's sides (subcellReach), so a side touching a step's wall does not carry the player.
constexpr float SUBCELL_TOUCH = 1e-3f;
constexpr float FOOT_INSET = 0.01f;
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
		(MapAt(col, row).type != Ladder && !isFloor(MapAt(col, row - 1))))
		return false;
	mapX = static_cast<float>(col) + 0.5f;
	mapY = static_cast<float>(row) + floorHeight(MapAt(col, row), 0.5f); // on a step in a shaped cell
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
	lostGrip = false;
	hazeMs = 0;
	storm = 0.f;
	muffle = 0.f;
	playerInGust = false;
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
	revealSecretRooms(); // the Wisdom of Seshat (plan heka-surplus)
}
//======================================================================================
Dungeon::Dungeon() {
	mapX = 0;
	mapY = 0;
}
//======================================================================================
void Dungeon::updateCrouch() {
	Player& player = *sim.player;
	if (!player.Alive()) {
		player.crouched = false;
		return;
	}
	if (player.crouchHeld && !player.crouched)
		player.crouched = !player.jump.jumping && !player.jump.falling && Map(mapX, mapY).type != Ladder;
	else if (!player.crouchHeld && player.crouched)
		player.crouched = !roomToStand();
}
//======================================================================================
void Dungeon::UpdateMovementState() {
	updateCrouch();
	if (sim.player->Dazed() && PlayerOnLadder()) { // a screech: the hands let go (plan giant-bat-screech)
		lostGrip = true;
		sim.events->Status("%s", "Your hands let go of the rungs");
	}
	if ((Map(mapX, mapY).type != Ladder || lostGrip) && !sim.player->jump.jumping) {
		// Climbing steps leave float drift (38.9999 for 39): without the snap a jump off the ladder lands a hair
		// below the floor top, falls and ends up inside the floor tile.
		const float nearestFloor = floorUnder(mapX, mapY + FALL_START_THRESHOLD);
		if (std::fabs(mapY - nearestFloor) < FALL_START_THRESHOLD)
			mapY = nearestFloor;
		if (heightOverFloor() > FALL_START_THRESHOLD || !floorBelow(mapX, mapY)) {
			JumpState& jump = sim.player->jump;
			if (jump.fall_inc.TimePassed()) {
				const std::optional<float> floorY = floorBelow(mapX, mapY);
				mapY -= jump.fall_velocity;
				if (floorY && mapY < *floorY) {
					mapY = *floorY; // landed: don't sink into the floor tile
					if (inHalfWater(Map(mapX, mapY)) && !playerOnStep())
						sim.events->Play(WorldSound::Splash);
				}
				jump.fall_velocity = std::min(jump.fall_velocity + FALL_GRAVITY_STEP, FALL_MAX_STEP);
			}
			jump.falling = true;
		} else {
			mapY = floorUnder(mapX, mapY); // drop the sub-threshold remainder left by the last step
			lostGrip = false;
			sim.player->jump.falling = false;
			sim.player->jump.fall_velocity = FALL_STEP;
		}
	}

	JumpState& jump = sim.player->jump;
	if (jump.jumping && jump.jump_inc.TimePassed()) {
		if (jump.dir_x != 0)
			Move(jump.dir_x * jump.speed, 0);

		const float from = mapY;
		mapY += jump.velocity;
		const float rise = jump.velocity;
		jump.velocity -= JUMP_GRAVITY_STEP;
		// The head meets a low ceiling (plan subcell-physics): the rise stops under it, the fall begins.
		if (mapY > from)
			if (const std::optional<float> ceiling = ceilingAt(mapY)) {
				const int spikes = ceilingTrapAt(mapY);
				mapY = std::max(from, *ceiling - playerBodyHeight());
				jump.velocity = std::min(jump.velocity, 0.f);
				if (spikes != NoObject) // any touch: the tips reach down (plan ceiling-spikes)
					ceilingSpikesHit(spikes);
				else {
					sim.events->Play(WorldSound::HeadBump);
					if (rise > HEAD_BUMP_MIN_RISE) // a graze only knocks (plan head-bump)
						sim.player->HeadBump(*sim.events, sim.random->gameplay);
				}
			}
		// Coming down onto a step higher than the take-off, or back to the take-off height (then a fall, over a
		// lower floor).
		const std::optional<float> step = floorBelow(mapX, from);
		const bool onStep = step && *step > jump.start_y + SUBCELL_TOUCH && mapY <= *step;
		if (jump.velocity <= 0 && (onStep || mapY <= jump.start_y)) {
			mapY = onStep ? *step : jump.start_y;
			jump.jumping = false;
			jump.falling = false;
			if (inHalfWater(Map(mapX, mapY)) && !playerOnStep())
				sim.events->Play(WorldSound::Splash);
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
	updateGusts();
	updateQuicksand();
	exploreAroundPlayer();
	updateMechanisms();
	updateDial();
	UpdateMonsters();
	noteSeenMonsters();
	updateMissiles();
	updateWebs();
	updateShots();
	if (sim.items != nullptr && sim.player != nullptr) // the player's ranged weapons (plan ranged-weaker-than-melee)
		sim.items->RecoverShots(UPDATE_TICK_MS, sim.player->stats.ShotRecoveryPace());
	updateBlazes();
	updateBalls();
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
std::optional<Dungeon::TrapContact> Dungeon::inTrap(float x, float y, float half) const {
	const float body = half * (1.f - TRAP_BOX_FORGIVE);
	const auto col = static_cast<int>(std::floor(x));
	const auto row = static_cast<int>(std::floor(y));
	const int span = 1 + static_cast<int>(std::ceil(body)); // the death trap's hitbox reaches into the next cells
	for (int j = row - 1; j <= row + 1; j++)
		for (int i = col - span; i <= col + span; i++) {
			if (!IsInBounds(i, j))
				continue;
			const int type = MapAt(i, j).type;
			if (type != Spike && type != Death)
				continue;
			const float scale = type == Death ? DEATH_TRAP_SCALE : SPIKES_SCALE;
			const float centre = static_cast<float>(i) + 0.5f, reach = TRAP_HITBOX_X_SCALE * scale + body;
			if (std::fabs(x - centre) <= reach && std::fabs(y - static_cast<float>(j)) <= TRAP_HITBOX_Y_SCALE * scale)
				return TrapContact{type, (x - centre) / reach, centre, reach};
		}
	return std::nullopt;
}
//======================================================================================
int Dungeon::TrapWalkHundredths(
	TrapHurt& hurt,
	float x,
	const std::optional<TrapContact>& here,
	int maxHp,
	std::optional<float> pace,
	bool airborne
) {
	const float lastX = hurt.lastX;
	hurt.lastX = x;
	if (!here)
		return 0;
	int hundredths = 100 * hurt.hit();
	const float moved = std::fabs(x - lastX);
	if (airborne || moved > 1.f) // a jump over it only grazes it (the ramp); a teleport or a load is not a step
		return hundredths;
	const float speed = pace.value_or(moved * 1000.f / static_cast<float>(UPDATE_TICK_MS));
	const double bell = TrapBell::trapCrossHundredths(here->type == Death, maxHp) *
						TrapBell::share((lastX - here->centre) / here->half, here->offset) *
						TrapBell::speedFactor(speed);
	return hundredths + static_cast<int>(std::lround(bell));
}
//======================================================================================
bool Dungeon::TrapVenomDue(TrapHurt& hurt, const std::optional<TrapContact>& here, int hundredths) {
	if (!here || here->type != Death) {
		hurt.venomed = false;
		return false;
	}
	if (hurt.venomed || hundredths <= 0)
		return false;
	hurt.venomed = true;
	return true;
}
//======================================================================================
// Every trap hurts the player and the monsters by one rule (plan same-hazards-for-all); a kind differs only by its
// numbers (trap share, resistances, poison ward). Only walkers on the ground stand in a spike trap: flyers pass over, a
// leaper is in the air. The player and a reckless walker touch it with their box; the others keep out of its cell and
// touch it with their centre only, or a death trap's spikes, reaching into the next cells, would stab them standing
// beside it. A fire vent's jet fills its cell (plan fire-jet-trap): it burns them all, the player in a jump too.
void Dungeon::updateTraps() {
	Player& player = *sim.player;
	PlayerStats& stats = player.stats;
	const float half = player.HalfWidth();
	const std::optional<TrapContact> here = player.Alive() ? inTrap(mapX, mapY, half) : std::nullopt;
	const bool airborne = player.jump.jumping || player.jump.falling;
	const int walk = TrapWalkHundredths(stats.vitals.trapHurt, mapX, here, stats.CurrentMaxHP(), {}, airborne);
	const bool venom = TrapVenomDue(stats.vitals.trapHurt, here, walk);
	if (const int dmg = stats.TrapDamageHundredths(walk); dmg > 0)
		player.TakeHit(dmg, SPIKE_ATTACK_MIX, *sim.events, true);
	if (venom && player.Alive()) { // the death trap's tips are venomous: once per entry
		sim.events->Status("The spike tips are wet with venom.");
		player.Poison(PoisonTier::Strong, *sim.events, sim.random->gameplay);
	}
	updateDrips();
	const bool jet = !here && player.Alive() && inFireJet(mapX - half, mapX + half, static_cast<int>(mapY));
	if (!jet)
		playerScorched = false;
	if (jet && stats.vitals.trapHurt.hit() > 0) {
		const int perSecond = fireJetHundredthsPerSecond(stats.CurrentMaxHP());
		if (const int dmg = stats.TrapDamageHundredths(perSecond * TRAP_HURT_INTERVAL_MS / 1000); dmg > 0) {
			player.TakeHit(dmg, FIRE_JET_MIX, *sim.events);
			const double share = perSecond / 100.0 * stats.TrapPermille() / FULL_TRAP_SHARE; // a second's fire
			stats.vitals.burning.Ignite(fireDamage(1, FIRE_JET_MIX, stats.resist) * share);
			if (!playerScorched) {
				playerScorched = true;
				sim.events->Status("The wall exhales. You smell your eyebrows.");
			}
		}
	}
	for (Monster& mon : monsters) {
		if (!mon.Active() || !mon.Alive())
			continue;
		const std::optional<TrapContact> trap = mon.flies() || mon.jumping()
													? std::nullopt
													: inTrap(
														  mon.CentreX(),
														  static_cast<float>(mon.Row()),
														  mon.reckless() ? mon.HalfWidth() : 0.f
													  );
		TrapHurt& hurt = mon.TrapState();
		const int dealt = TrapWalkHundredths(hurt, mon.CentreX(), trap, mon.MaxHealth(), mon.WalkPace(), false);
		if (TrapVenomDue(hurt, trap, dealt))
			mon.TakePoison(PoisonTier::Strong, false, sim.random->gameplay);
		if (trap) {
			mon.TrapHitHundredths(dealt);
			continue;
		}
		if (!mon.Hidden() && inFireJet(mon.Left(), mon.Right(), mon.Row()))
			mon.StandInFire(fireJetHundredthsPerSecond(mon.MaxHealth()));
	}
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
	sim.player->UpdateSenses();
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
		const int dir = dirX > 0 ? 1 : -1;
		const float probeX = mapX + dirX + static_cast<float>(dir) * sim.player->HalfWidth();
		if (!bodyBlocked(mapX + dirX, dir, mapY))
			mapX += dirX;
		else if (Map(probeX, mapY).type == Gate)
			bumpGate(static_cast<int>(probeX), static_cast<int>(mapY));
	}

	if (lostGrip || sim.player->Dazed() ||
		sim.player->crouched) // no grip on the rungs (plan giant-bat-screech), no climb crouched
		dirY = 0.f;
	if (dirY != 0.f && sim.player->jump.jumping && !catchRungs())
		dirY = 0.f; // the jump's rise and fall own mapY: a climb against it shook the camera
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
	if (moved && motionEffectsOn && sim.player->stats.IsSprinting() && !PlayerInSand() && !PlayerWading() &&
		PlayerSink() <= 0.f && !sim.player->jump.jumping && !sim.player->jump.falling &&
		Map(mapX, mapY).type != Ladder &&
		GameClock::now() - sprintPuffMs >= SPRINT_PUFF_MS) { // plan sprint-toon-blur-and-extras
		sprintPuffMs = GameClock::now();
		const int now = sprintPuffMs;
		puffs.erase(
			std::remove_if(
				puffs.begin(),
				puffs.end(),
				[now](const Hole& h) { return now - h.startMs >= SAND_PUFF_MS; }
			),
			puffs.end()
		); // the draw prunes too, but a headless run never draws
		puffs.push_back({mapX, static_cast<int>(mapY), now});
	}
	if (moved && PlayerWading() && GameClock::now() - wadeSoundMs >= WADE_SPLASH_MS) {
		wadeSoundMs = GameClock::now();
		sim.events->Play(WorldSound::Wade);
	}
	return moved;
}
//======================================================================================
bool Dungeon::catchRungs() {
	JumpState& jump = sim.player->jump;
	if (jump.velocity > 0.f || Map(mapX, mapY).type != Ladder) // rising: a jump off the rungs with up held goes on
		return false;
	jump.jumping = false;
	jump.falling = false;
	jump.velocity = 0.f;
	jump.fall_velocity = FALL_STEP;
	jump.dir_x = 0.f;
	// In reach of the rungs at once (PlayerOnLadder); the climb's pull brings the hands to the grip.
	const float grip = std::floor(mapX) + LADDER_GRIP_X;
	mapX = std::clamp(mapX, grip - LADDER_REACH, grip + LADDER_REACH);
	return true;
}
//======================================================================================
bool Dungeon::PlayerWading() const {
	const JumpState& jump = sim.player->jump;
	return !jump.jumping && !jump.falling && inHalfWater(Map(mapX, mapY)) && !playerOnStep();
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
	const float above = heightOverFloor(); // in the air over the floor: sinks as it comes down
	const float water = playerOnStep() ? 0.f : WaterSink(mapX, row); // a step stands up out of the basin
	return water * std::clamp(1.f - above, 0.f, 1.f) + QuicksandSink() + SwallowSink();
}
//======================================================================================
bool Dungeon::subcellAt(float x, float y) const {
	const auto col = static_cast<int>(std::floor(x)), row = static_cast<int>(std::floor(y));
	if (!IsInBounds(col, row))
		return false;
	const Tile& t = map[MapIndex(col, row)];
	return hasShape(t) && solidAt(t, x - static_cast<float>(col), y - static_cast<float>(row));
}
//======================================================================================
// From the sub-cell row whose top is at y (or the first under it) down to the bottom row of the cell under y's.
std::optional<float> Dungeon::floorBelow(float x, float y) const {
	const auto row = static_cast<int>(std::floor(y));
	for (auto k = static_cast<int>(std::floor((y + SUBCELL_TOUCH) * SUBCELLS)) - 1; k >= (row - 1) * SUBCELLS; k--) {
		const float top = static_cast<float>(k + 1) * SUBCELL_SIZE;
		const float mid = top - SUBCELL_SIZE / 2.f;
		if (k == row * SUBCELLS - 1 && isFloor(Map(x, static_cast<float>(row - 1))))
			return top;
		if (subcellAt(x, mid) || subcellAt(x - subcellReach(), mid) || subcellAt(x + subcellReach(), mid))
			return top;
	}
	return std::nullopt;
}
//======================================================================================
float Dungeon::floorUnder(float x, float y) const { return floorBelow(x, y).value_or(std::floor(y)); }
//======================================================================================
float Dungeon::heightOverFloor() const { return mapY - floorUnder(mapX, mapY); }
//======================================================================================
bool Dungeon::playerOnStep() const { return floorUnder(mapX, mapY) > std::floor(mapY) + SUBCELL_TOUCH; }
//======================================================================================
float Dungeon::subcellReach() const { return sim.player->HalfWidth() - FOOT_INSET; }
//======================================================================================
bool Dungeon::bodyBlocked(float toX, int dir, float y) const {
	const float edge = toX + static_cast<float>(dir) * sim.player->HalfWidth();
	if (isSolidTile(Map(mapX, y)) || isSolidTile(Map(edge, y)))
		return true;
	const float feet = y + SUBCELL_TOUCH, head = y + playerBodyHeight() - SUBCELL_TOUCH;
	for (const float x : {mapX, (mapX + edge) / 2.f, edge})
		for (const float h : {feet, (feet + head) / 2.f, head})
			if (subcellAt(x, h))
				return true;
	return false;
}
//======================================================================================
float Dungeon::playerBodyHeight() const {
	return sim.player->crouched ? CROUCH_HEIGHT_FACTOR * PLAYER_BODY_HEIGHT : PLAYER_BODY_HEIGHT;
}
//======================================================================================
bool Dungeon::roomToStand() const {
	const float low = mapY + CROUCH_HEIGHT_FACTOR * PLAYER_BODY_HEIGHT,
				head = mapY + PLAYER_BODY_HEIGHT - SUBCELL_TOUCH;
	for (const float h : {low, (low + head) / 2.f, head}) // a whole cell over the centre only, as ceilingAt
		if (isSolidTile(Map(mapX, h)) || subcellAt(mapX - subcellReach(), h) || subcellAt(mapX, h) ||
			subcellAt(mapX + subcellReach(), h))
			return false;
	return true;
}
//======================================================================================
std::optional<float> Dungeon::ceilingAt(float y) const {
	const float head = y + playerBodyHeight();
	if (isSolidTile(Map(mapX, head)) || subcellAt(mapX, head) || subcellAt(mapX - subcellReach(), head) ||
		subcellAt(mapX + subcellReach(), head))
		return std::floor(head * SUBCELLS) / SUBCELLS;
	return std::nullopt;
}
//======================================================================================
int Dungeon::ceilingTrapAt(float y) const {
	const float head = y + playerBodyHeight();
	for (const float x : {mapX, mapX - subcellReach(), mapX + subcellReach()}) {
		const Tile t = Map(x, head);
		if (spikedSubcell(t, x - std::floor(x), head - std::floor(head)))
			return t.fixture.type;
	}
	return NoObject;
}
//======================================================================================
void Dungeon::ceilingSpikesHit(int trap) {
	Player& player = *sim.player;
	sim.events->Play(WorldSound::SpikeThunk); // the stab, not the knock
	if (!player.Alive())
		return;
	const bool death = trap == CeilingDeath;
	PlayerStats& stats = player.stats;
	sim.events->PlayCharacter(PLAYER_CHARACTER, CharacterSound::Attack); // the grunt, as a head bump's
	sim.events->Status("Spikes in the ceiling!");
	const float walk = WALK_SPEED; // a whole walk through the floor kind, as a monster's (plan same-hazards-for-all)
	const int hundredths = TrapBell::walkHundredths(death, stats.CurrentMaxHP(), walk, player.HalfWidth());
	if (const int dmg = stats.TrapDamageHundredths(hundredths); dmg > 0)
		player.TakeHit(dmg, SPIKE_ATTACK_MIX, *sim.events, true);
	if (death && player.Alive()) {
		sim.events->Status("The spike tips are wet with venom.");
		player.Poison(PoisonTier::Strong, *sim.events, sim.random->gameplay);
	}
}
//======================================================================================
void Dungeon::StartJump(int dir) {
	Player& player = *sim.player;
	if (player.jump.jumping || player.jump.falling || !player.Alive() || won || PlayerHeldBy() != nullptr ||
		player.Dazed() || player.crouched)
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
	if (sim.player->crouched) { // plan crouch: no strike back from a duck
		if (tell)
			sim.events->Status("%s", "Stand up to strike");
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
	const float daze = sim.player->Dazed() ? DAZE_WALK_FACTOR : 1.f;
	const float crouch = sim.player->crouched ? CROUCH_WALK_FACTOR : 1.f;
	return daze * crouch * groundWalkFactor();
}
//======================================================================================
float Dungeon::PlayerStepFactor() const {
	const JumpState& jump = sim.player->jump;
	const bool firmGround = !PlayerWading() && !PlayerInSand(); // plan heka-surplus
	return PlayerWalkFactor() * sim.player->stats.WalkFactor(jump.jumping || jump.falling, firmGround);
}
//======================================================================================
bool Dungeon::PlayerCanSprint() const {
	return (!PlayerWading() || PlayerWadesFree()) && !PlayerInSand() && !sim.player->Dazed() && !sim.player->crouched;
}
//======================================================================================
float Dungeon::groundWalkFactor() const {
	const float slow = (GameClock::now() < sandSlowUntilMs ? SAND_SLOW_FACTOR : 1.f) * // a sand fall's
					   PlayerTerrainFactors()
						   .speed; // the terrain's: a web's (plans camel-spider-boss, same-hazards-for-all)
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
	if (PlayerInSand() || sim.player->Dazed() || sim.player->crouched)
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
	if (lostGrip || Map(mapX, mapY).type != Ladder || std::fabs(mapX - std::floor(mapX) - LADDER_GRIP_X) > LADDER_REACH)
		return false;
	return !floorBelow(mapX, mapY) || heightOverFloor() >= STANDING_EPSILON;
}
//======================================================================================
bool Dungeon::PlayerAtLadder() const {
	const auto col = static_cast<int>(mapX);
	const auto row = static_cast<int>(mapY);
	return (IsInBounds(col, row) && MapAt(col, row).type == Ladder) ||
		   (IsInBounds(col, row - 1) && MapAt(col, row - 1).type == Ladder);
}
//======================================================================================
bool Dungeon::PlayerFacesJump(int dir) const {
	constexpr float REACH = 0.9f; // tiles from the player's centre to the cell's edge
	const auto col = static_cast<int>(mapX);
	const auto row = static_cast<int>(mapY);
	if (heightOverFloor() >= STANDING_EPSILON || !floorBelow(mapX, mapY) || PlayerOnLadder())
		return false;
	const float edge = static_cast<float>(dir > 0 ? col + 1 : col);
	const int ahead = col + dir;
	if (std::fabs(edge - mapX) > REACH || !IsInBounds(ahead, row) || !IsInBounds(ahead, row - 1))
		return false;
	const Tile next = MapAt(ahead, row);
	if (isWall(next)) // a ledge: one cell high, open over it and over the player
		return IsInBounds(col, row + 1) && IsInBounds(ahead, row + 1) && !isSolidTile(MapAt(col, row + 1)) &&
			   !isSolidTile(MapAt(ahead, row + 1));
	if (isSolidTile(next) || next.type == Ladder || isFloor(MapAt(ahead, row - 1)))
		return false;
	for (int k = 2; k <= 3; k++) { // a gap: a floor a cell or two beyond the hole
		const int beyond = col + k * dir;
		if (!IsInBounds(beyond, row) || !IsInBounds(beyond, row - 1) || isSolidTile(MapAt(beyond, row)))
			return false;
		if (isFloor(MapAt(beyond, row - 1)))
			return true;
	}
	return false;
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
