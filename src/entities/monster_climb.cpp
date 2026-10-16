// Climbers on a ladder (plan monster-climbers): the path walk, the climb and its clip, the fall when killed on the
// rungs. The path itself: Dungeon::followPath (dungeon_walkers.cpp).
#include "monster.h"
#include <algorithm>
#include <cmath>
#include "figures.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"
#include "../graphics/render_config.h"

void Monster::WalkPath(int dir, bool blocked) {
	if (!Alive())
		return;
	fleeDir = 0;
	if (blocked) {
		enter(ModelState::Idle);
		return;
	}
	x += WalkStep() * static_cast<float>(dir);
	enter(ModelState::Move);
}

void Monster::StartClimb(int ladderCol, int toRow) {
	x = static_cast<float>(ladderCol - col);
	climb.toRow = toRow;
	climb.y = 0.f;
	enter(ModelState::Climb);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Climb);
}

void Monster::DropTo(int floorRow) {
	climb.toRow = floorRow;
	climb.falls = true;
}

void Monster::UpdateClimb() {
	const int dir = climb.toRow > row ? 1 : -1;
	const float factor = type->isBoss() ? BOSS_CLIMB_SPEED_FACTOR : CLIMB_SPEED_FACTOR;
	climb.y += static_cast<float>(dir) * (climb.falls ? CLIMBER_FALL_STEP : MONSTER_SEEK_STEP * type->speed * factor);
	if (climb.y * static_cast<float>(dir) >= 1.f) {
		row += dir;
		climb.y -= static_cast<float>(dir);
	}
	if (row == climb.toRow && climb.y * static_cast<float>(dir) >= 0.f)
		climb = Climb{};
	if (Alive())
		enter(ModelState::Climb);
}

void Monster::SawWay(int toCol, int toRow) {
	wayMs = GameClock::now();
	wayCol = toCol;
	wayRow = toRow;
}

bool Monster::HeadingFor(int& outCol, int& outRow) const {
	if (wayMs < 0 || GameClock::now() - wayMs > CLIMB_GIVE_UP_MS)
		return false;
	outCol = wayCol;
	outRow = wayRow;
	return true;
}

float Monster::climbLift() const { return climb.y * RenderConfig::TILE_SIZE; }

float Monster::ClimbDepth() const {
	if (!ShowsClimb() || type->climbRise <= 0.f)
		return 0.f;
	const float grip = type->climbGrip * type->model.referenceTop * type->scale * Figures::Scale();
	return RenderConfig::LADDER_RAIL_Z + grip - RenderConfig::MONSTER_DEPTH;
}

// One cycle of the clip per climbRise of height, so the gripping paws stay on the rungs; it runs backwards going down
// and holds when it stops.
void Monster::showClimb() {
	const ClipInfo& clip = type->model.Clip(ModelState::Climb);
	const float cycle = type->climbRise * type->model.referenceTop * type->scale * Figures::Scale();
	if (!clip.present || clip.frames < 1 || cycle <= 0.f)
		return;
	const float height = static_cast<float>(row) * RenderConfig::TILE_SIZE + climbLift();
	const float phase = height / cycle - std::floor(height / cycle);
	const auto frames = static_cast<float>(clip.frames);
	playback[static_cast<size_t>(ModelState::Climb)] = {std::min(phase * frames, frames - 0.001f), 0};
}
