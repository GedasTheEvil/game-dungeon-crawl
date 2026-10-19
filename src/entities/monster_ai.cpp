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
	fleeDir = 0;
	if (!blocked)
		x += WalkStep() * static_cast<float>(dir);
	enter(ModelState::Move);
	return true;
}

float Monster::WalkStep() const { return MONSTER_SEEK_STEP * type->speed * (inWater ? type->waterSpeed : 1.f); }

bool Monster::flees() const {
	const bool walks = type->locomotion == Locomotion::Walk || type->locomotion == Locomotion::WalkJump;
	return walks && !reckless() && !type->isBoss();
}

void Monster::Flee(bool blocked, int dir) {
	if (!Alive())
		return;
	alerted = true;
	if (blocked) { // its own dead end: it waits there
		Stand();
		return;
	}
	fleeDir = dir;
	x += WalkStep() * static_cast<float>(dir);
	enter(ModelState::Move);
}

void Monster::Stand() {
	fleeDir = 0;
	enter(ModelState::Idle);
}

void Monster::Attack(float py, bool playerWading) {
	if (!Alive() || type->damage <= 0) // harmless (an egg cluster)
		return;

	fleeDir = 0;
	enter(ModelState::Attack);
	if (!sameRow(py))
		return;
	alerted = true;
	const std::optional<HoldRules>& hold = type->hold;
	if (hold && (!hold->inWater || playerWading) && links.random->percent(hold->chancePercent))
		StartGrab();
	else
		bite();
}

void Monster::bite() {
	if (links.player->Dodges(*links.events, *links.random)) { // no damage, no poison
		sound(CharacterSound::Attack);
		return;
	}
	const int lost = links.player->TakeHit(type->damage, type->attackMix, *links.events);
	curse(type->meleeCurses);
	if (type->poison && (type->poisonChancePercent >= 100 || links.random->percent(type->poisonChancePercent))) {
		links.player->Poison(*type->poison, *links.events, *links.random);
		links.journal->SeeMove(type->id, links.level, CreatureMove::Poison);
	}
	if (type->staminaDrainPct > 0) { // the blue lotus's sedative, in god mode too (as the poison)
		PlayerStats& stats = links.player->stats;
		stats.SetStamina(stats.Stamina() - stats.MaxStamina() * type->staminaDrainPct / 100);
		links.journal->SeeMove(type->id, links.level, CreatureMove::Sedate);
	}
	health = std::min(type->maxHealth, health + lost * type->boss.lifeStealPct / 100);
	links.journal->HitByCreature(type->id, links.level);
	if (type->boss.lifeStealPct > 0 && lost > 0)
		links.journal->SeeMove(type->id, links.level, CreatureMove::Heal);
	sound(CharacterSound::Attack);
}

void Monster::curse(const CurseEntries& entries) {
	if (links.player->Curse(entries, *links.events, *links.random))
		links.journal->SeeMove(type->id, links.level, CreatureMove::Curse);
}

bool Monster::Lurk(float px, float py) {
	if (!lurking())
		return false;
	const bool ambush = type->locomotion == Locomotion::Ambush; // waits for Spring or a hit, however near the player
	const float range = entombed()	  ? MUMMY_WAKE_RANGE
						: submerged() ? (type->isBoss() ? BOSS_WAKE_RANGE
										 : type->charge ? type->charge->maxRange
														: SUBMERGED_WAKE_RANGE)
						: coiled()	  ? COILED_WAKE_RANGE
						: figurine()  ? FIGURINE_WAKE_RANGE
									  : FLOATING_WAKE_RANGE;
	if (ambush || !sameRow(py) || std::fabs(px - CentreX()) > range) {
		enter(ModelState::Idle);
		return true;
	}
	wake();
	return false;
}

void Monster::Spring() {
	if (lurking() && type->locomotion == Locomotion::Ambush)
		wake();
}

void Monster::wake() {
	alerted = true;
	const CreatureMove move = entombed()	? CreatureMove::Rise
							  : submerged() ? CreatureMove::Surface
							  : coiled()	? CreatureMove::Rear
							  : floats()	? CreatureMove::Bloom
							  : figurine()	? CreatureMove::StepOff
											: CreatureMove::Ambush;
	links.journal->SeeMove(type->id, links.level, move);
	enter(rises() ? ModelState::Rise : ModelState::Move);
	sound(CharacterSound::Wake);
}

bool Monster::canAttackRanged(float px, float py) const {
	if (!type->ranged || !Alive() || !alerted || lurking() || GameClock::now() < rangedReadyMs)
		return false;
	const int dir = attackDirection(px, py); // 0: in bite reach or not on its row
	const float gap = dir > 0 ? (px - links.player->HalfWidth()) - Right() : Left() - (px + links.player->HalfWidth());
	return dir != 0 && gap <= type->ranged->range;
}

void Monster::AttackRanged() {
	if (!type->ranged)
		return;
	enter(ModelState::AttackRanged);
	rangedReadyMs = GameClock::now() + type->ranged->cooldownMs;
	rangedReleased = false;
	sound(CharacterSound::AttackRanged);
	links.journal->SeeMove(type->id, links.level, CreatureMove::AttackRanged);
}

bool Monster::AttackingRanged() const {
	return Alive() && state == ModelState::AttackRanged && !type->model.Finished(state, playback);
}

bool Monster::TakeShot(float px, float& outX, float& outY) {
	if (!type->ranged || rangedReleased || !AttackingRanged() ||
		type->model.Progress(state, playback) < type->ranged->release)
		return false;
	rangedReleased = true;
	const float ahead = CentreX() + static_cast<float>(facing) * type->ranged->releaseAhead * HalfWidth();
	const float edge = px - static_cast<float>(facing) * links.player->HalfWidth();
	outX = facing > 0 ? std::min(ahead, edge) : facing < 0 ? std::max(ahead, edge) : CentreX();
	outY = static_cast<float>(row) +
		   (lift() + type->ranged->releaseY * type->model.referenceTop * type->scale * Figures::Scale()) /
			   RenderConfig::TILE_SIZE;
	return true;
}

float Monster::seekProbeX(int dir) const { return CentreX() + static_cast<float>(dir) * HalfWidth(); }
