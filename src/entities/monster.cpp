#include "monster.h"
#include <algorithm>
#include <cmath>

#include "player.h"
#include "figures.h"
#include "../graphics/render_config.h"
#include "../core/gameplay_config.h"

void Monster::Spawn(const MonsterType& kind, int spawnCol, int spawnRow, const MonsterLinks& world, Rng& effects) {
	links = world;
	type = &kind;
	if (!blood)
		blood = std::make_unique<ParticleSystem>();
	blood->setBloodColor(kind.blood.r, kind.blood.g, kind.blood.b);
	blood->Stop(); // no splash until the first hit
	col = spawnCol;
	row = spawnRow;
	health = kind.maxHealth;
	x = 0.f;
	alerted = false;
	minion = false;
	summonMs = -1;
	state = flies() || lurking() ? ModelState::Idle : ModelState::Move;
	tomb = entombed() ? MUMMY_COFFIN_DEPTH : 0.f;
	inWater = false;
	headInWater = false;
	sink = 0.f;
	swim = 0.f;
	swimPlaced = false;
	facing = 0;
	fleeDir = 0;
	flight = Flight{};
	leap = Leap{};
	burrow = Burrow{};
	charge = Charge{};
	trapHurt = TrapHurt{};
	trapDamageCarry = 0;
	spitReadyMs = 0;
	spitReleased = true;
	drop.reset();
	poison.Cure();
	poisonByPlayer = false;
	playback = kind.model.SpawnPlayback(effects);
	attackTimer.SetInterval(kind.attackMs); // a slot can respawn another kind
	if (!spawned) {
		stepTimer.Reset();
		attackTimer.Reset();
		spawned = true;
	}
}

void Monster::Clear() {
	type = nullptr;
	col = -1;
	row = -1;
	health = 0;
	drop.reset();
}

void Monster::MakeMinion(Summon how) {
	minion = true;
	alerted = true;
	summonMs = GameClock::now();
	summonedBy = how;
	if (entombed() && how == Summon::Coffin) // lies in the coffin it was summoned into, climbs out (tomb, Rising)
		wake();
	else if (rises()) // no coffin to climb out of; a cobra summoned comes out reared up
		enter(ModelState::Move);
	if (flies() && how == Summon::Drop) { // falls out of the ceiling to where bats turn, flapping (emergeLift)
		flight.lift = BAT_HIGH_LIFT;
		enter(ModelState::Move);
	} else if (flies())
		flight.lift = roostLift();
}

bool Monster::Emerging() const {
	return summonMs >= 0 && summonedBy != Summon::Coffin && GameClock::now() - summonMs < MINION_EMERGE_MS;
}

// Digging out it rises from its full height under the floor, slowing at the top. Dropping it falls from the
// ceiling, where a roosting flyer hangs out of sight, speeding up.
float Monster::emergeLift() const {
	if (!Emerging())
		return 0.f;
	const float p = static_cast<float>(GameClock::now() - summonMs) / static_cast<float>(MINION_EMERGE_MS);
	if (summonedBy == Summon::Drop)
		return (BAT_CEILING - flight.lift) * (1.f - p * p);
	return -type->model.referenceTop * type->scale * Figures::Scale() * (1.f - p) * (1.f - p);
}

float Monster::swimLift() const {
	if (!inWater || !headInWater || type->wading != Wading::Swimmer || !Alive())
		return 0.f;
	const float drawScale = type->scale * Figures::Scale();
	const float top = lurking() ? type->model.idleTop * drawScale - SUBMERGED_SHOW
								: type->model.referenceTop * drawScale - 2.f * SUBMERGED_SHOW;
	return std::max(0.f, RenderConfig::WATER_DEPTH - top);
}

float Monster::burrowLift() const {
	const float depth = -type->model.referenceTop * type->scale * Figures::Scale();
	const int age = GameClock::now() - burrow.startMs;
	switch (burrow.phase) {
	case BurrowPhase::Up:
		return 0.f;
	case BurrowPhase::Dive: {
		const float p = std::min(static_cast<float>(age) / static_cast<float>(BURROW_DIVE_MS), 1.f);
		return depth * p * p;
	}
	case BurrowPhase::Under:
		return depth;
	case BurrowPhase::Surface: {
		const float p = std::min(static_cast<float>(age) / static_cast<float>(BURROW_SURFACE_MS), 1.f);
		return depth * (1.f - p) * (1.f - p);
	}
	}
	return 0.f;
}

float Monster::lift() const {
	return (flies() ? std::max(flight.lift, 0.f) : leap.lift + swim - sink) + emergeLift() + burrowLift();
}

bool Monster::LeavesChest() const {
	return type->locomotion == Locomotion::Ambush && !Alive() && state == ModelState::Die &&
		   type->model.Finished(state, playback);
}

std::optional<ItemKind> Monster::TakeDrop() {
	if (!drop || Alive() || state != ModelState::Die || !type->model.Finished(state, playback))
		return std::nullopt;
	std::optional<ItemKind> d = drop;
	drop.reset();
	return d;
}

bool Monster::Rising() const {
	return rises() && Alive() && state == ModelState::Rise && !type->model.Finished(state, playback);
}

bool Monster::sameRow(float py) const { return std::fabs(static_cast<float>(row) - py) < 0.8f; }

float Monster::HalfWidth() const {
	return type->model.HalfWidth() * type->scale * Figures::Scale() / RenderConfig::TILE_SIZE;
}

float Monster::MeleeGap(float px, int dir) const {
	return (NearEdge(dir) - px) * static_cast<float>(dir) - links.player->HalfWidth();
}

bool Monster::Nearby(float px, float py, float reach, int dir) const {
	const float behind = (FarEdge(dir) - px) * static_cast<float>(dir); // < 0: the far edge is behind the player
	return MeleeGap(px, dir) <= reach && behind >= -MELEE_REACH_BEHIND &&
		   std::fabs(static_cast<float>(row) - py) < 0.7f;
}

// A roosting flyer, a lurker under the water and a coiled one show their idle clip.
float Monster::BottomY() const {
	const bool idle = (flies() && flight.phase == FlightPhase::Roost) || ((submerged() || coiled()) && lurking());
	const float bottom = idle ? type->model.idleBottom * type->scale * Figures::Scale() : 0.f;
	return static_cast<float>(row) + (lift() + bottom) / RenderConfig::TILE_SIZE;
}

float Monster::TopY() const {
	const bool idle = (flies() && flight.phase == FlightPhase::Roost) || ((submerged() || coiled()) && lurking());
	const float top = (idle ? type->model.idleTop : type->model.referenceTop) * type->scale * Figures::Scale();
	return static_cast<float>(row) + (lift() + top) / RenderConfig::TILE_SIZE;
}

bool Monster::takeHit(int dmg) {
	const int scale = static_cast<int>(type->scale);
	if (lurking())
		wake();
	alerted = true;
	if (Alive()) {
		health -= dmg;
		blood->Splash(scale);
	}

	if (Alive() || state == ModelState::Die)
		return false;
	enter(ModelState::Die);
	sound(CharacterSound::Die);

	// Death blood effect, stronger than a hit.
	blood->Splash(scale);
	for (int i = 0; i < 6; i++)
		blood->Explode();
	return true;
}

bool Monster::TakeWeaponHit(int dmg, const DamageMix& mix) {
	const int hit = resistedDamage(dmg, mix, type->resist);
	return takeHit(Stunned() ? hit * CHARGE_STUN_DAMAGE_FACTOR : hit);
}

void Monster::StandInTrap() {
	if (const int dmg = trapHurt.hit(); dmg > 0)
		TrapHit(dmg);
}

void Monster::TrapHit(int dmg) {
	const int hundredths = dmg * type->trapDamagePct + trapDamageCarry;
	trapDamageCarry = hundredths % 100;
	if (hundredths >= 100)
		takeHit(hundredths / 100); // no reward: the player must not farm kills with traps
}

bool Monster::TakePoison(PoisonTier tier, bool byPlayer, Rng& rng) {
	if (!Alive() || (type->poisonResistPercent > 0 && rng.percent(type->poisonResistPercent)))
		return false;
	poisonByPlayer = byPlayer || (poisonByPlayer && poison.Any());
	poison.Apply(tier);
	return true;
}

bool Monster::UpdatePoison() {
	if (!Alive()) {
		poison.Cure();
		return false;
	}
	const int hp = poison.Advance(UPDATE_TICK_MS);
	if (hp <= 0 || !takeHit(hp)) // armour does not help, as on the player
		return false;
	poison.Cure();
	return poisonByPlayer;
}

// The blood runs twice a tick while alive (and is drawn twice), once when dead: as it always has.
void Monster::Animate(float px, float py) {
	if (Alive()) {
		if (state == ModelState::Die) {
			if (flies())
				enter(flight.phase == FlightPhase::Roost ? ModelState::Idle : ModelState::Move);
			else if (lurking())
				enter(ModelState::Idle);
			else if (!attackDirection(px, py))
				enter(ModelState::Attack);
			else
				enter(ModelState::Move);
		}
		blood->Explode();
		blood->Fall();
	} else
		enter(ModelState::Die);
	blood->Explode();
	blood->Fall();

	if (Alive()) {
		if (jumping())
			facing = leap.toX > leap.fromX ? 1 : -1;
		else if (Charging())
			facing = charge.dir; // runs on past the player
		else if ((submerged() && lurking()) || (coiled() && (lurking() || Rising())))
			facing = px < CentreX() ? -1 : 1; // lies along the row (in its coil), watching the player
		else if (lurking() || Rising())
			facing = 0; // a chest doesn't turn to look at the player, a mummy lies along its coffin
		else if (fleeDir != 0)
			facing = fleeDir; // runs from the player
		else if (!flies()) {
			facing = attackDirection(px, py);
			if (facing == 0 && sameRow(py) && !rooted()) // biting: turned to the player, the jaws at them (the box)
				facing = px < CentreX() ? -1 : 1;
		} else
			facing = flight.phase == FlightPhase::Roost ? 0 : flight.dir;
	}
	type->model.Advance(state, playback);
	// A swimmer floats up as it wades in and sinks back to the floor on the bank; placed at once when it spawns.
	const float swimTarget = swimLift();
	if (!swimPlaced)
		swim = swimTarget;
	swimPlaced = true;
	swim += (swimTarget - swim) * std::min(1.f, SWIM_LIFT_RATE * static_cast<float>(UPDATE_TICK_MS) / 1000.f);
	if (Alive() && entombed() && !lurking()) { // climbing out; a mummy killed on the way stays where it fell
		const float t = state == ModelState::Rise ? type->model.Progress(state, playback) : 1.f;
		const float k = std::clamp((t - MUMMY_CLIMB_FROM) / (MUMMY_CLIMB_TO - MUMMY_CLIMB_FROM), 0.f, 1.f);
		tomb = MUMMY_COFFIN_DEPTH * (1.f - k * k * (3.f - 2.f * k));
	}
}

bool LoadMonsterTypes(MonsterTypes& types) {
	bool loaded = true;
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& kind = *monsterKind(id);
		MonsterType& type = types[static_cast<size_t>(id)];
		static_cast<MonsterKind&>(type) = kind;
		loaded = LoadClips(kind.model, ClipFilesOf(kind), type.model) && loaded;
	}
	return loaded;
}
