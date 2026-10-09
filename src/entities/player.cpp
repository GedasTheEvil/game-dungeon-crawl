#include "player.h"
#include <algorithm>
#include <cmath>
#include "figures.h"
#include "../core/gameplay_config.h"
#include "../graphics/render_config.h"

void Player::SetModel(const ModelInfo& info) {
	model = info;
	for (AnimPlayback& p : playback)
		p.stepStart = GameClock::now();
	state = model.reference;
}

float Player::HalfWidth() const { return model.HalfWidth() * scale * Figures::Scale() / RenderConfig::TILE_SIZE; }

float Player::Height() const { return model.referenceTop * scale * Figures::Scale() / RenderConfig::TILE_SIZE; }

// The blood runs twice a tick while alive (and is drawn twice), once when dead: as it always has.
void Player::Animate() {
	if (Alive()) {
		if (state == ModelState::Die) // healed back to life
			setModelState(ModelState::Idle);
		blood.Explode();
		blood.Fall();
	} else
		setModelState(ModelState::Die);
	blood.Explode();
	blood.Fall();
	if (state != ModelState::Climb) // the climb frame follows the height (showClimb)
		model.Advance(state, playback);
	shownFrame = static_cast<int>(playback[static_cast<int>(model.Shown(state))].frame);
}

int Player::TakeHit(int dmg, const DamageMix& mix, WorldEvents& events, bool ignoreArmor) {
	if (god)
		return 0;
	const int s = static_cast<int>(scale);
	int lost = 0;
	if (Alive()) {
		const int hit = stats.HitDamage(dmg, mix, ignoreArmor);
		lost = std::min(hit, stats.CurrentHP());
		stats.LoseHP(hit);
		blood.Splash(s);
		events.Note(FieldNote::Health);
	}

	if (!Alive() && state != ModelState::Die) {
		die(events);
		blood.Splash(s);
		for (int i = 0; i < 6; i++)
			blood.Explode();
	}
	return lost;
}

void Player::die(WorldEvents& events) {
	setModelState(ModelState::Die);
	events.PlayCharacter(PLAYER_CHARACTER, CharacterSound::Die);
	stats.poison.Cure();
	stats.EndResistance();
}

void Player::Poison(PoisonTier tier, WorldEvents& events, Rng& rng) {
	if (!Alive())
		return;
	if (stats.PoisonResistPercent() > 0 && rng.percent(stats.PoisonResistPercent())) {
		events.Status(stats.PotionResistPercent() > 0 ? "You resist the poison" : "Your amulet wards off the poison");
		return;
	}
	if (!stats.poison.Any())
		events.Status("You are poisoned!");
	stats.poison.Apply(tier);
	events.Note(FieldNote::Poison);
}

void Player::UpdatePoison(WorldEvents& events) {
	stats.AdvanceResistance(UPDATE_TICK_MS);
	const int hp = stats.poison.Advance(UPDATE_TICK_MS);
	if (hp <= 0 || !Alive() || god)
		return;
	stats.LoseHP(hp); // armour does not help
	if (!Alive())
		die(events);
}

void Player::Reanimate() {
	stats.HealFully();
	stats.poison.Cure();
	stats.EndResistance();
	setModelState(model.reference);
	blood.Stop(); // a new or loaded game starts without the last game's splash
}

void Player::showClimb(float phase) {
	const ClipInfo& climb = model.Clip(ModelState::Climb);
	if (!climb.present)
		return;
	state = ModelState::Climb;
	int frames = climb.frames;
	float frame = std::min(phase - std::floor(phase), 1.f) * static_cast<float>(frames);
	playback[static_cast<int>(ModelState::Climb)] = {std::min(frame, static_cast<float>(frames) - 0.001f), 0};
}
