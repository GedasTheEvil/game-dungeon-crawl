// Grabbers (HoldRules; plans crocodile-hold-bite, giant-scorpion-grab): the tell, the lunge, the hold and its end.
#include "monster.h"
#include <algorithm>
#include <cmath>
#include "player.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"

bool Monster::StartGrab() {
	if (!type->hold || Grabbing())
		return false;
	enter(ModelState::Attack);
	grab = Grab{GrabPhase::Tell, GameClock::now()};
	sound(CharacterSound::Wake);
	return true;
}

void Monster::UpdateGrab(float px, float py, bool playerWading) {
	const HoldRules rules = Hold();
	Player& player = *links.player;
	const int now = GameClock::now();
	if (!player.Alive()) {
		letGo();
		return;
	}
	if (grab.phase == GrabPhase::Tell) {
		if (now - grab.startMs < GRAB_TELL_MS)
			return;
		sound(CharacterSound::Attack);
		const bool onFloor = py - static_cast<float>(row) < GRAB_JUMP_CLEAR;
		const bool caught = sameRow(py) && attackDirection(px, py) == 0 && onFloor && (!rules.inWater || playerWading);
		if (!caught || player.Dodges(*links.events, *links.random)) { // backed off, jumped away: the jaws snap shut
			letGo();
			return;
		}
		player.TakeHit(type->damage * rules.grabDamagePct / 100, rules.grabMix.value_or(type->attackMix),
					   *links.events);
		curse(type->meleeCurses);
		links.journal->HitByCreature(type->id, links.level);
		links.journal->SeeMove(type->id, links.level, CreatureMove::Grab);
		links.events->Status("%s", rules.breakMs > 0 ? "Caught in its claws: hit them!" : "Caught in its jaws");
		grab = Grab{GrabPhase::Hold, now, now + rules.holdMs, 0};
		return;
	}
	if (!sameRow(py)) {
		letGo();
		return;
	}
	// The death roll: a tick every tickMs, the last one as it lets go.
	while (rules.tickMs > 0 && (grab.ticks + 1) * rules.tickMs <= std::min(now, grab.endMs) - grab.startMs) {
		grab.ticks++;
		player.TakeHit(type->damage * rules.tickDamagePct / 100, type->attackMix, *links.events);
		sound(CharacterSound::Attack);
	}
	if (now < grab.endMs)
		return;
	if (rules.sting)
		sting();
	letGo();
}

void Monster::sting() {
	Player& player = *links.player;
	player.TakeHit(type->damage, type->attackMix, *links.events);
	curse(type->meleeCurses);
	if (type->poison) {
		player.Poison(*type->poison, *links.events, *links.random);
		links.journal->SeeMove(type->id, links.level, CreatureMove::Poison);
	}
	sound(CharacterSound::Attack);
}

void Monster::letGo() {
	grab = Grab{};
	attackTimer.Reset();
}

void Monster::Struggle() {
	const int breakMs = Hold().breakMs;
	if (!Holding() || breakMs <= 0)
		return;
	grab.endMs -= breakMs;
	if (GameClock::now() < grab.endMs)
		return;
	letGo();
	links.events->Status("You break free");
}

int Monster::HoldAgeMs() const { return Holding() ? GameClock::now() - grab.startMs : 0; }

float Monster::HoldRoll() const {
	const int tickMs = Hold().tickMs;
	if (!Holding() || tickMs <= 0)
		return 0.f;
	const float t = static_cast<float>(HoldAgeMs() % tickMs) / static_cast<float>(tickMs);
	return 360.f * t * t * (3.f - 2.f * t); // a whole turn, slow out of the last tick, fast, slowing into the next
}
