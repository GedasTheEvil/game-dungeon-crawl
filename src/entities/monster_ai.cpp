#include "monster.h"
#include <algorithm>
#include <cmath>
#include "player.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"
#include "figures.h"
#include "../graphics/render_config.h"

int Monster::attackDirection(float px, float py) const {
	if (!sameRow(py))
		return 0;
	const float reach = rooted() ? ROOTED_BITE_REACH : MONSTER_BITE_REACH;
	const float player = links.player->HalfWidth();
	if (Left() - (px + player) > reach)
		return -1;
	if ((px - player) - Right() > reach)
		return 1;
	return 0;
}

bool Monster::Threatens(float px, float py) const {
	if (!Active() || !Alive() || !alerted || lurking() || Emerging() || !sameRow(py))
		return false;
	return !rooted() || attackDirection(px, py) == 0;
}

bool Monster::Seek(bool blocked, float px, float py) {
	if (!Alive())
		return false;
	int dir = attackDirection(px, py);
	if (dir == 0)
		return false;

	if (!rooted())
		alerted = true; // chasing the player
	const float water = inWater ? type->waterSpeed : 1.f;
	if (!blocked)
		x += MONSTER_SEEK_STEP * static_cast<float>(dir) * type->speed * water;
	enter(ModelState::Move);
	return true;
}

void Monster::Attack(float py) {
	if (!Alive() || type->damage <= 0) // harmless (an egg cluster)
		return;

	enter(ModelState::Attack);
	if (sameRow(py)) {
		alerted = true;
		bite();
	}
}

void Monster::bite() {
	const int lost = links.player->TakeHit(type->damage, type->attackMix, *links.events);
	if (type->poison) {
		links.player->Poison(*type->poison, *links.events, *links.random);
		links.journal->SeeMove(type->id, links.level, CreatureMove::Poison);
	}
	health = std::min(type->maxHealth, health + lost * type->boss.lifeStealPct / 100);
	links.journal->HitByCreature(type->id, links.level);
	if (type->boss.lifeStealPct > 0 && lost > 0)
		links.journal->SeeMove(type->id, links.level, CreatureMove::Heal);
	sound(CharacterSound::Attack);
}

bool Monster::Lurk(float px, float py) {
	if (!lurking())
		return false;
	const float range = entombed()	  ? MUMMY_WAKE_RANGE
						: submerged() ? (type->isBoss() ? BOSS_WAKE_RANGE : SUBMERGED_WAKE_RANGE)
						: coiled()	  ? COILED_WAKE_RANGE
									  : MIMIC_WAKE_RANGE;
	if (!sameRow(py) || std::fabs(px - CentreX()) > range) {
		enter(ModelState::Idle);
		return true;
	}
	wake();
	return false;
}

void Monster::wake() {
	alerted = true;
	const CreatureMove move = entombed()	? CreatureMove::Rise
							  : submerged() ? CreatureMove::Surface
							  : coiled()	? CreatureMove::Rear
											: CreatureMove::Ambush;
	links.journal->SeeMove(type->id, links.level, move);
	enter(rises() ? ModelState::Rise : ModelState::Move);
	sound(CharacterSound::Wake);
}

bool Monster::canSpit(float px, float py) const {
	if (!type->spit || !Alive() || !alerted || lurking() || GameClock::now() < spitReadyMs)
		return false;
	const int dir = attackDirection(px, py); // 0: in bite reach or not on its row
	const float gap = dir > 0 ? (px - links.player->HalfWidth()) - Right() : Left() - (px + links.player->HalfWidth());
	return dir != 0 && gap <= type->spit->range;
}

void Monster::Spit() {
	if (!type->spit)
		return;
	enter(ModelState::Spit);
	spitReadyMs = GameClock::now() + type->spit->cooldownMs;
	spitReleased = false;
	sound(CharacterSound::Spit);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Spit);
}

bool Monster::Spitting() const {
	return Alive() && state == ModelState::Spit && !type->model.Finished(state, playback);
}

bool Monster::TakeSpit(float& outX, float& outY) {
	if (!type->spit || spitReleased || !Spitting() ||
		type->model.Progress(state, playback) < type->spit->release)
		return false;
	spitReleased = true;
	outX = HeadX();
	outY = static_cast<float>(row) +
		   (lift() + type->spit->mouthY * type->model.referenceTop * type->scale * Figures::Scale()) /
			   RenderConfig::TILE_SIZE;
	return true;
}

bool Monster::canDive(float py) const {
	return type->locomotion == Locomotion::Burrow && Alive() && alerted && !Burrowing() && sameRow(py) &&
		   GameClock::now() >= burrow.nextMs;
}

void Monster::Dive(float toX) {
	burrow.phase = BurrowPhase::Dive;
	burrow.startMs = GameClock::now();
	burrow.toX = toX;
	enter(ModelState::Move);
	sound(CharacterSound::Wake);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Burrow);
}

void Monster::DelayDive() { burrow.nextMs = GameClock::now() + BURROW_RETRY_MS; }

bool Monster::UpdateBurrow() {
	const int now = GameClock::now();
	const int age = now - burrow.startMs;
	switch (burrow.phase) {
	case BurrowPhase::Up:
		return false;
	case BurrowPhase::Dive:
		if (age >= BURROW_DIVE_MS) {
			burrow.phase = BurrowPhase::Under;
			burrow.startMs = now;
		}
		return false;
	case BurrowPhase::Under:
		if (age < BURROW_UNDER_MS)
			return false;
		x = burrow.toX;
		burrow.phase = BurrowPhase::Surface;
		burrow.startMs = now;
		sound(CharacterSound::Wake);
		return true;
	case BurrowPhase::Surface:
		if (age >= BURROW_SURFACE_MS) {
			burrow.phase = BurrowPhase::Up;
			burrow.nextMs = now + BURROW_EVERY_MS;
		}
		return false;
	}
	return false;
}

bool Monster::canCharge(float px, float py) const {
	if (!type->charges || !Alive() || !alerted || Charging() || !sameRow(py) || GameClock::now() < charge.nextMs)
		return false;
	const float d = std::fabs(px - CentreX());
	return d >= CHARGE_MIN && d <= CHARGE_MAX;
}

void Monster::StartCharge(float px) {
	charge.phase = ChargePhase::Windup;
	charge.startMs = GameClock::now();
	charge.dir = px < CentreX() ? -1 : 1;
	charge.hit = false;
	enter(ModelState::Attack);
	sound(CharacterSound::Wake);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Charge);
}

void Monster::UpdateCharge(bool wallAhead, float px, float py) {
	const int now = GameClock::now();
	const int age = now - charge.startMs;
	switch (charge.phase) {
	case ChargePhase::Ready:
		return;
	case ChargePhase::Windup:
		if (age >= CHARGE_WINDUP_MS) {
			charge.phase = ChargePhase::Rush;
			charge.startMs = now;
			enter(ModelState::Move);
		}
		return;
	case ChargePhase::Rush: {
		if (wallAhead) {
			charge.phase = ChargePhase::Stunned;
			charge.startMs = now;
			enter(ModelState::Idle);
			sound(CharacterSound::Die);
			return;
		}
		x += static_cast<float>(charge.dir) * CHARGE_SPEED * static_cast<float>(UPDATE_TICK_MS) / 1000.f;
		const float head = HeadX();
		const bool onFloor = py - static_cast<float>(row) < CHARGE_JUMP_CLEAR; // a jump takes the player over it
		if (!charge.hit && sameRow(py) && onFloor && std::fabs(px - head) <= links.player->HalfWidth() + 0.1f) {
			charge.hit = true;
			links.player->TakeHit(type->damage * CHARGE_HIT_FACTOR, type->attackMix, *links.events);
			links.journal->HitByCreature(type->id, links.level);
			sound(CharacterSound::Attack);
		}
		if ((px - head) * static_cast<float>(charge.dir) < -CHARGE_OVERRUN) { // far past the player: it stops
			charge.phase = ChargePhase::Ready;
			charge.nextMs = now + CHARGE_COOLDOWN_MS;
		}
		return;
	}
	case ChargePhase::Stunned:
		if (age >= CHARGE_STUN_MS) {
			charge.phase = ChargePhase::Ready;
			charge.nextMs = now + CHARGE_COOLDOWN_MS;
			enter(ModelState::Move);
		}
		return;
	}
}

float Monster::seekProbeX(int dir) const { return CentreX() + static_cast<float>(dir) * HalfWidth(); }

float Monster::flightProbeX() const {
	int dir = flight.dir;
	if (flight.phase == FlightPhase::Return)
		dir = x > 0 ? -1 : 1;
	return CentreX() + static_cast<float>(dir) * BAT_WALL_MARGIN;
}

float Monster::roostLift() const { return BAT_CEILING - type->model.idleTop * type->scale; }

void Monster::Fly(bool wallAhead, float px, float py) {
	const float scale = type->scale;
	const int now = GameClock::now();
	const float dt = flight.lastMs < 0 ? 0.f : static_cast<float>(std::min(now - flight.lastMs, 100)) / 1000.f;
	flight.lastMs = now;
	const float roost = roostLift();
	if (flight.lift < 0)
		flight.lift = roost;

	if (!Alive()) { // drops to the floor
		flight.fall += BAT_FALL_GRAVITY * dt;
		flight.lift = std::max(0.f, flight.lift - flight.fall * dt);
		return;
	}

	const float dx = px - CentreX(); // to the player
	// A boss's minion, and a boss once it is roused, hunt the player along the whole row.
	const bool hunts = minion || (alerted && type->isBoss());
	const bool sees = sameRow(py) && (hunts || std::fabs(dx) <= BAT_SIGHT);
	const float step = BAT_TILES_PER_SPEED * type->speed * dt;
	float targetLift = roost;

	switch (flight.phase) {
	case FlightPhase::Roost:
		if (!sees && flight.lift < roost - 1.f) { // dropped from the ceiling, no one to hunt: back up
			enter(ModelState::Move);
			break;
		}
		if (!sees) {
			enter(ModelState::Idle);
			return;
		}
		flight.phase = FlightPhase::Swoop;
		flight.dir = dx >= 0 ? 1 : -1;
		flight.bitten = false;
		alerted = true;
		links.journal->SeeMove(type->id, links.level, CreatureMove::Swoop);
		[[fallthrough]];
	case FlightPhase::Swoop: {
		const float ahead = dx * static_cast<float>(flight.dir); // > 0: the player is still in front
		if (!flight.bitten && ahead <= BAT_BITE_REACH && ahead > -BAT_OVERSHOOT) {
			flight.bitten = true;
			flight.attackUntilMs = now + BAT_ATTACK_MS;
			if (sameRow(py))
				bite();
		}
		if (wallAhead || ahead < -BAT_OVERSHOOT) {
			flight.dir = -flight.dir;
			flight.bitten = false;
			if (!sees && std::fabs(dx) > BAT_OVERSHOOT + 0.5f) // lost the player (climbed away, ran off)
				flight.phase = FlightPhase::Return;
		} else
			x += static_cast<float>(flight.dir) * step;
		// Swoops down to the player, climbs away from them.
		const float k = std::min(std::fabs(dx) / BAT_OVERSHOOT, 1.f);
		const float low = BAT_LOW_LIFT + BAT_WING_DIP * scale;
		targetLift = low + (std::max(BAT_HIGH_LIFT, low) - low) * k * k * (3 - 2 * k);
		enter(now < flight.attackUntilMs ? ModelState::Attack : ModelState::Move);
		break;
	}
	case FlightPhase::Return:
		if (sees) {
			flight.phase = FlightPhase::Swoop;
			flight.dir = dx >= 0 ? 1 : -1;
			break;
		}
		flight.dir = x > 0 ? -1 : 1;
		if (std::fabs(x) <= step) {
			x = 0;
			if (std::fabs(flight.lift - roost) < 1.f)
				flight.phase = FlightPhase::Roost;
		} else if (!wallAhead)
			x += static_cast<float>(flight.dir) * step;
		enter(ModelState::Move);
		break;
	}
	flight.lift += (targetLift - flight.lift) * std::min(1.f, BAT_LIFT_RATE * dt);
}

bool Monster::canJump() const {
	return type->locomotion == Locomotion::WalkJump && health > 0 && !jumping() && GameClock::now() >= leap.readyMs;
}

void Monster::Jump(float toX) {
	int now = GameClock::now();
	leap.fromX = x;
	leap.toX = toX;
	leap.startMs = now;
	leap.readyMs = now + MONSTER_JUMP_COOLDOWN_MS;
	leap.lift = 0.f;
	enter(ModelState::Jump);
	sound(CharacterSound::Jump);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Leap);
}

void Monster::UpdateJump() {
	float t = static_cast<float>(GameClock::now() - leap.startMs) / static_cast<float>(MONSTER_JUMP_MS);
	if (t >= 1.f) {
		x = leap.toX;
		leap.lift = 0.f;
		leap.startMs = -1;
		if (Alive())
			enter(ModelState::Move);
		return;
	}
	float air = std::clamp((t - MONSTER_JUMP_TAKEOFF) / (MONSTER_JUMP_TOUCHDOWN - MONSTER_JUMP_TAKEOFF), 0.f, 1.f);
	x = leap.fromX + (leap.toX - leap.fromX) * air;
	leap.lift = 4.f * MONSTER_JUMP_HEIGHT * air * (1.f - air);
}
