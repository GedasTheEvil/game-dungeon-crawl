// Chargers (Sobek, the hippo, Seth): the windup, the rush along the row, the stun in a wall and Seth's storm roar.
#include "monster.h"
#include <cmath>
#include "player.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"

bool Monster::canCharge(float px, float py) const {
	if (!type->charge || !Alive() || !alerted || Charging() || !sameRow(py) || GameClock::now() < charge.nextMs)
		return false;
	const float d = std::fabs(px - CentreX());
	return d >= type->charge->minRange && d <= type->charge->maxRange;
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

bool Monster::UpdateCharge(bool wallAhead, float px, float py) {
	const int now = GameClock::now();
	const int age = now - charge.startMs;
	switch (charge.phase) {
	case ChargePhase::Ready:
		return false;
	case ChargePhase::Windup:
		if (age >= CHARGE_WINDUP_MS) {
			charge.phase = ChargePhase::Rush;
			charge.startMs = now;
			enter(ModelState::Move);
		}
		return false;
	case ChargePhase::Rush: {
		if (wallAhead) {
			charge.phase = ChargePhase::Stunned;
			charge.startMs = now;
			enter(ModelState::Idle);
			sound(CharacterSound::Die);
			return false;
		}
		x += static_cast<float>(charge.dir) * type->charge.value_or(ChargeRules{}).speed *
			 static_cast<float>(UPDATE_TICK_MS) / 1000.f;
		const float head = HeadX();
		const bool onFloor = py - static_cast<float>(row) < CHARGE_JUMP_CLEAR; // a jump takes the player over it
		if (!charge.hit && sameRow(py) && onFloor && std::fabs(px - head) <= links.player->HalfWidth() + 0.1f) {
			charge.hit = true;
			if (!links.player->Dodges(*links.events, *links.random)) {
				links.player->TakeHit(type->damage * CHARGE_HIT_FACTOR, type->attackMix, *links.events);
				curse(type->meleeCurses);
				links.journal->HitByCreature(type->id, links.level);
			}
			sound(CharacterSound::Attack);
		}
		if ((px - head) * static_cast<float>(charge.dir) < -CHARGE_OVERRUN) { // far past the player: it stops
			charge.phase = ChargePhase::Ready;
			charge.nextMs = now + CHARGE_COOLDOWN_MS;
		}
		return false;
	}
	case ChargePhase::Stunned:
		if (age < CHARGE_STUN_MS)
			return false;
		charge.phase = ChargePhase::Ready;
		charge.nextMs = now + CHARGE_COOLDOWN_MS;
		if (!type->roar) {
			enter(ModelState::Move);
			return false;
		}
		enter(ModelState::Attack); // the roar: the threat yawn
		sound(CharacterSound::Roar);
		links.journal->SeeMove(type->id, links.level, CreatureMove::Roar);
		return true;
	}
	return false;
}
