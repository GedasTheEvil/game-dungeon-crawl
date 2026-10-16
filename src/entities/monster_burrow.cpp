// Burrowers (Apep): the dive, the time under the floor and the surfacing. Where it comes up: Dungeon::burrowTarget
// (dungeon_burrow.cpp).
#include "monster.h"
#include <algorithm>
#include "figures.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"

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
