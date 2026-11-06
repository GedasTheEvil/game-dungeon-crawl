// Burrowers (Apep, the worm in sand): the dive, the time under the floor and the surfacing. Where it comes up:
// Dungeon::burrowTarget, Dungeon::sandTarget (dungeon_burrow.cpp).
#include "monster.h"
#include <algorithm>
#include <cmath>
#include "figures.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"

int Monster::underMs() const { return type->sandBurrow ? type->sandBurrow->underMs : BURROW_UNDER_MS; }

bool Monster::burrowClip() const { return type->sandBurrow && type->model.Clip(ModelState::Rise).present; }

float Monster::burrowProgress() const {
	const int ms = burrow.phase == BurrowPhase::Dive ? BURROW_DIVE_MS : BURROW_SURFACE_MS;
	return std::min(static_cast<float>(GameClock::now() - burrow.startMs) / static_cast<float>(ms), 1.f);
}

float Monster::burrowLift() const {
	const float depth = -type->model.referenceTop * type->scale * Figures::Scale();
	switch (burrow.phase) {
	case BurrowPhase::Up:
		return 0.f;
	case BurrowPhase::Dive: {
		if (burrowClip()) // the clip takes it down
			return 0.f;
		const float p = burrowProgress();
		return depth * p * p;
	}
	case BurrowPhase::Under:
		return depth;
	case BurrowPhase::Surface: {
		if (burrowClip())
			return 0.f;
		const float p = burrowProgress();
		return depth * (1.f - p) * (1.f - p);
	}
	}
	return 0.f;
}

void Monster::showBurrow() {
	if (!burrowClip() || (burrow.phase != BurrowPhase::Dive && burrow.phase != BurrowPhase::Surface))
		return;
	const float p = burrowProgress();
	const auto last = static_cast<float>(type->model.Clip(ModelState::Rise).frames - 1);
	playback[static_cast<size_t>(ModelState::Rise)] = {last * (burrow.phase == BurrowPhase::Dive ? 1.f - p : p), 0};
}

bool Monster::canDive(float py) const {
	return (type->locomotion == Locomotion::Burrow || type->sandBurrow) && Alive() && alerted && !Burrowing() &&
		   sameRow(py) && GameClock::now() >= burrow.nextMs;
}

void Monster::Dive(float toX) {
	burrow.phase = BurrowPhase::Dive;
	burrow.startMs = GameClock::now();
	burrow.fromX = x;
	burrow.toX = toX;
	burrow.dives++;
	enter(burrowClip() ? ModelState::Rise : ModelState::Move);
	sound(CharacterSound::Wake);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Burrow);
}

void Monster::DelayDive() { burrow.nextMs = GameClock::now() + BURROW_RETRY_MS; }

void Monster::BuryInSand() {
	burrow.phase = BurrowPhase::Under;
	burrow.lurk = true;
	burrow.fromX = burrow.toX = x;
	summonMs = -1; // a minion summoned into sand lies in wait at once, no digging out, no health bar till Surge
	alerted = false;
	enter(ModelState::Idle);
}

void Monster::Surge(float toX) {
	burrow.lurk = false;
	burrow.startMs = GameClock::now();
	burrow.fromX = x;
	burrow.toX = toX;
	alerted = true;
	enter(ModelState::Move);
	sound(CharacterSound::Wake);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Burrow);
}

std::optional<float> Monster::BurrowRipple() const {
	if (burrow.phase != BurrowPhase::Under)
		return std::nullopt;
	const float base = static_cast<float>(col) + 0.5f;
	if (burrow.lurk) // lying in wait: the sand stirs a little to and fro
		return base + x + SAND_RIPPLE_SWAY * std::sin(static_cast<float>(GameClock::now()) * 0.002f);
	const float p = std::min(
		static_cast<float>(GameClock::now() - burrow.startMs) / static_cast<float>(underMs()),
		1.f
	);
	return base + burrow.fromX + (burrow.toX - burrow.fromX) * p;
}

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
		if (burrow.lurk)
			return false;
		if (type->sandBurrow) // under the sand it moves along to where it comes up
			x = burrow.fromX +
				(burrow.toX - burrow.fromX) * std::min(static_cast<float>(age) / static_cast<float>(underMs()), 1.f);
		if (age < underMs())
			return false;
		x = burrow.toX;
		burrow.phase = BurrowPhase::Surface;
		burrow.startMs = now;
		if (burrowClip())
			enter(ModelState::Rise);
		sound(CharacterSound::Wake);
		return true;
	case BurrowPhase::Surface:
		if (age >= BURROW_SURFACE_MS) {
			burrow.phase = BurrowPhase::Up;
			burrow.nextMs = now + (type->sandBurrow ? type->sandBurrow->everyMs : BURROW_EVERY_MS);
			if (burrowClip())
				enter(ModelState::Move);
		}
		return false;
	}
	return false;
}
